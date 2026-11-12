#include <TM1637Display.h>
#include <WiFiS3.h>

const char* ssid       = "IPFrequently67;
const char* password   = "ItHertzWhenIP67";
const char* serverIP   = "192.168.1.201"; // IP address
const int   serverPort = 5000;

const int BUTTON_PIN = 2;
const int CLK_PIN    = 4;
const int DIO_PIN    = 5;
const int BUZZER_PIN = 8;

TM1637Display display(CLK_PIN, DIO_PIN);
WiFiClient client;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  display.setBrightness(0x0f);

  uint8_t readySegments[] = { SEG_G, SEG_G, SEG_G, SEG_G };
  display.setSegments(readySegments);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
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
  while (digitalRead(BUTTON_PIN) == HIGH) {}

  display.clear();
  delay(random(2000, 5000));

  digitalWrite(BUZZER_PIN, HIGH);
  unsigned long startTime = millis();

  while (digitalRead(BUTTON_PIN) == HIGH) {}

  digitalWrite(BUZZER_PIN, LOW);
  unsigned long reactionTime = millis() - startTime;

  display.showNumberDec(reactionTime, false);
  sendScoreToBackend(reactionTime);

  delay(3000);
  uint8_t readySegments[] = { SEG_G, SEG_G, SEG_G, SEG_G };
  display.setSegments(readySegments);
}