#include <Arduino.h>

// dumps the ServiceData for 0x1a18 BLE advertisement.


#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Config.h>
#include <WiFi.h>
#include <M5StickC.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

int scanTime = 30; //In seconds
BLEScan* pBLEScan;

WiFiClient client;
PubSubClient mqttClient(client);
DynamicJsonDocument measurementDocument(256);

void publishData(String sensor, float temp, float humidity, int batt) {
  String topic = MQTT_TOPIC+String("/")+sensor;
  char buffer[256];
  measurementDocument["temperature"] = temp;
  measurementDocument["humidity"] = humidity;
  measurementDocument["battery"] = batt;
  serializeJson(measurementDocument, buffer);
  mqttClient.publish(topic.c_str(), buffer);
}


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      //      Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
      //    //Serial.printf("getServiceDataUUID() %s\n",  advertisedDevice.getServiceDataUUID());
      //    if ( advertisedDevice.haveServiceUUID()) {
      //      Serial.println("has service UUID");
      //    }
      //    if ( advertisedDevice.haveName()) {
      //      Serial.println("has Name");
      //    }
      
      if ( advertisedDevice.haveServiceData()) {

        char buff[20];
        int datalen;

        //Serial.printf(">>>> ServiceDataUUID 0x");
        datalen = (*advertisedDevice.getServiceDataUUID().getNative()).len;
        memcpy(buff, &(*advertisedDevice.getServiceDataUUID().getNative()).uuid, datalen);
        //Serial.printf("UUID Len  %d \n", (*advertisedDevice.getServiceDataUUID().getNative()).len);
        //Serial.printf("Service Data UUID length %d\n", datalen);
        /*
        for (int i = 0; i < datalen; i++) {
          Serial.printf("%0x", buff[i]);
        }
        */
        //Serial.println();
        char tempUUID[] = { 0x1a, 0x18, 0x0} ;

        if ( strncmp( buff, tempUUID, datalen) == 0 ) {
          Serial.printf("Found device named: %s\n", advertisedDevice.getAddress().toString().c_str());
          Serial.println("Found a ServiceDataUUID for Temperature");
          Serial.printf(advertisedDevice.getAddress().toString().c_str());
          datalen = advertisedDevice.getServiceData().length();
          Serial.println(advertisedDevice.getName().c_str());
          memcpy(buff, advertisedDevice.getServiceData().c_str(), datalen);
          Serial.printf(">>>>  Serrvice Data ");
          for (int i = 0; i < datalen; i++) {
            //Serial.printf("%02.2x  ", buff[i]);
            Serial.printf("%0x ",advertisedDevice.getServiceData().c_str()[i]);
          }
          Serial.println();
          Serial.println(advertisedDevice.getRSSI());
          int16_t temp = buff[6]+buff[7]*255;
          float ftemp = temp/100.0;
          float temp_in_f = (ftemp * 9/5)+32;
          Serial.printf("temp: %f\n", temp_in_f);
          char buffer[12];
          sprintf(buffer, "%.1f", temp_in_f);
          float humidity = (buff[8]+buff[9]*255)/100;
          Serial.printf("humidity: %f\n", humidity);
          uint8_t battery = buff[12];
          Serial.printf("battery percentage: %d\n", battery);
          publishData(advertisedDevice.getAddress().toString().c_str(), temp_in_f, humidity, battery);
        }
        else {
          // Serial.println(" ServiceDataUUID doesn't match");
        }
      }
    }
};





void mqttReconnect() {
  while(!mqttClient.connected()) {
    Serial.println("attempting to connect to mqtt broker");
    if (mqttClient.connect("climateClient", MQTT_USER, MQTT_PASS)) {
      Serial.println("connected to broker");
    }
    else{
      Serial.println("failed to connect");
      delay(500);
    }
  }
}

void setup() {
  Serial.begin(115200);
  M5.begin();
  M5.Lcd.setRotation(3);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(WIFI_SSID, WIFI_PW);
  while(WiFi.status() != WL_CONNECTED) {
    Serial.println("connectioning...");
  }
  IPAddress ip = WiFi.localIP();
  Serial.printf("connected: %s\n", ip.toString().c_str());
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);


  delay(1000);
  mqttReconnect();
  mqttClient.publish("climate/test", "hi there");

  Serial.println("Scanning...");
  //esp_log_level_set("*", ESP_LOG_DEBUG);

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(false); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
}

void loop() {
  mqttReconnect();
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  Serial.print("Devices found: ");
  Serial.print(foundDevices.getCount());
  Serial.println("   Scan done!");
  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
  delay(5000);
}