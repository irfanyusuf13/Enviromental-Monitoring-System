// Blynk Config
#define BLYNK_TEMPLATE_ID "TMPL62d80911l"
#define BLYNK_TEMPLATE_NAME "Finpro"
#define BLYNK_AUTH_TOKEN "gU2c7jLT3jR49avcPaK64yCGOw8_taNK"
#define BLYNK_PRINT Serial

// MQTT Config
#define mqtt_server "broker.hivemq.com"
#define mqtt_port 1883
#define mqtt_topic "env-monitor/output"

#include <EEPROM.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>
#include "freertos/semphr.h"
#include "time.h"
#include <DHT.h>  
#include <ESP32Servo.h>

#define DHT_PIN 15          
#define DHT_TYPE DHT11      

DHT dht(DHT_PIN, DHT_TYPE);

#define ledPin 5
#define MQ135_PIN 34
#define pinServo  4

// Initialize LCD and mutex
LiquidCrystal_I2C lcd(0x27, 20, 4);
SemaphoreHandle_t xMutex;

// WiFi credentials
const char ssid[] = "Kazoku_EXT";
const char password[] = "nept2908";
uint32_t lastPublish = 0;

// NTP server configuration
const long  gmtOffset_sec = 25200;
const int   daylightOffset_sec = 0;
const char* ntpServer = "asia.pool.ntp.org";

// Environmental variables
float gasLevel = 0.0; 
float temperature = 0.0; 
float humidity = 0.0; 

// EEPROM addresses
const int addrGas = 0;
const int addrTemperature = 4;
const int addrHumidity = 8;

// Blynk timer
BlynkTimer timer;

// Servo
Servo myservo;

// WiFi and MQTT clients
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
bool isReconnecting = false;

void sendEnvDataToBlynk();
void readEnvDataFromEEPROM();
void saveEnvDataToEEPROM();

void setup()
{
  // Start serial communication
  Serial.begin(115200);

  // Initialize Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);

  // Set up MQTT server and callback
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(callback);

  // Initialize time synchronization
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Create mutex
  xMutex = xSemaphoreCreateMutex();

  
  // Initialize LCD
  lcd.init();
  lcd.backlight();

  // Initialize EEPROM
  EEPROM.begin(32);

  // Initialize DHT sensor
  dht.begin();

  // Read environmental data from EEPROM
  readEnvDataFromEEPROM();

  // Set up Blynk timer interval
  timer.setInterval(5000L, sendEnvDataToBlynk);

  // Delay 
  delay(1000);

  // Set up LED pin
  pinMode(ledPin, OUTPUT);

  // Initialize MQ135 sensor pin
  pinMode(MQ135_PIN, INPUT);

  // Initialize Servo
  myservo.attach(pinServo); 
}

void loop()
{
  // Run Blynk and timer
  Blynk.run();
  timer.run();

  // Create and run MQTT task on a separate core
  xTaskCreatePinnedToCore(
    mqttTask,         
    "MQTT_Task",        
    8192,              
    NULL,               
    1,                
    NULL,           
    1                   
  );

  // Reconnect to MQTT if necessary
  if (!mqttClient.connected() && !isReconnecting) {
    isReconnecting = true;
    reconnect();
  }
  
  mqttClient.loop();

  // Publish message to MQTT Topic every 50 seconds
  if (millis() - lastPublish >= 50000 && mqttClient.connected()) {
    lastPublish = millis();
    mqttClient.publish(mqtt_topic, "Environment monitored.");
    printLocalTime();
    Serial.println("\tPublished message to MQTT Topic");
  }
}

// Print local time
void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.print(&timeinfo, "%H:%M:%S");
}

// MQTT task
void mqttTask(void *pvParameters) {
  (void)pvParameters;

  while (true) {
    // Handle MQTT events
    if (!mqttClient.connected() && !isReconnecting) {
      isReconnecting = true;
      reconnect();
    }
    mqttClient.loop();
    
    // Sleep for a short duration to allow other tasks to run
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// Send environmental data to Blynk
void sendEnvDataToBlynk()
{
  if(xSemaphoreTake(xMutex, portMAX_DELAY)) {
    printLocalTime();
    Serial.println("\tMutex taken");

    // Read data from DHT22 sensor
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

    gasLevel = analogRead(MQ135_PIN);
    gasLevel = map(gasLevel, 0, 4095, 0, 500); 

    // Check if the sensor readings are valid
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Clamp AQI and temperature to valid ranges
    gasLevel = max(0.0f, min(gasLevel, 100.0f));  // Same for temperature
    temperature = max(-10.0f, min(temperature, 50.0f));  // Same for temperature
    humidity = max(0.0f, min(humidity, 100.0f));  // Clamp humidity

    printLocalTime();
    Serial.printf("\tGas: %.2f\tTemperature: %.2f°C\tHumidity: %.2f%%\n", gasLevel, temperature, humidity);

    saveEnvDataToEEPROM();

    // Write to Blynk
    Blynk.virtualWrite(V0, gasLevel);
    Blynk.virtualWrite(V1, temperature);
    Blynk.virtualWrite(V2, humidity);

    // Write to LCD Display
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Gas: ");
    lcd.print(gasLevel, 2);
    lcd.print(" ppm");

    lcd.setCursor(0, 1);
    lcd.print("Temp: ");
    lcd.print(temperature, 2);
    lcd.print(" C");

    lcd.setCursor(0, 2);
    lcd.print("Humidity: ");
    lcd.print(humidity, 2);
    lcd.print(" %");

    
    // Not Good Condition

    if (gasLevel > 50) {
      Blynk.logEvent("gas_alert", "Gas Leakage Detected");
      digitalWrite(ledPin, HIGH);  /
      printLocalTime();
      Serial.println("\tALERT! AIR QUALITY IS POOR - Servo activated");
    } 
    else {
      digitalWrite(ledPin, LOW);  /
      printLocalTime();
      Serial.println("\tAir quality is GOOD - Servo deactivated");
    }

    if (temperature > 40) {
      Blynk.logEvent("temperature_alert", "Overheat Detected");
      myservo.write(90); 
      printLocalTime();
      Serial.println("\tALERT! Temperature is too HIGH!");
      delay(500); 
      myservo.write(0);
      delay(500);
    } 
    else {
      myservo.write(0); 
      printLocalTime();
      Serial.println("\tTemperature is within normal range.");
    }

    

    xSemaphoreGive(xMutex);
    printLocalTime();
    Serial.println("\tMutex given");
    Serial.println("---------------------------------");
  }
}

// Read environmental data from EEPROM
void readEnvDataFromEEPROM()
{
  EEPROM.get(addrGas, gasLevel);
  EEPROM.get(addrTemperature, temperature);
  EEPROM.get(addrHumidity, humidity);
}

// Save environmental data to EEPROM
void saveEnvDataToEEPROM()
{
  EEPROM.put(addrGas, gasLevel);
  EEPROM.put(addrTemperature, temperature);
  EEPROM.put(addrHumidity, humidity);
  EEPROM.commit();
}

// MQTT callback function
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }
  Serial.println();
}

// Reconnect to MQTT
void reconnect() {
  if (!mqttClient.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (mqttClient.connect("ESP32Client - Env")) {
      Serial.println("MQTT Connected!");
      mqttClient.subscribe(mqtt_topic);
      Serial.println("Subscribed to MQTT Topic");
      isReconnecting = false;
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}
