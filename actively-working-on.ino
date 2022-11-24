#include <ESP8266WiFi.h>
#include "FirebaseESP8266.h"
#include <DHT.h>

#ifndef STASSID
#define STASSID "ZTE_2.4G_ANGQrk"
#define STAPSK  "tifaniyo"
#endif

#define FIREBASE_HOST "fathinlka-default-rtdb.asia-southeast1.firebasedatabase.app" 
#define FIREBASE_AUTH "J60tG3NEI0NakoIeam9oizPTrbuYL6DSHdzDPCde"

#define DHTPIN D1
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define fan1Pin D2

FirebaseData firebaseData;
FirebaseJson json;

const char* ssid     = STASSID;
const char* password = STAPSK;

float h;
float t;

void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);

  Serial.println("IP address: " + WiFi.localIP().toString());

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  pinMode(fan1Pin, OUTPUT);
}

void loop() {
  sensorUpdate();
  relayUpdate();

  if (Firebase.getBool(firebaseData, "/Fan/isAuto")){
    if (firebaseData.boolData()) {
      Serial.println("Fan is in auto");
      if(t>30){
        digitalWrite(fan1Pin, HIGH);
        Serial.println("Fan is on");
      } else {
        digitalWrite(fan1Pin, LOW);
        Serial.println("Fan is off");
      }
    } else {
      Serial.println("Fan is in manual");
      fanUpdate();
    }
  }

  Serial.println("-------------------------");
  Serial.println();
  delay(1000);
}

void sensorUpdate(){
  h = dht.readHumidity();
  t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("C"));
  Serial.println();

  if (Firebase.setFloat(firebaseData, "/Temp/suhu1", t)) {
    Serial.println("suhu1 update success");
    } else {
      Serial.println("suhu1 update failure");
      Serial.println("REASON: " + firebaseData.errorReason());
      Serial.println("------------------------------------");
      Serial.println();
  }

  if (Firebase.setFloat(firebaseData, "/Temp/hum1", h)) {
    Serial.println("hum1 update success");
    } else {
      Serial.println("hum1 update failure");
      Serial.println("REASON: " + firebaseData.errorReason());
      Serial.println("------------------------------------");
      Serial.println();
  }
}

void relayUpdate(){
  if (Firebase.getString(firebaseData, "/Relay/StatusRelay1")){
    if (firebaseData.stringData() == "ON") {
    Serial.println("Relay 1 is ON");
    } else {
      Serial.println("Relay 1 is OFF");
      }
  }

  if (Firebase.getString(firebaseData, "/Relay/StatusRelay2")){
    if (firebaseData.stringData() == "ON") {
    Serial.println("Relay 2 is ON");
    } else {
      Serial.println("Relay 2 is OFF");
      }
  }
}

void fanUpdate(){
  if (Firebase.getString(firebaseData, "/Fan/fan1")){
    if (firebaseData.stringData() == "ON") {
    Serial.println("fan 1 is ON");
    digitalWrite(fan1Pin, HIGH);
    } else {
      Serial.println("fan 1 is OFF");
      digitalWrite(fan1Pin, LOW);
      }
  }
}