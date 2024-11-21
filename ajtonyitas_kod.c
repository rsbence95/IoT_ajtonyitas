#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Wi-Fi és Blynk konfiguráció
#define WIFI_SSID "IRODA"  
#define WIFI_PASSWORD "Iroda22"
#define BLYNK_TEMPLATE_ID "TMPL4JYotc2CZ"
#define BLYNK_TEMPLATE_NAME "doorv2"
#define BLYNK_AUTH_TOKEN "rSiqgOYIVjFzghKKpfYaKV1upxdIHSgN"

// Kijelző és egyéb beállítások
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1  // OLED reset pin, ha nincs használatban, akkor -1
#define SERVO_PIN 14  // D5 = GPIO14 pin az ESP8266-on
#define BUTTON_PIN 13  // D7 = GPIO13 pin a gombhoz
#define SCREEN_ADDR 0x3C  // Az OLED kijelző I2C címe

// Időzóna eltérés: CET (UTC+1) → 3600 másodperc, CEST (UTC+2) → 7200 másodperc
const long timeZoneOffset = 3600;  // Állítsd 7200-ra nyári időszámításhoz

// NTP beállítások
WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", timeZoneOffset, 60000); // Időzóna kezeléssel

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // OLED kijelző objektum
Servo myServo;   // Szervó objektum

int doorOpenCount = 0; // Nyitások számának tárolása
unsigned long lastOpenTime = 0; // Az utolsó nyitás időpontja

void setup() {
  Serial.begin(115200);  // Soros kommunikáció indítása

  // Wi-Fi csatlakozás
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi kapcsolódva.");

  // Blynk inicializálása
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASSWORD);

  // Gomb beállítása
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Belső felhúzóellenállás a gombhoz

  // Szervó inicializálása
  myServo.attach(SERVO_PIN);  // A szervó csatlakoztatása a GPIO14 pinhez

  // OLED kijelző inicializálása
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDR)) {
    Serial.println(F("OLED kijelző nem található!"));
    while (true);  // Ha nem található a kijelző, megállítjuk a programot
  }

  display.clearDisplay();    // A kijelző törlése
  display.setTextSize(2);    // Betűméret beállítása
  display.setTextColor(SSD1306_WHITE);  // Fehér szöveg
  display.setCursor(0, 0);   // Bal felső sarok kezdése

  display.println(F("Ajto allapota:"));
  display.display();  // Frissítjük a kijelzőt

  // Szervó alaphelyzetbe (140 fokra) mozgatása - ajtó zárt állapot
  myServo.write(140);
  delay(500);  // 0.5 másodperc várakozás

  // OLED kijelző frissítése ajtó állapotának kiírására
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("Zarva"));
  display.display();

  // NTP időszinkronizálás
  timeClient.begin();
}

void sendDateTimeToBlynk() {
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime(); // Epoch idő lekérése és időzóna hozzáadása

  // Epoch időből dátum számítása (UTC alapján)
  epochTime += timeZoneOffset; // Időzóna eltérés hozzáadása

  int year = 1970 + epochTime / 31556926; // Hozzáadjuk az alapévet
  int month = (epochTime % 31556926) / 2629743 + 1; // Hónap számítása
  int day = ((epochTime % 2629743) / 86400) + 0; // Nap számítása

  // Formázott dátum és idő
  String formattedTime = timeClient.getFormattedTime();
  String formattedDate = String(year) + "-" + String(month) + "-" + String(day);

  // Küldjük a Blynk-hez
  Blynk.virtualWrite(V3, formattedTime); // Idő a V3 pinre
  Blynk.virtualWrite(V4, formattedDate); // Dátum a V4 pinre

  Serial.print("Datum: ");
  Serial.println(formattedDate);
  Serial.print("Ido: ");
  Serial.println(formattedTime);
}

void loop() {
  Blynk.run();  // Blynk működtetése

  int buttonState = digitalRead(BUTTON_PIN);  // Gomb állapotának olvasása

  if (buttonState == LOW) {  // Ha a gombot lenyomták
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("Nyitas..."));
    display.display();

    for (int angle = 140; angle >= 0; angle -= 5) {  // Nyitás
      myServo.write(angle);
      delay(5);
    }

    doorOpenCount++;  // Nyitások száma
    lastOpenTime = millis();

    sendDateTimeToBlynk();

    Blynk.virtualWrite(V1, doorOpenCount);

    delay(1000);  // 1 másodperc várakozás

    // Zárás
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("Zaras..."));
    display.display();

    for (int angle = 0; angle <= 140; angle += 5) {
      myServo.write(angle);
      delay(5);
    }

    delay(1000);
    myServo.write(140);
    delay(500);
  }
}
