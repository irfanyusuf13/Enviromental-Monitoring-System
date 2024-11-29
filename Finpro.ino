#define BLYNK_TEMPLATE_ID "TMPL6NnIARO7z"
#define BLYNK_TEMPLATE_NAME "TP NO 5"
#define BLYNK_AUTH_TOKEN "-LYV7MHePowiYXDc-wAGfV7EiEf5hJ9R"
#define BLYNK_PRINT Serial

#define mqtt_server "broker.hivemq.com"
#define mqtt_port 8884
const char mqttTopic[] = "IoT/FinalProject";

#include <EEPROM.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>
#include "freertos/semphr.h"
#include "time.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);
SemaphoreHandle_t xSemaphore;

char ssid[] = " ";
char pass[] = " ";

const long gmtOffset_sec = 25200;
const int daylightOffset_sec = 0;
const char *ntpServer = "pool.ntp.org";

float aqi = 0;
float temperature = 0;

const int addrAQI = 0
const int addrTemp = 4;
BlynkTimer timer;




void setup () {
    serial.begin(115200);

    Blnyk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(callback);

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    xSemaphore = xSemaphoreCreateMutex();

    lcd.init();
    lcd.backlight();

    EEPROM.begin(512);

}

void loop() {
    blnyk.run();
    timer.run();

    if (!mqttClient.connected () && !isReconnecting) {
        isRecconnecting = true;
        reconnectMqtt();
    }

    mqttClient.loop();
}


void printLocalTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void mqttTask(void *pvParameters) {
    while (true) {
        if (!mqttClient.connected()) {
            reconnectMqtt();
        }
        mqttClient.loop();
    }
}

void sendDatatoBlnyk() {

}

void readDataFromEEPROM() {

}

void saveDataToEEPROM() {

}

void callback(char *topic, byte *payload, unsigned int length) {

}


void reconnectMqtt() {

}
