
/*
  Seamless MTA

  A stand alone IOT device to tell you when is the best time to leave home to catch the next coming subway.

  mta real time data server : http://167.71.173.220:5000/by-location?lat=40.648939&lon=-74.010006

  Circuit:

  - 3 buttons
    - Yellow : after the user customize their settings, press it to add advanceTime 1 min each time, to make the thing more flexible. the advance time will be reset each cycle.
    - Blue : make a HTTP call to the mta time server, to pull the coming train table，based on user's setting, do the caculation, and return the one which user is able to catch up.
    - Green : start the counter.

  - 2 LEDS
    - Yellow : show the wifi state, stay on when the connection is solid, off if wifi disconnected.
    - Green : stay on when the device get the mta data from server, turn off after the buzzer off.

  - Buzzer : buzz for 10s when it's time to go.

  HTTP requests list:                               **** change your own ip in the beginning of each url ****

  1.  http://IPaddress/nexttrain/                  // GET- to get the time of when will the next tain comes
  2.  http://IPaddress/setTimeToStation/value      // GET- to set the time it takes you to your closest subway station, normally a walking distance.
  3.  http://IPaddress/setAdvanceTime/value        // GET- to set the time makes you comfortable before the train arrives at the platform, safty tiem also.
  4.  http://IPaddress/home/                       // GET- go the home page to see the full description and submit the form to do the setup.
  
  5.  http://IPaddress/showresult/                 // POST- after client hit the "calculate" button, it will show the result to the user.

  Last modified time : 30/11/2019
  by Jasper & Ashley

  Bugs: time caculate algorithm is not working, when time is at hourly shift. need to improve.
  
*/

/* define the pins */
#define wifiStateLEDPin 6         // the yellow led
#define serverStateLEDPin 7       // the green one
#define startButtonPin A1         // the green button
#define addTimeButtonPin A6       // the yellow button
#define pullMtaButtonPin A3       // the blue button
#define buzzerPin 2
#define SCREEN_WIDTH 128          // OLED display width, in pixels
#define SCREEN_HEIGHT 32          // OLED display height, in pixels
#define OLED_RESET     -1         // Reset pin # (or -1 if sharing Arduino reset pin)

/* include all the libraries here */
#include <SPI.h>
#include <WiFiNINA.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoHttpClient.h>
#include <Arduino_JSON.h>
#include <WiFiUdp.h>
#include <RTCZero.h>

/* setup the oled display */
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/* include all the config files*/
#include "config.h"
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

/* setup wifi variables */
int status = WL_IDLE_STATUS;
WiFiClient wifi;
RTCZero rtc;
const int GMT = -5;                                     /* new york time zone */

/* setup the device as a server */
WiFiServer server(80);
WiFiClient client = server.available();

/* setup client variables, include the remote server's ip address, port, and the whole uri*/
const char serverAddress[] = "167.71.173.220";
int port = 5000;
HttpClient mkrClient = HttpClient(wifi, serverAddress, port);
String route = "/by-location?lat=40.648939&lon=-74.010006";

/* users' setting variables */
int timeToStation = 0;
int advanceTime = 0;
int advanceTimeBuffer = 0;
int timeToGo;
int nextComingTrain;

/* wifi rtc variables */
int rtcSum;
int rtcHour;
int rtcMinutes;
int rtcSeconds;

int trainTable[10];              /* for fake data only */

/* state lEDs, Buttons, Buzzer */
int greenButtonState = LOW;
int yellowButtonState = LOW;
int blueButtonState = LOW;
int lastGreenButtonState = LOW;
int lastYellowButtonState = LOW;
int lastBlueButtonState = LOW;
bool startCalculation = false;
unsigned long lastGreenButtonPressedTime = 0;
unsigned long lastYellowButtonPressedTime = 0;
unsigned long lastBlueButtonPressedTime = 0;
int debounceTime = 50;
bool greenLEDState = false;
bool yellowLEDState = false;

/* checktime for the counter */
bool startCount = false;
unsigned long lastCheckTime;

/* flag to check if the user already done the setting via http request */
bool userSet = false;

/* flag to check if it is the time to request the mta data from the digitalocean server */
bool sendRequest_and_caculateNextTrain = false;

/* flag for buzzer */
bool alert = false;
unsigned long lastBuzzetime = 0;
unsigned long lastAlertTime = 0;

