#include "include/secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <Wire.h>

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"
//#define BUZ 2
#define SDA 22
#define SCL 21

byte aux[2];
int temp;
char tempChar[8];

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin("EBT_WIFI", "wifiebt0955");

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.print("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    //noTone(BUZ);
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
  //noTone(BUZ);
}

void temperature() {
  byte error, address;
  Wire.beginTransmission(0x49);
  Wire.write(byte(0x00));
  Wire.endTransmission();
  delay(70);
  Wire.requestFrom(0x49,2); //2bytes
  // Slave may send less than requested
  int i =0;
    while(Wire.available()) {
        char c = Wire.read();    // Receive a byte as character
    aux[i] = c;
        //Serial.println(c,DEC);         // Print the character
    i++;
    }
    temp = (aux[0] << 8 | aux[1]) >> 5;
    temp = temp * 0.125;
    sprintf(tempChar, "%d", temp);
    delay(500);
}

void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["temperature"] = tempChar;
  //doc["sensor_a0"] = random(100);
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void messageHandler(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

//  StaticJsonDocument<200> doc;
//  deserializeJson(doc, payload);
//  const char* message = doc["message"];
}

void setup() {
  //pinMode(LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(9600);
  Wire.begin(SDA,SCL);
  connectAWS();
}

void loop() {
  temperature();
  publishMessage();
  client.loop();
  delay(1000);
}
