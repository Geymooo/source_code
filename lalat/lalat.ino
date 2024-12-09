#define BLYNK_TEMPLATE_ID "TMPL6GrhgesU7"
#define BLYNK_TEMPLATE_NAME "Lalat"
#define BLYNK_AUTH_TOKEN "p6IEVg-3SYz75ScgA2GyHFPobIk4AS-H"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <BlynkSimpleEsp8266.h>

// Konfigurasi WiFi dan Google Sheets
const char* ssid = "Maggot Pass";
const char* pass = "03081987";
const char* host = "script.google.com";
const int httpsPort = 443;
WiFiClientSecure client;
String GAS_ID = "AKfycbx8YAoG89nIIokmhcA6kr8NxCxkZLtaqbbZMI_PanU1LQyVZr-Xz_VnbqjkYGsaNDrY";

// Konfigurasi pin dan perangkat
#define DHTPIN 5 // D1
#define RELAYPIN1 4 // D2
#define RELAYPIN2 12 // D6
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

bool relayStatus1 = false;
bool relayStatus2 = false;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Terhubung ke WiFi!");

  Wire.begin(2, 0); // LCD | SDA (D4) SCL (D3)

  dht.begin();
  lcd.init();
  lcd.backlight();

  pinMode(RELAYPIN1, OUTPUT);
  pinMode(RELAYPIN2, OUTPUT);
  digitalWrite(RELAYPIN1, HIGH);
  digitalWrite(RELAYPIN2, HIGH);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  client.setInsecure();
}

void loop() {
  Blynk.run();
  float suhu = dht.readTemperature();
  float kelembaban = dht.readHumidity();

  if (suhu < -40 || suhu > 125) {  // Validasi suhu
    Serial.println("Gagal membaca sensor DHT22!");
    lcd.setCursor(0, 1);
    lcd.print("Sensor error!    ");
    delay(2000);
    return;
`}

  // Kirim data ke Blynk
  Blynk.virtualWrite(V1, suhu);
  Blynk.virtualWrite(V2, kelembaban);

  // Tampilkan data di LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("S: ");
  lcd.print(suhu);
  lcd.print(" H: ");
  lcd.print(kelembaban);

  // Kirim data ke Google Sheets
  sendDataToGoogleSheets(suhu, kelembaban);
  delay(5000); // Kirim data setiap 5 detik
}

void sendDataToGoogleSheets(float suhu, float kelembaban) {
  Serial.println("==========");
  Serial.print("connecting to ");
  Serial.println(host);

  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }

  String string_temperature = String(suhu);
  String string_humidity = String(kelembaban);
  String url = "/macros/s/" + GAS_ID + "/exec?temperature=" + string_temperature + "&humidity=" + string_humidity;

  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("request sent");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("Data berhasil dikirim ke Google Sheets!");
  } else {
    Serial.println("Gagal mengirim data ke Google Sheets.");
  }
  client.stop();
  Serial.println("==========");
}

// Relay 1 kontrol via Blynk
BLYNK_WRITE(V3) {
  relayStatus1 = param.asInt();
  digitalWrite(RELAYPIN1, relayStatus1);
  lcd.setCursor(0, 1);
  lcd.print(relayStatus1 ? "Relay 1 ON" : "Relay 1 OFF");
  Serial.println(relayStatus1 ? "Relay 1 ON" : "Relay 1 OFF");
}

// Relay 2
BLYNK_WRITE(V4) {
  relayStatus2 = param.asInt();
  digitalWrite(RELAYPIN2, relayStatus2);
  lcd.setCursor(0, 1);
  lcd.print(relayStatus2 ? "Relay 2 ON" : "Relay 2 OFF");
  Serial.println(relayStatus2 ? "Relay 2 ON" : "Relay 2 OFF");
}
