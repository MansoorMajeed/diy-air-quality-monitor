#include <DHT.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>

// https://github.com/fu-hsi/PMS
#include "PMS.h"

// https://github.com/plapointe6/EspMQTTClient
#include "EspMQTTClient.h"

// https://github.com/WifWaf/MH-Z19
#include "MHZ19.h"

// Serial Port init
// For the PM Sensor
HardwareSerial pmsSerial(1);
// For the CO2 Sensor
HardwareSerial co2Serial(2);    // Init UART for MHZ19


PMS pms(pmsSerial);
PMS::DATA data;


const int PMS_RX = 32;
const int PMS_TX = 33;

MHZ19 myMHZ19;

#define CO2_TX 16
#define CO2_RX 17
#define CO2_BAUDRATE 9600


// Temp and humidity sensor
#define DHTPIN 21  // GPIO pin number
#define DHTTYPE DHT11 // change the type if using different version

DHT dht(DHTPIN, DHTTYPE);

// WiFi setup
const char* ssid = "Your Wifi SSID";
const char* password = "Your Wifi Password";

// MQTT broker setup
const char* mqtt_broker = "192.168.0.70 MQTT Broker IP Address";
const char* mqtt_username = "mqtt_username";
const char* mqtt_password = "mqtt_password";
const char* mqtt_sensor_topic = "sensors/readings";

int mhz19_temp;
int mhz19_co2;


EspMQTTClient client(
  ssid,
  password,
  mqtt_broker,  // MQTT Broker server ip
  mqtt_username,   // Can be omitted if not needed
  mqtt_password,   // Can be omitted if not needed
  "Esp32"      // Client name that uniquely identify your device
);



void setup() {
  Serial.begin(115200);
  delay(1000);

  dht.begin();

  pmsSerial.begin(9600, SERIAL_8N1, PMS_RX, PMS_TX);

  co2Serial.begin(CO2_BAUDRATE, SERIAL_8N1, CO2_RX, CO2_TX);

  myMHZ19.begin(co2Serial);
  myMHZ19.autoCalibration();

  // using passive mode on the PMS5003. This means we can read
  // values from the sensor when we want to. Active mode on the other
  // hand keeps sending data
  pms.passiveMode();
}

void onConnectionEstablished() {
  client.publish("mytopic/test", "connected");
}

void loop() {

  client.loop();

  // The sensor is runninng continuously
  // not the best for the life of it, but gives accurate readings
  // pms.wakeUp();
  // delay(10000);


  // Humidity and temperature sensor readings
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" *C ");

  mhz19_co2 = myMHZ19.getCO2();   

  if(myMHZ19.errorCode == RESULT_OK) {
    Serial.print("CO2 Value successfully Received: ");
    Serial.println(mhz19_co2);
    Serial.print("Response Code: ");
    Serial.println(myMHZ19.errorCode);
  } 
  else 
  {
    Serial.println("Failed to recieve CO2 value - Error");
    Serial.print("Response Code: ");
    Serial.println(myMHZ19.errorCode);  
  }

  // In my testing, the temperature reading from MHZ19 was not accurate.
  // maybe I am misunderating the use of that one
  mhz19_temp = myMHZ19.getTemperature();   

  Serial.print("Co2: ");
  Serial.print(mhz19_co2);
  Serial.print("...\t");
  Serial.print("temp: ");
  Serial.print(mhz19_temp);
  Serial.println(" C");


  pms.requestRead();  // In passive mode, request data
  if (pms.readUntil(data)) {  
    Serial.print("PM 1.0 (ug/m3): ");
    Serial.println(data.PM_AE_UG_1_0);
    Serial.print("PM 2.5 (ug/m3): ");
    Serial.println(data.PM_AE_UG_2_5);
    Serial.print("PM 10 (ug/m3): ");
    Serial.println(data.PM_AE_UG_10_0);

    Serial.println(String(PM_TO_AQI_US(data.PM_AE_UG_2_5)));

  } else {
    Serial.println("Failed to read from sensor!");
  }

  // Create a single JSON object to hold all the sensor readings
  StaticJsonDocument<300> doc;
  doc["temp"] = t;
  doc["humidity"] = h;
  doc["co2"] = mhz19_co2;
  doc["mh_temp"] = mhz19_temp;
  doc["pm_1"] = data.PM_AE_UG_1_0;
  doc["pm_2_5"] = data.PM_AE_UG_2_5;
  doc["pm_10"] = data.PM_AE_UG_10_0;
  doc["us_aqi"] = PM_TO_AQI_US(data.PM_AE_UG_2_5);
  char payload[300];
  serializeJson(doc, payload);


  // Send all sensor readings to one topic
  if (client.publish(mqtt_sensor_topic, payload)){
    Serial.println("Publish success");
  } else {
    Serial.println("Publish failed");
  }

  // Sleep if you want to save power and extend the life of the PM sensor
  // but then make sure to wake up the sensor before reading data
  // pms.sleep();
  delay(5000);
}


// A function that converts PM2.5 to US Air Quality Index
// Copied it from AirGradient's code
int PM_TO_AQI_US(int pm02) {
  if (pm02 <= 12.0) return ((50 - 0) / (12.0 - .0) * (pm02 - .0) + 0);
  else if (pm02 <= 35.4) return ((100 - 50) / (35.4 - 12.0) * (pm02 - 12.0) + 50);
  else if (pm02 <= 55.4) return ((150 - 100) / (55.4 - 35.4) * (pm02 - 35.4) + 100);
  else if (pm02 <= 150.4) return ((200 - 150) / (150.4 - 55.4) * (pm02 - 55.4) + 150);
  else if (pm02 <= 250.4) return ((300 - 200) / (250.4 - 150.4) * (pm02 - 150.4) + 200);
  else if (pm02 <= 350.4) return ((400 - 300) / (350.4 - 250.4) * (pm02 - 250.4) + 300);
  else if (pm02 <= 500.4) return ((500 - 400) / (500.4 - 350.4) * (pm02 - 350.4) + 400);
  else return 500;
};