void setup() {

  Serial.begin(9600);

  pinMode(wifiStateLEDPin, OUTPUT);
  pinMode(serverStateLEDPin, OUTPUT);
  pinMode(startButtonPin, INPUT);
  pinMode(addTimeButtonPin, INPUT);
  pinMode(pullMtaButtonPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  noTone(buzzerPin);

  /*--------------------------------------------  setup oled  ------------------------------------------------*/
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {       // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);                                             // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("WELCOME TO "));
  display.setCursor(0, 15);
  display.setTextSize(1);
  display.println(F("SEAMLESS MTA"));
  display.display();
  delay(5000);                                            // Pause for 5 seconds
  display.clearDisplay();

  /* ------------------------------------------  open the serial moniter to start the program here  ------------------------------------*/
  
  while (!Serial) {                                                                  /* open the serial, then start all, for test only */
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
    //blinkLED();
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);

    delay(3000);
    printWifiStatus();
  }

  server.begin();

  /* ------------------------  wifi_rtc setup here  -----------------------*/
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

  /* display the text */
  textDisplay();

  /* set the wifi LED state here */
  setLED();

  /* button detection here */
  handleButtonPress();

  /* alert watching */
  if (alert) {
    alertStart();
    if (millis() - lastAlertTime >= 10 * 1000) {
      alert = false;
      greenLEDState = false;
      advanceTime = advanceTimeBuffer;
    }
  }

  /* here is a way to prove that the globle variables are changed through client's request. */
  // Serial.println(timeToStation);
  // Serial.println(advanceTime);
  // Serial.println(timeToGo);

  /* fake calcuation here */
  //  if (startCalculation) {
  //    calculation();
  //    startCalculation = false;                 // only do the calculation once each time
  //    startCount = true;
  //  }

  if (sendRequest_and_caculateNextTrain) {
    /* get the real time */
    caculateRTC();
    requestTrainTime_and_caculate();
    sendRequest_and_caculateNextTrain = false;     // only do the calculation once each time
  }

  /* ----------------------------------------------  counter here -----------------------------------------------------*/
  
  if (startCount) {
    if ( timeToGo == 0) {
      alert = true;
      lastAlertTime = millis();
      Serial.println("time to go");
      startCount = false;
    } else {
      if (millis() - lastCheckTime >= 1000 * 6 ) {
        timeToGo --;
        lastCheckTime = millis();
        Serial.println(timeToGo);
      }
    }
  }

  /* ------------------------------------------------------- server loop here -------------------------------------------------------*/
  
  client = server.available();
  if (client.available()) {
    Serial.println("new client");

    while (client.connected()) {
      if (client) {
        String userRequest = client.readStringUntil(' ');             // get the request method, GET or POST
        Serial.println(userRequest);

        /* If the request mehod is GET, handle it here. */

        if (userRequest == "GET" ) {
          String lastToken = "";

          while (!lastToken.endsWith("HTTP")) {
            String currentToken = client.readStringUntil('/');


            if (lastToken == "nexttrain") {                            // if the client is asking for the next arrival time only through GET
              calculation();
              client.print("The next train you can catch up is coming in ");    // ideally, send a request to mtaapi to get the data and parse it.
              client.print(nextComingTrain);
              client.println(" mins.");
              client.println();
              client.print("You have ");
              client.print(timeToGo);
              client.println(" mins to leave home.");
              client.println();
              client.println("Start counting now.");
            }

            if (lastToken == "home") {
              Serial.println("Client is at the home page");
              responseMainPage();                                       // show the web page to the client.
            }

            if (lastToken == "currentLocation") {
              Serial.println("currentLocation here");
              client.println("currentLocation will be shown here.");
            }

            if (lastToken == "setTimeToStation") {
              timeToStation = currentToken.toInt();
              Serial.print("SetTimeToStation: ");
              Serial.println(timeToStation);
              client.print("You set the timeToStation to ");
              client.print(timeToStation);
              client.println(" mins successfully!");
            }

            if (lastToken == "setAdvanceTime") {
              advanceTime = currentToken.toInt();
              Serial.print("SetTimeStation: ");
              Serial.println(advanceTime);
              client.print("You set the advanceTime to ");
              client.print(advanceTime);
              client.println(" mins successfully!");
            }

            lastToken = currentToken;
          }

          //          client.println("HTTP 200 OK\n\n");                           // for debug only.
          delay(1);

          if (client.connected()) {
            client.stop();
            Serial.println("client disonnected");
          }
        }

        /* ------------------------------- If the request mehod is POST, handle it here. ---------------------------------------*/

        if (userRequest == "POST" ) {

          String lastToken = "";

          while (!lastToken.endsWith("currentLocation")) {
            String currentToken = client.readStringUntil('=');

            if (lastToken.endsWith("timeToStation")) {

              timeToStation = currentToken.toInt();
              Serial.print("Time To Station: ");
              Serial.println(timeToStation);
              client.print("You set the timeToStation to ");
              client.print(timeToStation);
              client.println(" mins successfully!");
              //responseSetupPage();
            }

            if (lastToken.endsWith("advanceTime")) {

              advanceTime = currentToken.toInt();
              advanceTimeBuffer = currentToken.toInt();
              Serial.print("Advanced Time: ");
              Serial.println(advanceTime);
              client.print("You set the advanceTime to ");
              client.print(advanceTime);
              client.println(" mins successfully!");
            }

            userSet = true;
            lastToken = currentToken;                                    // update the lastToken.
          }

          //client.println("HTTP 200 OK\n\n");                           // for debug only.
          client.println("");
          client.println("Hit the button when you feel ready to go.");

          delay(1);

          if (client.connected()) {                                    // close the connection.
            client.stop();
            Serial.println("client disonnected");
          }

        }
      }
    }

  }
}

/* ----------------------------------------- a fake caculation ----------------------------------------------------*/

void calculation() {              

  for (int i = 0; i < 10; i++) {
    trainTable[i] = random(20);
  }

  int timeAll = timeToStation + advanceTime;

  int j = 0;
  while (trainTable[j] <= timeAll) {
    j++;
  }

  nextComingTrain = trainTable[j];
  timeToGo = nextComingTrain - timeAll;
  Serial.print("You have ");
  Serial.print(timeToGo);
  Serial.println(" mins to go.");
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

void responseMainPage() {
  /* start the web page */
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();

  client.println("<!DOCTYPE HTML>");
  client.println("<html><head><title>");
  client.println("Home");
  client.println("</title></head><body style='background-color:#F8F8F5; margin:0'><h1 style=' background: rgb(114, 51, 197); color:#f5f5f5; font-family:verdana; text-align:center; padding: 30px; margin:0'>");
  client.println("Welcome to Seamless MTA");

  client.print("</h1> <div style='border-radius: 5px; background-color: #f2f2f2; padding: 20px; text-align: center;'> <form action='/showresult/' method=post enctype='application/x-www-form-urlencoded'>");   // action is the url followed by the main ip.
  client.print(" <label>Time To Station</label>");
  client.print("<input style='width: 100%; padding: 12px 20px; margin: 8px 0; box-sizing: border-box; border: 1px solid rgb(114, 51, 197); border-radius: 4px;' type='number' name='timeToStation' size='3' placeholder='The time you walk to the station..'><br><br>");
  client.print("<label>Advance Time</label>");
  client.print("<input style='width: 100%; padding: 12px 20px; margin: 8px 0; box-sizing: border-box; border: 1px solid rgb(114, 51, 197); border-radius: 4px;' type='number' name='advanceTime' size='3'  placeholder='How much time you want to have before the train arrives..'><br><br>");
  client.print("<label>Current Location</label>");
  client.print("<input style='width: 100%; padding: 12px 20px; margin: 8px 0; box-sizing: border-box; border: 1px solid rgb(114, 51, 197); border-radius: 4px;' type='text' name='currentLocation' placeholder='The station you go..'><br><br>");
  client.print("<input style='width: 100%; background-color: rgb(63, 177, 67); font-size: 10pt; color: white; padding: 15px 20px;margin: 8px 0; border: none; border-radius: 4px;cursor: pointer;' type=submit value=Submit></form></div></body></html>");
  client.stop();
}

void responseSetupPage() {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();

  client.println("<!DOCTYPE HTML>");
  client.println("<html><head><title>");
  client.println("Setup Done");
  client.println("</title></head><body style='background-color:#F8F8F5; margin:0'><h1 style=' background: rgb(114, 51, 197); color:#f5f5f5; font-family:verdana; text-align:center; padding: 30px; margin:0'>");
  client.println("Welcome to Seamless MTA");

  client.println("</h1> <p style='background:#f2f2f2; width: 100%; padding: 12px 20px; font-family:verdana;'>");
  client.println("<b>Time To Station</b> =");
  client.println(timeToStation);
  client.println("<br /><br /><b>Time ahead</b> =");
  client.println(advanceTime);
  client.println("<br /><br /><br /><br /> Setup successfully!<br /><br /> Press the <b>Green</b> button when you feel ready to go.<br /><br />Press the <b>Yellow</b> button if you want to add more adead time.</p></body></html");
  client.stop();
}

void blinkLED() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(wifiStateLEDPin, HIGH);
    delay(200);
    digitalWrite(wifiStateLEDPin, LOW);
    delay(200);
  }
}

