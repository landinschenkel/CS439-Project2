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

TM1637Display display(CLK_PIN, DIO_PIN);
WiFiClient client;

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

  // Ensure button is released before starting new round
  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(10);
  }

  // Wait for button press to arm the game
  while (digitalRead(BUTTON_PIN) == HIGH) {
    delay(10);
  }

  display.clear();
  Serial.println("Round armed. Waiting for random delay...");

  // Random delay between 2 to 5 seconds
  unsigned long randomDelay = random(2000, 5000);
  unsigned long startTime = millis();

  // Non-blocking wait that checks for early false starts
  bool falseStart = false;
  while (millis() - startTime < randomDelay) {
    if (digitalRead(BUTTON_PIN) == LOW) {
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

  // Wait for player to press button in response to buzzer
  while (digitalRead(BUTTON_PIN) == HIGH) {
    // Waiting for reaction press
  }

  // Turn off buzzer immediately
  digitalWrite(BUZZER_PIN, LOW);
  unsigned long reactionTime = millis() - reactionStart;

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