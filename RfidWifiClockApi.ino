#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_PN532.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <time.h>

// ======================================
// WiFi Settings
// ======================================
const char* WIFI_SSID     = "hasom";
const char* WIFI_PASSWORD = "1234567890";

// API Endpoint
const char* API_URL = "http://52.78.56.158/api/log_scan.php";

// NTP (Korea Standard Time: UTC+9)
#define TZ_OFFSET_SEC 32400
const char* NTP_SERVERS[] = {
  "time.google.com",
  "pool.ntp.org",
  "time.cloudflare.com"
};

// ======================================
// Pin Configuration
// ======================================
#define OLED_SDA    14  // D5
#define OLED_SCL    12  // D6
#define PN532_IRQ   4   // D2
#define PN532_RESET 5   // D1
#define BUZZER_PIN  13  // D7

// ======================================
// Display
// ======================================
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_PN532   nfc(PN532_IRQ, PN532_RESET);

bool wifiConnected  = false;
bool timeAvailable  = false;

// ======================================
// Sound Functions
// ======================================
void playBootSound() {
  tone(BUZZER_PIN, 523, 120); delay(160);
  tone(BUZZER_PIN, 659, 120); delay(160);
  tone(BUZZER_PIN, 784, 200); delay(250);
  noTone(BUZZER_PIN);
}

void playCardSound() {
  tone(BUZZER_PIN, 1047, 80); delay(120);
  noTone(BUZZER_PIN);         delay(60);
  tone(BUZZER_PIN, 1047, 80); delay(120);
  noTone(BUZZER_PIN);
}

void playErrorSound() {
  tone(BUZZER_PIN, 400, 200); delay(250);
  tone(BUZZER_PIN, 300, 300); delay(350);
  noTone(BUZZER_PIN);
}

// ======================================
// Status Display Helper
// ======================================
void showStatus(String title, String line1, String line2 = "", String line3 = "", String line4 = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(title);
  display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
  display.setCursor(0, 14);
  display.println(line1);
  if (line2 != "") display.println(line2);
  if (line3 != "") display.println(line3);
  if (line4 != "") display.println(line4);
  display.display();
}

// ======================================
// WiFi Connection
// ======================================
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  showStatus("[ WiFi ]", "Connecting to:", String(WIFI_SSID), "Please wait...");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    attempts++;
    if (attempts % 2 == 0) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.println("[ WiFi Connecting... ]");
      display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
      display.setCursor(0, 14);
      display.println("SSID: " + String(WIFI_SSID));
      display.println("Attempt: " + String(attempts) + "/40");
      int progress = map(attempts, 0, 40, 0, SCREEN_WIDTH - 2);
      display.drawRect(0, 48, SCREEN_WIDTH, 12, SSD1306_WHITE);
      display.fillRect(2, 50, progress, 8, SSD1306_WHITE);
      display.display();
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    String ip = WiFi.localIP().toString();
    Serial.println("WiFi OK! IP: " + ip);
    showStatus("[ WiFi OK ]", "SSID: " + String(WIFI_SSID), "IP: " + ip, "Syncing NTP time...");
    delay(800);

    for (int i = 0; i < 3 && !timeAvailable; i++) {
      configTime(TZ_OFFSET_SEC, 0, NTP_SERVERS[i]);
      Serial.print("NTP sync: "); Serial.println(NTP_SERVERS[i]);
      showStatus("[ NTP Sync ]", "Server: " + String(NTP_SERVERS[i]), "Waiting...", "IP: " + ip);

      unsigned long startMs = millis();
      while (millis() - startMs < 6000) {
        time_t now = time(nullptr);
        if (now > 1609459200UL) {
          timeAvailable = true;
          Serial.println("Time sync OK!");
          break;
        }
        delay(200);
      }
    }

    if (timeAvailable) {
      showStatus("[ System Ready ]",
        "WiFi: " + String(WIFI_SSID),
        "IP: " + ip,
        "Time: OK (UTC+9)");
    } else {
      showStatus("[ Warning ]",
        "WiFi: OK",
        "NTP: FAILED",
        "Time unavailable.",
        "Check internet.");
    }
    delay(1200);

  } else {
    wifiConnected = false;
    Serial.println("WiFi FAILED!");
    showStatus("[ WiFi FAILED ]",
      "SSID: " + String(WIFI_SSID),
      "Check hotspot is ON",
      "Continuing without",
      "WiFi & time...");
    playErrorSound();
    delay(2000);
  }
}

// ======================================
// Time Helpers
// ======================================
String getTimeString() {
  if (!timeAvailable) return "--:--:--";
  time_t now = time(nullptr);
  struct tm* t = localtime(&now);
  char buf[12];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
  return String(buf);
}

