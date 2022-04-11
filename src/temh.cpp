#include <Arduino.h>
#include <WiFi.h>
#include <aliyun_mqtt.h>
#include <SPI.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_Sensor.h>
#include <SoftwareSerial.h>

#define DHTPIN 23
#define DHTTYPE DHT11
#define SENSOR_PIN 10

#define WIFI_SSID   "29"//连接WIFI
#define WIFI_PASSWD "qinbiao20"

#define PRODUCT_KEY "a1iZqrEUHIN"//阿里云中设备信息
#define DEVICE_NAME "dev_esp32s"
#define DEVICE_SECRET "3ce0d91730d2c6b8b7a094040b3f8141"

#define DEV_VERSION "S-TH-WIFI-v1.0-20190220"//订阅
#define ALINK_BODY_FORMAT         "{\"id\":\"123\",\"version\":\"1.0\",\"method\":\"%s\",\"params\":%s}"
#define ALINK_TOPIC_PROP_POST     "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/event/property/post"
#define ALINK_TOPIC_PROP_POSTRSP  "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/event/property/post_reply"
#define ALINK_TOPIC_PROP_SET      "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/service/property/set"
#define ALINK_METHOD_PROP_POST    "thing.event.property.post"
#define ALINK_TOPIC_DEV_INFO      "/ota/device/inform/" PRODUCT_KEY "/" DEVICE_NAME ""    
#define ALINK_VERSION_FROMA      "{\"id\": 123,\"params\": {\"version\": \"%s\"}}"


unsigned long lastMs = 0;
DHT dht(DHTPIN, DHTTYPE);

unsigned long lastSend;
#define TIMESTAMP "23668"
const int WIFI_RX = 2;
const int WIFI_TX = 3;
SoftwareSerial softSerial(WIFI_RX, WIFI_TX); // RX, TX
WiFiClient   espClient;
PubSubClient mqttClient(espClient);

void connectWiFi()
{
    //连接WiFi
    int status = WiFi.status();
    // attempt to connect to WiFi network
    while (status != WL_CONNECTED)
    {
        Serial.print(F("Attempting to connect to Wifi: "));
        Serial.println(WIFI_SSID);
        // Connect to WPA/WPA2 network
        status = WiFi.begin(WIFI_SSID, WIFI_PASSWD);
        delay(500);
    }
}

void initWiFi(const char *ssid, const char *password)//初始化WiFi
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi does not connect, try again ...");
        delay(500);
    }

    connectWiFi();
    Serial.print(F("Connected to AP: "));
    Serial.println(WiFi.localIP());
}

void checkMqttConnection() //检查连接
{
    
    while (!mqttClient.connected())//
    {
        while (!connectAliyunMQTT(mqttClient, PRODUCT_KEY, DEVICE_NAME, DEVICE_SECRET))
        {
            Serial.println("MQTT connect succeed!");
            //client.subscribe(ALINK_TOPIC_PROP_POSTRSP);
            mqttClient.subscribe(ALINK_TOPIC_PROP_SET);
            
            Serial.println("subscribe done");
            checkMqttConnection();
        }
    }
    
}

void getAndSendTemperatureAndHumidityData() //获取和发送温湿度
{
    // Reading temperature or humidity takes about 250 milliseconds!
    int h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    int t = dht.readTemperature();
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t))
    {
        Serial.println(F("Failed to read sensor data!"));
        return;
    }

    Serial.print(F("Humidity: "));
    Serial.print(h);
    Serial.print(F("%  Temperature: "));
    Serial.print(t);
    Serial.println(F("°C"));

    String temperature = String(t);
    String humidity = String(h);

    // Prepare a JSON payload string
    String payload = "{";
    payload += "\"temperature\":";
    payload += temperature;
    payload += ",";
    payload += "\"humidity\":";
    payload += humidity;
    payload += "}";

    // Send payload as Alink data
    String jsonData = F("{\"id\":\"123\",\"version\":\"1.0\",\"method\":\"thing.event.property.post\",\"params\":");
    jsonData += payload;
    jsonData += F("}");

    char alinkData[128];
    jsonData.toCharArray(alinkData, 128);
    mqttClient.publish(ALINK_TOPIC_PROP_POST, alinkData);
    Serial.println(alinkData);
}

void setup()
{
    Serial.begin(115200);
    initWiFi(WIFI_SSID, WIFI_PASSWD);
    dht.begin();
    mqttPrepare(TIMESTAMP, PRODUCT_KEY, DEVICE_NAME, DEVICE_SECRET);
    lastSend = 0;
}

void loop()
{
    checkMqttConnection();

    if (millis() - lastSend >= 10000)
    { // Update and send only after 1 seconds
        getAndSendTemperatureAndHumidityData();
        lastSend = millis();
    }
    mqttClient.loop();
}