#include <ESP8266WiFi.h>
#include "FirebaseESP8266.h"
#include <DHT.h>
#include <PZEM004Tv30.h>
#include <SoftwareSerial.h>

//start of wifi stuff
#ifndef STASSID
#define STASSID "ZTE_2.4G_ANGQrk"
#define STAPSK  "tifaniyo"
#endif

const char* ssid     = STASSID;
const char* password = STAPSK;
//end of wifi

//start of pzem
#if !defined(PZEM_RX_PIN) && !defined(PZEM_TX_PIN)
#define PZEM_RX_PIN D5
#define PZEM_TX_PIN D6
#endif

SoftwareSerial pzemSWSerial(PZEM_RX_PIN, PZEM_TX_PIN);
PZEM004Tv30 pzem(pzemSWSerial);

float voltage = 0.0;
float current = 0.0;
float power = 0.0;
float energy = 0.0;
float frequency = 0.0;
float pf = 0.0;
//end of pzem

//start of firebase
#define FIREBASE_HOST "fathinlka-default-rtdb.asia-southeast1.firebasedatabase.app" 
#define FIREBASE_AUTH "J60tG3NEI0NakoIeam9oizPTrbuYL6DSHdzDPCde"

FirebaseData firebaseData;
FirebaseJson json;
//end of firebase

//dht stuff
#define DHTPIN D1
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float h;
float t;
//end of dht

//pin declaration
#define fan1Pin D2

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
  fanUpdate();
  pzemRead();

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
  }
}

void pzemRead(){
  Serial.print("Custom Address:");
  Serial.println(pzem.readAddress(), HEX);

  voltage = pzem.voltage();
  current = pzem.current();
  power = pzem.power();
  energy = pzem.energy();
  frequency = pzem.frequency();
  pf = pzem.pf();

  // Check if the data is valid
  if(isnan(voltage)){
      Serial.println("Error reading voltage");
  } else if (isnan(current)) {
      Serial.println("Error reading current");
  } else if (isnan(power)) {
      Serial.println("Error reading power");
  } else if (isnan(energy)) {
      Serial.println("Error reading energy");
  } else if (isnan(frequency)) {
      Serial.println("Error reading frequency");
  } else if (isnan(pf)) {
      Serial.println("Error reading power factor");
  } else {

      // Print the values to the Serial console
      Serial.print("Voltage: ");      Serial.print(voltage);      Serial.println("V");
      Serial.print("Current: ");      Serial.print(current);      Serial.println("A");
      Serial.print("Power: ");        Serial.print(power);        Serial.println("W");
      Serial.print("Energy: ");       Serial.print(energy,3);     Serial.println("kWh");
      Serial.print("Frequency: ");    Serial.print(frequency, 1); Serial.println("Hz");
      Serial.print("PF: ");           Serial.println(pf);
  }
  Serial.println();
}