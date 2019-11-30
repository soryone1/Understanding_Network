/* setup for wifi */
#include <SPI.h>
#include <WiFiNINA.h>
#include <Wire.h>

/* setup for http request */
#include <ArduinoHttpClient.h>
/* setup for parsing JSON */
#include <Arduino_JSON.h>

/* setup for wifi rtc */
#include <WiFiUdp.h>
#include <RTCZero.h>

/*setup for wifi password */
#include "config.h"
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

int status = WL_IDLE_STATUS;
WiFiClient wifi;

RTCZero rtc;
const int GMT = -5;    // new york time

const char serverAddress[] = "167.71.173.220";
int port = 5000;
HttpClient client = HttpClient(wifi, serverAddress, port);
String route = "/by-location?lat=40.648939&lon=-74.010006";

int rtcSum;
int nextComingTrain;
int timeToGo;

void setup() {
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < "1.0.0") {
    Serial.println("Please upgrade the firmware");
  }

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);

    delay(3000);
    printWifiStatus();
  }

  rtc.begin();
  unsigned long epoch;
  int numberOfTries = 0, maxTries = 6;
  do {
    epoch = WiFi.getTime();
    numberOfTries++;
  }
  while ((epoch == 0) && (numberOfTries < maxTries));

  if (numberOfTries == maxTries) {
    Serial.print("NTP unreachable!!");
    while (1);
  }
  else {
    Serial.print("Epoch received: ");
    Serial.println(epoch);
    rtc.setEpoch(epoch);
    Serial.println();
  }
}

void loop() {
  /* send the get request */
  client.get(route);
  /* read the body of the response */
  String response = client.responseBody();

  /* parse the reponse body twice */
  JSONVar myObject = JSON.parse(response);
  JSONVar objectKeys = myObject.keys();
  JSONVar data = myObject[objectKeys[0]];
  String dataString = JSON.stringify(data[0]);
  JSONVar mySecondObject = JSON.parse(dataString);
  JSONVar dataKeys = mySecondObject.keys();
  JSONVar northTrain = mySecondObject[dataKeys[0]];     // northTrain is an array, contains objects.

  /* put the time value in a new array */
  int trainSum = northTrain.length();
  String northTrainString [trainSum];
  String timeTable [trainSum];
  for (int i = 0; i < trainSum; i++) {
    northTrainString[i] = JSON.stringify(northTrain[i]);
    JSONVar lastObject = JSON.parse(northTrainString[i]);
    JSONVar northTrainKeys = lastObject.keys();
    JSONVar TrainTime = lastObject[northTrainKeys[1]];
    timeTable [i] =  TrainTime;
  }

  /* get the int value of minutes and seconds, convert them to seconds, put them into a new array */
  int ComingTimeSecinds[trainSum];
  for ( int i = 0; i < trainSum; i++) {
    //Serial.println(timeTable [i]);
    String minutes = timeTable [i].substring(14, 16);
    String seconds = timeTable [i].substring(17, 19);
    int SumSeconds = minutes.toInt() * 60 + seconds.toInt();
    ComingTimeSecinds[i] = SumSeconds;
  }

  for (int i = 0; i < trainSum; i++) {
    Serial.println(ComingTimeSecinds[i]);
  }

  /* wifi trc here. */
  Serial.println();
  caculateRTC();
  
  int j = 0;
  while(ComingTimeSecinds[j] <= rtcSum){
    j++;
  }

  nextComingTrain = ComingTimeSecinds[j];
  Serial.print("next train: ");
  Serial.println(nextComingTrain);
  
  Serial.println("Wait 10 seconds");
  Serial.println();
  delay(10000);
}

void printWifiStatus() {
  /* print the SSID of the network you're attached to: */
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  /* print your board's IP address: */
  IPAddress ip = WiFi.localIP();
  Serial.print("Main IP Address: ");
  Serial.println(ip);

  /* print the received signal strength: */
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void caculateRTC()
{
  int rtcMinutes = rtc.getMinutes();
  int rtcSecond = rtc.getSeconds();
  rtcSum = rtcMinutes * 60 + rtcSecond;
  Serial.print("rtc = ");
  Serial.println(rtcSum);
}
