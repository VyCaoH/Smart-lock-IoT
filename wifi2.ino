#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <string.h>
#include <PubSubClient.h>
#include "ThingSpeak.h" // always include thingspeak header file after other header files and custom macros

const char* host = "maker.ifttt.com";
//SoftwareSerial mySerial(D2, D1); // RX, TX
const char* ssid = "Galaxy A51 068F";
const char* password = "uvas3000";

const int address = 0; // Địa chỉ trong EEPROM

const long utcOffsetInSeconds = 7 * 3600; // Điều chỉnh múi giờ (7 giờ)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

const char* mqttServer = "broker.hivemq.com"; 
int port = 1883;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
String tmp;

unsigned long myChannelNumber = 2243219;
const char * myWriteAPIKey = "8Y90PLCPMQNB1AOK";

String door_opened = "Opened!";
String door_closed = "Closed";
String buzzer_on = "ON";
String buzzer_off = "OFF";
String fail = "Failed 3 times";
String motion_detected = "Motion_detect";
String not_detected = "Motion_not_detect";


void sendDataToIFTTT_motion(String eventData) {
  WiFiClient client;
  const char* server = "maker.ifttt.com";
  String url = "/trigger/Motion_detect/with/key/920Pewjq49gk5MBb37BdS7ywZe1p2wS_IILUM3ci50";
  if (client.connect(server, 80)) {
    String request = "GET " + url + "?value1=" + eventData + " HTTP/1.1\r\n" +
                     "Host: " + server + "\r\n" +
                     "Connection: close\r\n\r\n";
    client.print(request);
    Serial.println("Data sent to IFTTT");
  }
  client.stop();
}

void sendDataToIFTTT_fail(String eventData) {
  WiFiClient client;
  const char* server = "maker.ifttt.com";
  String url = "/trigger/Fail/with/key/920Pewjq49gk5MBb37BdS7ywZe1p2wS_IILUM3ci50";
  if (client.connect(server, 80)) {
    String request = "GET " + url + "?value1=" + eventData + " HTTP/1.1\r\n" +
                     "Host: " + server + "\r\n" +
                     "Connection: close\r\n\r\n";
    client.print(request);
    Serial.println("Data Fail sent to IFTTT");
  }
  client.stop();
}




void setup() {
  Serial.begin(9600);
  //mySerial.begin(9600);
  delay(100);


  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }

  timeClient.begin(); // Khởi động NTPClient

  mqttClient.setServer(mqttServer, port);
  mqttClient.setCallback(callback);
  mqttClient.setKeepAlive( 90 );
  // Set the buffer to handle the returned JSON. NOTE: A buffer overflow of the message buffer will result in your callback not being invoked.
  mqttClient.setBufferSize( 2048 );

  ThingSpeak.begin(wifiClient);  // Initialize ThingSpeak
}

void loop() {
  if(!mqttClient.connected()) {
    mqttConnect();
  }
  mqttClient.loop();
  timeClient.update(); // Cập nhật thời gian từ NTPClient

  //Signal from Deadlock
  if (Serial.available()) {
    //nếu cửa mở thành công
    String receivedMessage = Serial.readStringUntil('\n');
    receivedMessage.trim();
    if (receivedMessage.compareTo(door_opened) == 0) {
      unsigned long currentEpochTime = timeClient.getEpochTime();
      EEPROM.put(address, currentEpochTime); // Lưu thời gian vào EEPROM
      ThingSpeak.setField(1, 1);
      ThingSpeak.setField(2, 1);
      mqttClient.publish("nhom9/door","Open");
      int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
      delay(1000); // Wait 1 second to update the channel again
    }

    //nếu thất bại
    else if (receivedMessage.compareTo(door_closed) == 0)
    {
      mqttClient.publish("nhom9/door","Closed");
      unsigned long currentEpochTime = timeClient.getEpochTime();
      EEPROM.put(address, currentEpochTime); // Lưu thời gian vào EEPROM
      ThingSpeak.setField(1, 0);
      int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
      delay(1000); // Wait 1 second to update the channel again
    }
    //when the buzzer on
    else if (receivedMessage.compareTo(buzzer_on) == 0)
    {
      mqttClient.publish("nhom9/buzzer","ON");
    }
    //when the buzzer off
    else if (receivedMessage.compareTo(buzzer_off) == 0)
    {
      mqttClient.publish("nhom9/buzzer","OFF");
    }
    else if (receivedMessage.compareTo(fail)==0)
    {
      sendDataToIFTTT_fail("Fail");
      ThingSpeak.setField(2, 0);
    }
    else if (receivedMessage.compareTo(motion_detected)==0)
    {
      sendDataToIFTTT_motion(motion_detected); 
      ThingSpeak.setField(3, 1);
      mqttClient.publish("nhom9/sensor","ON");
    }
    else if (receivedMessage.compareTo(not_detected)==0)
    {
      ThingSpeak.setField(3, 0);
      mqttClient.publish("nhom9/sensor","OFF");
    }
  }
}

String formatTime(unsigned long epochTime) {
  time_t time = (time_t)epochTime;
  struct tm *timeinfo;
  timeinfo = gmtime(&time); // Chuyển đổi epoch time thành struct tm
  char timeStr[20];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo); // Định dạng thời gian
  return String(timeStr);
}


void mqttConnect() {
  while(!mqttClient.connected()) {
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if(mqttClient.connect(clientId.c_str())) {

      //***Subscribe all topic you need***
      mqttClient.subscribe("nhom9/door");
      mqttClient.subscribe("nhom9/buzzer");
      mqttClient.subscribe("nhom9/sensor");
    }
    else {
      delay(500); //reconnect the server in 0.5 seconds
    }
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  String strMsg;
  for(int i=0; i<length; i++) {
    strMsg += (char)message[i];
  }
  
  //***Code here to process the received package***
  //truyền dữ liệu từ ESP lên Node-red
  if (strMsg.compareTo("true")==0 && strcmp(topic,"nhom9/door") ==0)
  {
    mqttClient.publish("nhom9/door","Open");
    ThingSpeak.setField(1, 1);
    ThingSpeak.setField(2, 1);
    Serial.println("11");
  }
  if (strMsg.compareTo("false")==0 && strcmp(topic,"nhom9/door")==0)
  {
     mqttClient.publish("nhom9/door","Closed");
     ThingSpeak.setField(1, 0);
     Serial.println("10");

  }
  if (strMsg.compareTo("true")==0 && strcmp(topic,"nhom9/buzzer")==0)
  {
    mqttClient.publish("nhom9/buzzer","ON");
    Serial.println("21");
  }
  if (strMsg.compareTo("false")==0 && strcmp(topic,"nhom9/buzzer")==0)
  {
    mqttClient.publish("nhom9/buzzer","OFF");
    Serial.println("20");
  }
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  delay(1000); // Wait 1 second to update the channel again
 
}
