#include "utils.h"

void setup() {
  Serial.begin(115200);
  sensor.begin();
  WiFi.onEvent(WiFiEvent); // Register the event handler
  connectWIFI();
  
  pinMode(RELAY, OUTPUT);
  pinMode(WLED, OUTPUT);
  digitalWrite(RELAY, LOW);
  digitalWrite(WLED, HIGH);
}

void loop() {
  if (millis() - previousMillis >= interval) {
    sensor.requestTemperatures();
    temp = sensor.getTempCByIndex(0);
    if(temp == DEVICE_DISCONNECTED_C && previousTemp != temp) {
      publishNotification(previousTemp);
      previousTemp = temp;
    }
    // check variation over 2 from the previous one
    else if (temp - previousTemp > 2.0 || previousTemp - temp > 2.0) {
      Serial.print("On the threshold of target temperature: ");
      Serial.println(temp);
      publishMessage(temp, "On the threshold of target temperature");
      previousTemp = temp;
    }
    previousMillis = millis();
  }

  // reverse-act hysteresis controller 
  if (temp <= (setpoint - 2) && relayStatus)
    relay_toggle (&relayStatus);
  else if (temp >= (setpoint + 1) && !relayStatus)
    relay_toggle (&relayStatus);

  client.loop();
}
