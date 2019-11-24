
/*
  Seamless MTA

  A stand alone IOT device to tell you when is the best time to leave home to catch the next coming subway.

  Circuit:

  - 3 buttons
    - Blue : make a HTTP call to the mta server, to pull the coming train table，only show the first one, press again get the second one.
    - Yellow : after the user customize their settings, press it to add advanceTime 1 min each time, to make the thing more flexible.
    - Green : start calcultion and start the counter.
    
  - 2 LEDS
    - Yellow : show the wifi state, blink 5 times when the device is connecting the wifi; stay on when the connection is solid, off if wifi disconnected.
    - Green : show the connection between device and the mta server.
    
  - Buzzer : buzz for 10s when it's time to go.

  HTTP requests list:  -change your own ip in the beginning of each url-

  1.  http://IPaddress/nexttrain/                  // GET- to get the time of when will the next tain comes
  2.  http://IPaddress/setTimeToStation/value      // GET- to set the time it takes you to your closest subway station, normally a walking distance.
  3.  http://IPaddress/setAdvanceTime/value        // GET- to set the time makes you comfortable before the train arrives at the platform, safty tiem also.
  4.  http://IPaddress/home/                       // GET- go the home page to see the full description and submit the form to do the setup.
  5.  http://IPaddress/showresult/                 // POST- after client hit the "calculate" button, it will show the result to the user.

  Last modified time : 24/11/2019
  by Jasper & Ashley

*/

/* define the pins */
#define wifiStateLEDPin 5         // the yellow led
#define serverStateLEDPin 6       // the green one
#define startButtonPin A4         // the green button
#define addTimeButtonPin A6       // the yellow button
#define buzzerPin 10

/* include all the libraries here */
#include <SPI.h>
#include <WiFiNINA.h>
//#include <ArduinoHttpClient.h>

/* include all the config files*/
#include "config.h"
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

/* setup wifi variables */
int status = WL_IDLE_STATUS;
WiFiServer server(80);
WiFiClient client = server.available();

/* users' setting variables */
int timeToStation = 0;
int advanceTime = 0;
int trainTable[10];
int nextComingTrain;
int timeToGo;

/* state lEDs, Buttons, Buzzer */
int greenButtonState = LOW;
int yellowButtonState = LOW;
int lastGreenButtonState = LOW;
int lastYellowButtonState = LOW;
bool startCalculation = false;
unsigned long lastGreenButtonPressedTime = 0;
unsigned long lastYellowButtonPressedTime = 0;
int debounceTime = 50;
bool greenLEDState = false;
bool yellowLEDState = false;

/* checktime for the counter */
bool startCount = false;
unsigned long lastCheckTime;

/* flag to check if the user already done the setting via http request */
bool userSet = false;

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
  pinMode(buzzerPin, OUTPUT);

  noTone(buzzerPin);

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
    blinkLED();
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);

    delay(3000);
  }

  server.begin();
  printWifiStatus();
}


void loop() {
  /* set the wifi LEd state here */
  setLED();

  /* button detection here */
  handleButtonPress();

  /* alert watching */
  if (alert) {
    alertStart();
    if (millis() - lastAlertTime >= 10 * 1000) {
      alert = false;
    }
  }

  /* here is a way to prove that the globle variables are changed through client's request. */
  // Serial.println(timeToStation);
  // Serial.println(advanceTime);
  // Serial.println(timeToGo);
  
  /* calcuation here */
  if (startCalculation) {
    calculation();
    startCalculation = false;                 // only do the calculation once each time
    startCount = true;
  }

  /* counter here */
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

  /* server loop here */
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
              responsMainPage();                                       // show the web page to the client.
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

        /* If the request mehod is POST, handle it here. */

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
            }

            if (lastToken.endsWith("advanceTime")) {

              advanceTime = currentToken.toInt();
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

void responsMainPage() {
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

void blinkLED() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(wifiStateLEDPin, HIGH);
    delay(200);
    digitalWrite(wifiStateLEDPin, LOW);
    delay(200);
  }
}

void setLED() {
  if (status == WL_CONNECTED) {
    digitalWrite(wifiStateLEDPin, HIGH);
  } else {
    digitalWrite(wifiStateLEDPin, LOW);
  }
}

void handleButtonPress() {
  /* button detection here */
  int sensorValueGreen = digitalRead(startButtonPin);
  int sensorValueYellow = digitalRead(addTimeButtonPin);

  if (sensorValueGreen != lastGreenButtonState) {
    lastGreenButtonPressedTime = millis();
  }

  if (sensorValueYellow != lastYellowButtonState) {
    lastYellowButtonPressedTime = millis();
  }

  if (millis() - lastGreenButtonPressedTime >= debounceTime) {
    if (sensorValueGreen != greenButtonState) {
      greenButtonState = sensorValueGreen;
      if (greenButtonState == HIGH) {                                               // green button here.
        if (userSet) {
          Serial.println("start Calculation!");
          startCalculation = true;
        }
      }
    }
  }

  if (millis() - lastYellowButtonPressedTime >= debounceTime) {
    if (sensorValueYellow != yellowButtonState) {
      yellowButtonState = sensorValueYellow;
      if (yellowButtonState == HIGH) {                                               // yellow button here.
        if (userSet) {
          Serial.println("Add 1 minute");
          advanceTime += 1;
          Serial.print("advanceTime: ");
          Serial.println(advanceTime);
        }
      }
    }
  }

  lastGreenButtonState = sensorValueGreen;
  lastYellowButtonState = sensorValueYellow;
}

void alertStart() {
  if (millis() - lastBuzzetime >= 2000) {
    tone(buzzerPin, 1000, 400);
    lastBuzzetime = millis();
  }
}