void setLED() {
  if (status == WL_CONNECTED) {                           /* set the wifi state led */
    digitalWrite(wifiStateLEDPin, HIGH);
  } else {
    digitalWrite(wifiStateLEDPin, LOW);
  }

  if (greenLEDState == true) {                           /* set the server state led */
    digitalWrite(serverStateLEDPin, HIGH);
  } else {
    digitalWrite(serverStateLEDPin, LOW);
  }
}

void handleButtonPress() {                                         /* button detection here */
  int sensorValueGreen = digitalRead(startButtonPin);
  int sensorValueYellow = digitalRead(addTimeButtonPin);
  int sensorValueBlue = digitalRead(pullMtaButtonPin);

  if (sensorValueGreen != lastGreenButtonState) {
    lastGreenButtonPressedTime = millis();
  }

  if (sensorValueYellow != lastYellowButtonState) {
    lastYellowButtonPressedTime = millis();
  }

  if (sensorValueBlue != lastBlueButtonState) {
    lastBlueButtonPressedTime = millis();
  }

  if (millis() - lastGreenButtonPressedTime >= debounceTime) {
    if (sensorValueGreen != greenButtonState) {
      greenButtonState = sensorValueGreen;
      if (greenButtonState == HIGH) {                                               // green button here.
        if (userSet) {
          Serial.println("start counting down!");
          startCount = true;
        }
      }
    }
  }

  if (millis() - lastYellowButtonPressedTime >= debounceTime) {
    if (sensorValueYellow != yellowButtonState) {
      yellowButtonState = sensorValueYellow;
      if (yellowButtonState == HIGH) {                                               // yellow button here.
        if (userSet) {
          Serial.println("Add 1 min");
          advanceTime += 1;
          Serial.print("advanceTime: ");
          Serial.print(advanceTime);
          Serial.println(" mins");
          Serial.println();
        }
      }
    }
  }

  if (millis() - lastBlueButtonPressedTime >= debounceTime) {
    if (sensorValueBlue != blueButtonState) {
      blueButtonState = sensorValueBlue;
      if (blueButtonState == HIGH) {                                                // button button here.
        if (userSet) {
          sendRequest_and_caculateNextTrain = true;
          Serial.println("pulled the mta data.");
        }
      }
    }
  }


  lastGreenButtonState = sensorValueGreen;
  lastYellowButtonState = sensorValueYellow;
  lastBlueButtonState = sensorValueBlue;
}

