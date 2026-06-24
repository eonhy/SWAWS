#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_PN532.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// I2C Pins (shared by OLED and PN532)
// SDA = D5 (GPIO 14), SCL = D6 (GPIO 12)
#define OLED_SDA 14
#define OLED_SCL 12

// PN532 Control Pins
#define PN532_IRQ   4  // D2 (GPIO 4)
#define PN532_RESET 5  // D1 (GPIO 5)

// Buzzer Pin
#define BUZZER_PIN 13  // D7 (GPIO 13)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

// ======================================
// Sound Functions
// ======================================

// Boot melody: ascending 3-note chime (C5 -> E5 -> G5)
void playBootSound() {
  tone(BUZZER_PIN, 523, 120); // C5
  delay(160);
  tone(BUZZER_PIN, 659, 120); // E5
  delay(160);
  tone(BUZZER_PIN, 784, 200); // G5
  delay(250);
  noTone(BUZZER_PIN);
}

// Card detect sound: quick double-beep
void playCardSound() {
  tone(BUZZER_PIN, 1047, 80); // C6
  delay(120);
  noTone(BUZZER_PIN);
  delay(60);
  tone(BUZZER_PIN, 1047, 80); // C6
  delay(120);
  noTone(BUZZER_PIN);
}

// Error sound: descending sad beep
void playErrorSound() {
  tone(BUZZER_PIN, 400, 200);
  delay(250);
  tone(BUZZER_PIN, 300, 300);
  delay(350);
  noTone(BUZZER_PIN);
}

// ======================================
// Setup
// ======================================
void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);

  // Initialize I2C with custom pins
  Wire.begin(OLED_SDA, OLED_SCL);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 OLED allocation failed");
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("System Booting...");
  display.println("Init OLED: OK");
  display.display();
  delay(500);

  // Initialize PN532
  Serial.println("Initializing PN532...");
  display.println("Init PN532 RFID...");
  display.display();

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("ERROR: PN532 Not Found!");
    display.println("Check connections.");
    display.display();
    playErrorSound();
    while (1);
  }

  Serial.print("Found PN532! FW: ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

  display.printf("PN532 Found! FW:%d.%d\n", (versiondata>>16) & 0xFF, (versiondata>>8) & 0xFF);
  display.display();
  delay(500);

  nfc.SAMConfig();

  // Play boot sound after successful initialization
  playBootSound();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("=== RFID SCANNER ===");
  display.println("  System Ready!");
  display.println("");
  display.println("  Waiting for card.");
  display.display();

  Serial.println("System ready. Waiting for card...");
  delay(1000);
}

// ======================================
// Main Loop
// ======================================
int scanAnimFrame = 0;

void loop() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("=== RFID SCANNER ===");
  display.println("");
  display.println("Place card near");
  display.println("the PN532 reader...");
  display.println("");
  display.print("Scanning");
  for (int i = 0; i < (scanAnimFrame % 4); i++) {
    display.print(".");
  }
  display.display();
  scanAnimFrame++;

  // Try to read a card (non-blocking, 300ms timeout)
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;
  bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 300);

  if (success) {
    // Print to Serial
    Serial.println("============================");
    Serial.println("  CARD DETECTED!");
    Serial.print("  UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
    Serial.print("  UID Value : ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("============================");

    // Display on OLED
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("====================");
    display.println("   CARD DETECTED!   ");
    display.println("====================");
    display.println("");
    display.printf("Type: Mifare(%dB)\n", uidLength);
    display.print("UID : ");
    for (uint8_t i = 0; i < uidLength; i++) {
      display.printf("%02X ", uid[i]);
    }
    display.display();

    // Play card detection sound
    playCardSound();

    delay(2500);
  }
}
