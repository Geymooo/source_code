#define BLYNK_AUTH_TOKEN "mt4A-3gL6Opdqo6Il_TmUpZRFHNWjD1n"
#define BLYNK_TEMPLATE_ID "TMPL6ZX8iR_9p"
#define BLYNK_TEMPLATE_NAME "Maggot"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Konfigurasi LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// WiFi Credentials
char ssid[] = "Maggot Pass";
char pass[] = "03081987";

// Konfigurasi Google Sheets
const char* googleHost = "script.google.com";
const int httpsPort = 443;
WiFiClientSecure client;
String GAS_ID = "AKfycbx0Cjpd7lFxy2cAkxAqkKWmciCF8a6ayMoukgI-cGyTfL2_7xvQi0JHRSNpO_ImKX4"; // Spreadsheet Script ID

// Pin Konfigurasi
#define LED_PIN D2
const int sensorPin = A0;

// Timer
BlynkTimer timer;

void setup() {
  // Serial & LCD Initialization
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.backlight();

  // WiFi & Blynk Initialization
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  Wire.begin(D5, D1); // Pin SDA dan SCL
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Koneksi Aman untuk Google Sheets
  client.setInsecure();

  // Tampilan Awal LCD
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  Serial.println("Setup selesai.");

  // Interval Pengiriman Data
  timer.setInterval(2000L, readAndSendSensorData);
}

void readAndSendSensorData() {
  // Membaca Kelembaban Tanah
  int analogValueMoisture = analogRead(sensorPin);
  float humidity = map(analogValueMoisture, 1023, 0, 0, 100);

  // Membaca pH
  int analogValuePH = analogRead(sensorPin); // Jika menggunakan pin sama
  float pH = analogValuePH * (14.0 / 1023.0);

  // Menampilkan Data di Serial Monitor
  Serial.print("Kelembaban Tanah: ");
  Serial.print(humidity);
  Serial.println(" %");
  Serial.print("pH Tanah: ");
  Serial.println(pH);

  // Menampilkan Data di LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tanah: " + String(humidity) + "%");
  lcd.setCursor(0, 1);
  lcd.print("pH: " + String(pH));

  // Mengirim Data ke Blynk
  Blynk.virtualWrite(V0, humidity);
  Blynk.virtualWrite(V1, pH);

  // Mengirim Data ke Google Sheets
  sendDataToGoogleSheets(humidity, pH);
}

void sendDataToGoogleSheets(float humidity, float pH) {
  Serial.println("==========");
  Serial.print("Menghubungkan ke ");
  Serial.println(googleHost);

  if (!client.connect(googleHost, httpsPort)) {
    Serial.println("Koneksi ke Google Sheets gagal.");
    return;
  }

  // Memproses URL untuk mengirim data
  String url = "/macros/s/" + GAS_ID + "/exec?pH=" + String(pH, 2) + "&humidity=" + String(humidity, 2);
  Serial.print("Mengirim URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + googleHost + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Connection: close\r\n\r\n");

  // Memeriksa Respons
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("Headers diterima.");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("Data berhasil dikirim ke Google Sheets!");
  } else {
    Serial.println("Pengiriman data ke Google Sheets gagal.");
  }
  Serial.println("==========");
}

BLYNK_WRITE(V2) {
  int relayState = param.asInt();
  digitalWrite(LED_PIN, relayState);
  if (relayState) {
    Serial.println("Relay ON");
  } else {
    Serial.println("Relay OFF");
  }
}

void loop() {
  Blynk.run();
  timer.run();
}
