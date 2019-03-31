#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <ESP8266WiFi.h>
#include <WiFiUDP.h>

WiFiUDP Client;

int green = D6;
int red = D7;

//initialize sensor variable with required integration time and gain
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_154MS, TCS34725_GAIN_1X);
uint16_t r, g, b, c;

uint16_t values[5];    //variable for maintaining values for sliding-window average calculation
int t = 0;              //variable used for counting the seconds, to get the count of values for sliding window

char* buff = "";

const char* ssid     = "UCInet Mobile Access";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "";     // The password of the Wi-Fi network

const char* server_ip = "169.234.34.161";     //IP address of the Raspberry Pi
unsigned int localUdpPort = 50000;  // local port to listen on

int sec = 0;        //variable for counting the seconds, to decide when to send data to the server 

void setup()
{
  Serial.begin(115200);
  WiFi.begin(ssid,password);
  while(WiFi.status() != WL_CONNECTED) 
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Client.begin(localUdpPort);

  tcs.setInterrupt(true);   //turn off the LED on the sensor
  
  if (tcs.begin()) {
    Serial.println("Found sensor");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1);
  }

  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);

  digitalWrite(red, LOW);
  digitalWrite(green, LOW);
}

uint16_t average_lux()
{
  tcs.getRawData(&r, &g, &b, &c);
  uint16_t sum = 0; 
  t++;
  values[(t-1)%5] = c;

  if (t >= 5) {
     for (int i = 0; i < 5; i++) {
        sum = sum + values[i];
      }
  }

  return sum/5;
}

void send_data_to_server(uint16_t data)
{
  Client.beginPacket(server_ip,localUdpPort);
  String d = String(data);
  d.toCharArray(buff, d.length() + 1);
  byte ack = Client.write(buff, d.length() + 1);
  Client.endPacket();
}



void receive_data_from_server(uint16_t data)
{
 int level = 0;
 bool read_again = true;
 while (read_again)
 {
  level++;
  int rec_size = Client.parsePacket();
  uint16_t server_avg = 0;
  if (rec_size > 0)
  {
    Client.read(buff, UDP_TX_PACKET_MAX_SIZE);
    String val = buff;
    server_avg = (uint16_t) val.toInt();
    Serial.println("Remote value: ");
    Serial.println(server_avg);
    if (data > server_avg)
    {
      digitalWrite(green, HIGH);
      digitalWrite(red, LOW);
      
    }
    else if (server_avg > data)
    {
      digitalWrite(green, LOW);
      digitalWrite(red, HIGH);
    }
    read_again = false;
  }
  else
  {
   Serial.println("No data from receiver....try again");
   read_again = true;
   if (level > 10)
   {
    digitalWrite(green, HIGH);
    digitalWrite(red, HIGH);
    read_again = false;
   }
   continue;
  }
 }
}

void loop()
{
  uint16_t avg = average_lux();
  sec=t;
  if (t > 5 && sec > 0 && sec%2==0)
  {
    send_data_to_server(c);
    Serial.println("c value: ");
    Serial.println(c);
    Serial.println("average: ");
    Serial.println(avg);
    receive_data_from_server(c);
  }
  
  delay(1000);
}
