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
float temp;
char tempChar[8];
char datetime[25];

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);


// Constantes -------------------------------------------
const char*   ntpServer           = "pool.ntp.org";
const long    gmtOffset_sec       = -3 * 60 * 60;   // -3h*60min*60s = -10800s
const int     daylightOffset_sec  = 0;              // Fuso em horário de verão

// Variáveis globais ------------------------------------
time_t        nextNTPSync         = 0;

// Funções auxiliares -----------------------------------
String dateTimeStr(time_t t, int8_t tz = -3) {
  // Formata time_t como "aaaa-mm-dd hh:mm:ss"
  if (t == 0) {
    return "N/D";
  } else {
    t += tz * 3600;                               // Ajusta fuso horário
    struct tm *ptm;
    ptm = gmtime(&t);
    String s;
    s = ptm->tm_year + 1900;
    s += "-";
    if (ptm->tm_mon < 9) {
      s += "0";
    }
    s += ptm->tm_mon + 1;
    s += "-";
    if (ptm->tm_mday < 10) {
      s += "0";
    }
    s += ptm->tm_mday;
    s += " ";
    if (ptm->tm_hour < 10) {
      s += "0";
    }
    s += ptm->tm_hour;
    s += ":";
    if (ptm->tm_min < 10) {
      s += "0";
    }
    s += ptm->tm_min;
    s += ":";
    if (ptm->tm_sec < 10) {
      s += "0";
    }
    s += ptm->tm_sec;
    return s;
  }
}
String timeStatus() {
  // Obtém o status da sinronização
  if (nextNTPSync == 0) {
    return "não definida";
  } else if (time(NULL) < nextNTPSync) {
    return "atualizada";
  } else {
    return "atualização pendente";
  }
}

// Callback de sincronização
#ifdef ESP8266
  // ESP8266
  void ntpSync_cb() {
#else
  // ESP32
  void ntpSync_cb(struct timeval *tv) {
#endif
  time_t t;
  t = time(NULL);
  // Data/Hora da próxima atualização
  nextNTPSync = t + (SNTP_UPDATE_DELAY / 1000) + 60;

  Serial.println("Sincronizou com NTP em " + dateTimeStr(t));
  Serial.println("Limite para próxima sincronização é " +
                  dateTimeStr(nextNTPSync));
}

void connectWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  connectAWS();
}

void connectAWS(){
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
    sprintf(tempChar, "%.2f", temp);
    Serial.println(tempChar);
    
}

void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["Temperature"] = tempChar;
  doc["Data Hora"] = dateTimeStr(time(NULL));
  Serial.println("Publishing: " + dateTimeStr(time(NULL)));
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

  // Inicializa Serial e define callback
  #ifdef ESP8266
    // ESP8266
    Serial.begin(9600);
    settimeofday_cb(ntpSync_cb);
  #else
    // ESP32
    Serial.begin(9600);
    sntp_set_time_sync_notification_cb(ntpSync_cb);
  #endif

  // Intervalo de sincronização - definido pela bibioteca lwIP
  Serial.printf("\n\nNTP sincroniza a cada %d segundos\n",
                SNTP_UPDATE_DELAY / 1000);

  // Função para inicializar o cliente NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  Wire.begin(SDA,SCL);
  connectWifi();
}

void loop() {
  while(timeStatus() == "não definida")
  {
    dateTimeStr(time(NULL)) + "\tStatus: " + timeStatus();
  }
  
  temperature();
  dateTimeStr(time(NULL));
  publishMessage();
  client.loop();

  if (WiFi.status() != WL_CONNECTED){
    Serial.println("WIFI desconectado!");
    connectWifi();
  }


  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    connectAWS();
  }
  delay(5000);
}
