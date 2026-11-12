#include <TM1637Display.h>
#include <WiFiS3.h>

const char* ssid     = "Landin's iphone (2)";     // Ensure no spaces/apostrophes if renamed
const char* password = "Landin48";

// Manually assign a static IP within your iPhone hotspot's subnet (e.g., 172.20.10.15)
IPAddress ip(172, 20, 10, 15);
IPAddress gateway(172, 20, 10, 1);
IPAddress subnet(255, 255, 255, 0);

const char* serverIP = "172.20.10.2"; // Your Mac's IP on the hotspot
const int   serverPort = 5000;

const int BUTTON_PIN = 2;
const int CLK_PIN    = 4;
const int DIO_PIN    = 5;
const int BUZZER_PIN = 8;
const unsigned long BUTTON_DEBOUNCE_MS = 30;

TM1637Display display(CLK_PIN, DIO_PIN);
WiFiClient client;

void waitForStableButtonState(uint8_t targetState) {
  unsigned long stableSince = millis();

  while (millis() - stableSince < BUTTON_DEBOUNCE_MS) {
    if (digitalRead(BUTTON_PIN) != targetState) {
      stableSince = millis();
    }
    delay(1);
  }
}

bool isButtonPressed() {
  if (digitalRead(BUTTON_PIN) != LOW) {
    return false;
  }

  unsigned long pressedSince = millis();
  while (millis() - pressedSince < BUTTON_DEBOUNCE_MS) {
    if (digitalRead(BUTTON_PIN) != LOW) {
      return false;
    }
    delay(1);
  }
  return true;
}

unsigned long waitForButtonPress() {
  while (true) {
    while (digitalRead(BUTTON_PIN) != LOW) {
      delay(1);
    }

    unsigned long pressedAt = millis();
    if (isButtonPressed()) {
      return pressedAt;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== SYSTEM BOOTING ===");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  display.setBrightness(0x0f);

  uint8_t readySegments[] = { SEG_G, SEG_G, SEG_G, SEG_G };
  display.setSegments(readySegments);

  // Apply static IP configuration before connecting
  WiFi.config(ip, gateway, subnet);

  Serial.print("Connecting to Wi-Fi with static IP: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}
void sendScoreToBackend(unsigned long reactionTime) {
  if (client.connect(serverIP, serverPort)) {
    String jsonBody = "{\"reaction_time\":" + String(reactionTime) + "}";
    client.println("POST /api/score HTTP/1.1");
    client.print("Host: "); client.println(serverIP);
    client.println("Content-Type: application/json");
    client.print("Content-Length: "); client.println(jsonBody.length());
    client.println("Connection: close");
    client.println();
    client.println(jsonBody);
    client.stop();
  }
}

void loop() {
  Serial.println("Waiting for button press to start round...");

  waitForStableButtonState(HIGH);
  waitForButtonPress();
  display.showNumberDec(0, true);
  Serial.println("Round armed. Waiting for random delay...");

  // The start press must be released before the reaction window begins.
  waitForStableButtonState(HIGH);

  // Random delay between 2 to 5 seconds
  unsigned long randomDelay = random(2000, 5000);
  unsigned long startTime = millis();

  bool falseStart = false;
  while (millis() - startTime < randomDelay) {
    if (isButtonPressed()) {
      falseStart = true;
      break;
    }
  }

  if (falseStart) {
    Serial.println("False start! Button pressed too early.");
    // Show 'Err' on display
    uint8_t errSegments[] = { SEG_A | SEG_D | SEG_E | SEG_F | SEG_G, SEG_E | SEG_G, SEG_E | SEG_G, 0x00 };
    display.setSegments(errSegments);
    delay(2000);

    // Reset to ready dashes
    uint8_t readySegments[] = { SEG_G, SEG_G, SEG_G, SEG_G };
    display.setSegments(readySegments);
    return;
  }

  // Turn on buzzer to signal reaction time
  digitalWrite(BUZZER_PIN, HIGH);
  unsigned long reactionStart = millis();
  Serial.println("BUZZER ON! React now!");

  unsigned long pressedAt = waitForButtonPress();

  // Turn off buzzer immediately
  digitalWrite(BUZZER_PIN, LOW);
  unsigned long reactionTime = pressedAt - reactionStart;

  // Output score to display and backend
  display.showNumberDec(reactionTime, false);
  Serial.print("Reaction Time recorded: ");
  Serial.print(reactionTime);
  Serial.println(" ms");

  sendScoreToBackend(reactionTime);

  // Pause so user can read score
  delay(3000);

  // Reset display back to ready bars
  uint8_t readySegments[] = { SEG_G, SEG_G, SEG_G, SEG_G };
  display.setSegments(readySegments);
}