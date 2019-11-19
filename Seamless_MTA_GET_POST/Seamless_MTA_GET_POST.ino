
/*
  Seamless MTA

  A stand alone IOT device to tell you when is the best time to leave home to catch the next coming subway.

  Circuit:

  - A button
  _ LEDS
  - Buzzer

  HTTP requests list:  -change your own ip in the beginning of each url-

  1.  http://IPaddress/nexttrain/                  // GET- to get the time of when will the next tain comes
  2.  http://IPaddress/setTimeToStation/value      // GET- to set the time it takes you to your closest subway station, normally a walking distance.
  3.  http://IPaddress/setAdvanceTime/value        // GET- to set the time makes you comfortable before the train arrives at the platform, safty tiem also.
  4.  http://IPaddress/home/                       // GET- go the home page to see the full description and submit the form to do the setup.
  5.  http://IPaddress/showresult/                 // POST- after client hit the "calculate" button, it will show the result to the user.

  Last modified time : 19/11/2019
  by Jasper Wang.

*/

#include <SPI.h>
#include <WiFiNINA.h>
//#include <ArduinoHttpClient.h>

/* include all the config files*/
#include "config.h"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

int status = WL_IDLE_STATUS;

WiFiServer server(80);
WiFiClient client = server.available();

/* globle variables here */
int timeToStation = 5;
int advanceTime = 5;
int trainTable[10];
int nextComingTrain;
int timeToGo;

int buttonStatus;
int ledStatus;
unsigned long Counter;

void setup() {

  Serial.begin(9600);

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
  }

  server.begin();
  printWifiStatus();
}


void loop() {

  client = server.available();

  /* here is a way to prove that the globle variables are changed through client's request. */
  // Serial.println(timeToStation);
  // Serial.println(advanceTime);

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
              client.print("The next train you can catch is coming in ");    // ideally, send a request to mtaapi to get the data and parse it.
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
  Serial.println(nextComingTrain);
  timeToGo = nextComingTrain - timeAll;
  Serial.println(timeToGo);

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

void responseHeader() {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println("Refresh: 20");          // refresh the page automatically every 5 sec
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println(" main info here");
  client.println("</html>");
}

void responsMainPage() {
  /* start the web page */
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html><head></head><body>");
  client.println();
  client.println("<p> Hello client.</p><br>");
  client.println("<p> Tell me how much time it takes you to the station </p>");
  client.println("<p> and how long you wish to arrive at the platform before the train comes. </p><br>");
  client.print("<form action='/showresult/' method=post>");   // action is the url followed by the main ip.
  client.print("<b> TimeToStation </b><br>");
  client.print("<input type='number' name=timeToStation value='3'><br>");
  client.print("<b> AdvanceTime </b><br>");
  client.print("<input type='number' name=advanceTime value='1'><br>");
  client.print("<b> CurrentLocation </b><br>");
  client.print("<input type='text' name=currentLocation value='45th'><br>");
  client.print("<input type=submit value=set></form>");
  client.println("</body>");
  client.println("</html>");
  client.stop();
}