void alertStart() {
  if (millis() - lastBuzzetime >= 2000) {
    tone(buzzerPin, 1000, 400);
    lastBuzzetime = millis();
  }
}

void textDisplay() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, 0);
  display.println(F("Next train: "));

  display.setCursor(113, 0);
  display.print(nextComingTrain);

  display.setCursor(2, 12);
  display.print(F("Time ahead: "));

  display.setCursor(113, 12);
  display.println(advanceTime);

  display.setCursor(2, 24);
  display.print(F("Time to go: "));

  display.setCursor(113, 24);
  display.println(timeToGo);

  display.display();
}

void requestTrainTime_and_caculate () {
  /* send the get request */
  mkrClient.get(route);
  /* read the body of the response */
  String response = mkrClient.responseBody();

  /* parse the reponse body twice */
  JSONVar myObject = JSON.parse(response);

  if (JSON.typeof(myObject) != "undefined") {
    greenLEDState = true;
  }

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
  int ComingTimeSeconds[trainSum];
  for ( int i = 0; i < trainSum; i++) {
    //Serial.println(timeTable [i]);
    String minutes = timeTable [i].substring(14, 16);
    String seconds = timeTable [i].substring(17, 19);
    int SumSeconds = minutes.toInt() * 60 + seconds.toInt();
    ComingTimeSeconds[i] = SumSeconds;
  }

  Serial.println();

  /* print all the time values here. */
  for (int i = 0; i < trainSum; i++) {
    int nextTrainMin = ComingTimeSeconds[i] / 60;
    Serial.print("No.");
    Serial.print(i + 1);
    Serial.print(" is coming at ");
    Serial.print(":");
    Serial.println(nextTrainMin);
    //    Serial.print(" = ");
    //    Serial.print(ComingTimeSeconds[i]);
    //    Serial.println("s");
  }

  /*get the overall time the user needs to get to the station. */
  int timeAll = timeToStation + advanceTime;           /* user use mins. */
  int timeAllSeconds = timeAll * 60;                   /* convert it to seconds.*/
  Serial.println();
  Serial.print("time to platform: ");
  Serial.print(timeAll);
  Serial.println(" mins.");

  int j = 0;
  while (ComingTimeSeconds[j] <= rtcSum + timeAllSeconds) {
    j++;
  }

  nextComingTrain = ComingTimeSeconds[j];
  int comingTrainMin = nextComingTrain / 60 - rtcMinutes;
  Serial.print("next train you can catch up is coming in: ");
  Serial.print(comingTrainMin);
  Serial.println(" mins.");

  timeToGo = comingTrainMin - timeAll;
  Serial.print("You have ");
  Serial.print(timeToGo);
  Serial.println(" mins to go.");
  Serial.println();
}

void caculateRTC() {
  rtcMinutes = rtc.getMinutes();
  rtcSeconds = rtc.getSeconds();
  rtcHour = rtc.getHours() + GMT;
  rtcSum = rtcMinutes * 60 + rtcSeconds;
  //  Serial.print("rtc value = ");
  //  Serial.println(rtcSum);
  //  Serial.println();
}