String getDateString() {
  if (!timeAvailable) return "No NTP Time";
  time_t now = time(nullptr);
  struct tm* t = localtime(&now);
  const char* days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
  char buf[24];
  snprintf(buf, sizeof(buf), "%04d.%02d.%02d(%s)",
    t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, days[t->tm_wday]);
  return String(buf);
}

// ======================================
// Setup
// ======================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== System Boot ===");
  pinMode(BUZZER_PIN, OUTPUT);

  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("OLED init failed!");
    for (;;);
  }

  showStatus("[ System Boot ]", "OLED: OK", "Initializing...");
  delay(500);

  // WiFi + NTP
  connectWiFi();

  // PN532 Init
  showStatus("[ PN532 RFID ]", "Initializing...");
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    showStatus("[ ERROR ]", "PN532 not found!", "Check connections:", "SDA:D5 SCL:D6", "IRQ:D2 RST:D1");
    playErrorSound();
    while (1);
  }
  nfc.SAMConfig();
  char fwStr[20];
  snprintf(fwStr, sizeof(fwStr), "FW: %d.%d", (versiondata>>16)&0xFF, (versiondata>>8)&0xFF);
  showStatus("[ PN532 OK ]", String(fwStr));
  delay(500);

  playBootSound();
  Serial.println("System ready!");
}

// ======================================
// Main Loop
// ======================================
int scanAnimFrame = 0;

void loop() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Date line
  display.setCursor(0, 0);
  display.println(getDateString());

  // Time (large)
  display.setTextSize(2);
  display.setCursor(8, 11);
  display.println(getTimeString());

  // Divider
  display.setTextSize(1);
  display.drawFastHLine(0, 31, SCREEN_WIDTH, SSD1306_WHITE);

  // WiFi status
  display.setCursor(0, 34);
  if (wifiConnected) {
    display.print("WiFi: "); display.println(WIFI_SSID);
  } else {
    display.println("WiFi: Disconnected");
  }

  // RFID scan animation
  display.setCursor(0, 46);
  display.print("RFID Scanning");
  for (int i = 0; i < (scanAnimFrame % 4); i++) display.print(".");
  display.display();
  scanAnimFrame++;

  // RFID read (non-blocking, 300ms)
  uint8_t uid[7] = {0};
  uint8_t uidLength;
  bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 300);

  if (success) {
    // Format UID string
    String uidStr = "";
    for (uint8_t i = 0; i < uidLength; i++) {
      if (uid[i] < 0x10) uidStr += "0";
      uidStr += String(uid[i], HEX);
      if (i < uidLength - 1) uidStr += ":";
    }
    uidStr.toUpperCase();

    Serial.println("=== CARD DETECTED ===");
    Serial.println("UID: " + uidStr);

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("====================");
    display.println("   CARD DETECTED!   ");
    display.println("====================");
    display.println("UID : " + uidStr);
    display.println("Verifying with API...");
    display.display();

    // Send HTTP POST to PHP API
    bool isAllowed = false;
    String userName = "Unknown";
    
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      HTTPClient http;
      
      http.begin(client, API_URL);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      
      String postData = "uid=" + uidStr;
      Serial.println("Sending API POST: " + postData);
      
      int httpCode = http.POST(postData);
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("Response: " + payload);
        
        // Simple JSON checking
        if (payload.indexOf("\"allowed\":true") >= 0) {
          isAllowed = true;
          
          // Parse Name (ASCII or English fallback if present, but since Korea uses UTF-8 and SSD1306 uses ASCII, 
          // we can check if a name is returned and if it's displayable, otherwise we just print standard APPROVED screen)
          int nameStart = payload.indexOf("\"name\":\"");
          if (nameStart >= 0) {
            nameStart += 8;
            int nameEnd = payload.indexOf("\"", nameStart);
            if (nameEnd >= 0) {
              userName = payload.substring(nameStart, nameEnd);
              // If it contains unicode escapes like \uXXXX, we fallback to "Access Granted" for OLED safety
              if (userName.indexOf("\\u") >= 0) {
                userName = "Access Granted";
              }
            }
          }
        }
      } else {
        Serial.printf("HTTP POST failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    } else {
      Serial.println("WiFi Disconnected. Cannot verify card.");
    }

    // OLED display access result & Buzzer
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    
    if (isAllowed) {
      display.println("====================");
      display.println("   ACCESS GRANTED   ");
      display.println("====================");
      display.println("");
      display.println("User: " + userName);
      display.println("UID : " + uidStr);
      display.display();
      playCardSound(); // Success double beep
    } else {
      display.println("====================");
      display.println("   ACCESS DENIED!   ");
      display.println("====================");
      display.println("");
      display.println("Unauthorized Card");
      display.println("UID : " + uidStr);
      display.display();
      playErrorSound(); // Error buzzer tone
    }

    delay(2500); // Hold result screen
  }
}
