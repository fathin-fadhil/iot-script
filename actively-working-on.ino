#include <ESP8266WiFi.h>
#include "FirebaseESP8266.h"
#include <DHT.h>
#include <PZEM004Tv30.h>
#include <SoftwareSerial.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2);

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

float voltage;
float current;
float power;
float energy;
float frequency;
float pf;
//end of pzem

//start of firebase
#define FIREBASE_HOST "fathinlka-default-rtdb.asia-southeast1.firebasedatabase.app" 
#define FIREBASE_AUTH "J60tG3NEI0NakoIeam9oizPTrbuYL6DSHdzDPCde"

FirebaseData firebaseData;
FirebaseJson json;
//end of firebase

//dht stuff
#define DHTPIN D2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float h;
float t;
//end of dht

//pin declaration
#define fan1Pin D1
#define relay1Pin D7

void setup() {
  Serial.begin(115200);
  dht.begin();
  Wire.begin(2,0);
  lcd.init();
  lcd.backlight();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  lcd.setCursor(0,0);
  lcd.print("Waiting for WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  lcd.clear();
  lcd.print("WiFi Connected");

  Serial.println("IP address: " + WiFi.localIP().toString());

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  pinMode(fan1Pin, OUTPUT);
  pinMode(relay1Pin, OUTPUT);
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
    digitalWrite(relay1Pin, HIGH);
    } else {
      Serial.println("Relay 1 is OFF");
      digitalWrite(relay1Pin, LOW);
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
      lcd.clear(); lcd.print("volt read Error");
  } else if (isnan(current)) {
      Serial.println("Error reading current");
      lcd.clear(); lcd.print("amps read Error");
  } else if (isnan(power)) {
      Serial.println("Error reading power");
      lcd.clear(); lcd.print("pwr read Error");
  } else if (isnan(energy)) {
      Serial.println("Error reading energy");
      lcd.clear(); lcd.print("energy read Err");
  } else if (isnan(frequency)) {
      Serial.println("Error reading frequency");
      lcd.clear(); lcd.print("Hz read Error");
  } else if (isnan(pf)) {
      Serial.println("Error reading power factor");
      lcd.clear(); lcd.print("pf read Error");
  } else {
      // Print the values to the Serial console
      Serial.print("Voltage: ");      Serial.print(voltage);      Serial.println("V");
      Serial.print("Current: ");      Serial.print(current);      Serial.println("A");
      Serial.print("Power: ");        Serial.print(power);        Serial.println("W");
      Serial.print("Energy: ");       Serial.print(energy,3);     Serial.println("kWh");
      Serial.print("Frequency: ");    Serial.print(frequency, 1); Serial.println("Hz");
      Serial.print("PF: ");           Serial.println(pf);

      //print ke el ce de
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(voltage, 1); lcd.print("V"); lcd.setCursor(7,0); lcd.print(energy, 3); lcd.print("kWh");
      lcd.setCursor(0,1);
      lcd.print(power); lcd.print("W"); lcd.setCursor(9,1); lcd.print(current); lcd.print("A");

      //upload basis api
      if (Firebase.setFloat(firebaseData, "/Wmeter/current", current)) {
      Serial.println("current update success");
      } else {
        Serial.println("current update failure");
        Serial.println("REASON: " + firebaseData.errorReason());
        Serial.println("------------------------------------");
        }

      if (Firebase.setFloat(firebaseData, "/Wmeter/tWatt", energy)) {
      Serial.println("energy update success");
      } else {
        Serial.println("energy update failure");
        Serial.println("REASON: " + firebaseData.errorReason());
        Serial.println("------------------------------------");
        }

      if (Firebase.setFloat(firebaseData, "/Wmeter/volt", voltage)) {
      Serial.println("voltage update success");
      } else {
        Serial.println("voltage update failure");
        Serial.println("REASON: " + firebaseData.errorReason());
        Serial.println("------------------------------------");
        }

      if (Firebase.setFloat(firebaseData, "/Wmeter/watt", power)) {
      Serial.println("power update success");
      } else {
        Serial.println("power update failure");
        Serial.println("REASON: " + firebaseData.errorReason());
        Serial.println("------------------------------------");
        }
  }
  Serial.println();
}