#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"

#include "DHT.h"

#define DHTPIN 5 // what pin we're connected to
#define DHTTYPE DHT22 // DHT 22 (AM2302)

// WiFi credentials
const char* wifi_ssid = "MEO-714430";
const char* wifi_password = "9d067d8f5a";

// MQTT broker details
const char* mqtt_server = "192.168.1.172";
const int mqtt_port = 1883;
const char* mqtt_username = "user";
const char* mqtt_password = "pass";


DHT dht(DHTPIN, DHTTYPE);   // Initialize DHT sensor for normal 16mhz Arduino
SSD1306Wire display(0x3c, D6, D5);   // ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino.h e.g. https://github.com/esp8266/Arduino/blob/master/variants/nodemcu/pins_arduino.h

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg_temperature[MSG_BUFFER_SIZE];
char msg_humidity[MSG_BUFFER_SIZE];
char display_msg[MSG_BUFFER_SIZE] = "DomoNode: BedRoom v1.0";
int value = 0;

void setup_wifi()
{

  delay(10);  
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  //if(topic == "domonodes/node1/display")
  {    
    int i = 0;
    for (i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
      display_msg[i] = (char)payload[i];
    }
    display_msg[i] = '\0';

    Serial.println(display_msg);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "DomoNode-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), "user","pass")) {
      Serial.println("connected");
      
      client.subscribe("domonodes/node1/display");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void show_display(String DisplayLines[4])
{
    display.clear();
  
    display.setColor(WHITE);
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 0, (char*)DisplayLines[0].c_str());

    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 15, (char*)DisplayLines[1].c_str());
    display.drawString(0, 32, (char*)DisplayLines[2].c_str());
    
    display.setFont(ArialMT_Plain_10);    
    display.drawString(0, 52, (char*)DisplayLines[3].c_str());

    display.display();
}

void setup() 
{
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output  
  Serial.begin(115200);

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  String DisplayLines[4] = {"DomoNode: BedRoom V1.0", "Starting...", "", ""};
  show_display(DisplayLines);

  dht.begin();

  setup_wifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);  
}

void loop() 
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 10000) 
  {
    lastMsg = now;
    
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float hum = dht.readHumidity();    
    float temp = dht.readTemperature();  // Read temperature as Celsius

    // Check if any reads failed and exit early (to try again).
    if (isnan(hum) || isnan(temp)) 
    {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    snprintf (msg_temperature, MSG_BUFFER_SIZE, "%.2f", temp);
    Serial.print("Temp: ");
    Serial.println(temp);
    client.publish("domonodes/node1/dht/temp", msg_temperature);
        
    snprintf (msg_humidity, MSG_BUFFER_SIZE, "%.2f", hum);
    Serial.print("Humidity: ");
    Serial.println(hum);
    client.publish("domonodes/node1/dht/hum", msg_humidity);

    String DisplayLines[4] = {String(display_msg), "Temp: " + String(msg_temperature), "Hum: " + String(msg_humidity), WiFi.localIP().toString()};
    show_display(DisplayLines);
  }
}
