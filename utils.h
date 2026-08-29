#include <Arduino_BuiltIn.h>

#include "certs.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <OneWire.h>
#include <DallasTemperature.h>


#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"
#define WLED 27
#define RELAY 22

void publishMessage(float metricsValue, String msg);

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

// Example for Brazil (Brasília Time: UTC-3, no DST)
const long  gmtOffset_sec = -10800; // -3 hours * 3600 seconds
const int   daylightOffset_sec = 0;
// controller parameters and data update rate
unsigned long previousMillis = 0;
const int interval = 3000;
float temp, previousTemp, setpoint = 0;
bool relayStatus = true;

const int oneWireBus = 4; // GPIO where the DS18B20 is connected to
OneWire oneWire(oneWireBus); // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensor(&oneWire); // Pass our oneWire reference to Dallas Temperature sensor 

void relay_toggle (bool *relay) {
  if (*relay) {
    String msg = "Relay off, current temp below setpoint";
    digitalWrite (RELAY, HIGH);
    publishMessage(temp, msg);
    Serial.println(msg);
    *relay = false; // run till temp get back to setpoint
  } else {
    String msg = "Relay on";
    digitalWrite (RELAY, LOW);
    publishMessage(temp, msg);
    Serial.println(msg);
    *relay = true;
  }
}

void setTemp (float t) {
  setpoint = t;
  publishMessage (t, "temp updated by user");
  Serial.println("setpoint updated successfully");
}

void messageHandler(char* topic, byte* payload, unsigned int length) {
  Serial.print("incoming: ");
  Serial.println(topic);
 
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);

  if (doc["relay"])
    doc["relay"] == "on" ? digitalWrite(RELAY, LOW) : digitalWrite(RELAY, HIGH);
  //const char* message = doc["message"];
  if (doc["temp"])
    publishMessage(temp, "temp requested by user");
  if (doc["setpoint"])
    setTemp (doc["setpoint"]);
}

void connectWIFI() {
  WiFi.mode(WIFI_STA);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  Serial.println("Connecting to Wi-Fi");
 
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
  
  // Optional POSIX TZ string alternative for automatic rules
  setenv("TZ", "BRT3", 1);
  tzset();
}

void connectAWS() {
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
 
  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);
 
  // Create a message handler
  client.setCallback(messageHandler);
 
  Serial.println("Connecting to AWS IOT");
 
  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }
 
  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }
 
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
 
  Serial.println("AWS IoT Connected!");
}

// This function acts like your "interrupt service routine"
void WiFiEvent(WiFiEvent_t event) {
    switch(event) {
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("Connected to Wi-Fi network AP!");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("IP address assigned: ");
            Serial.println(WiFi.localIP());
            connectAWS();
            digitalWrite(WLED, LOW);
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("Wi-Fi connection lost! Attempting reconnect...");
            digitalWrite(WLED, HIGH);
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // Auto-reconnect rule
            break;
    }
}

void publishMessage(float metricsValue, String msg) {
  StaticJsonDocument<200> doc;
  struct tm timeinfo;
  char timeStr[64];
  doc["temperature"] = metricsValue;
  if (getLocalTime(&timeinfo)) {
    strftime(timeStr, sizeof(timeStr), "%A, %d %B %Y %H:%M:%S", &timeinfo);
    doc["time"] = timeStr;
  }
  doc["msg"] = msg;

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
 
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}