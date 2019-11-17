
/*
  Seamless MTA

  A stand alone IOT device to tell you when is the best time to leave home to catch the next coming subway.

  Circuit:
  
  - A button
  _ LEDS
  - Buzzer

  HTTP requests list:  -change your own ip in the beginning of each url-
  
  1.  http://192.168.0.16/nexttrain/                  // GET- to get the time of when will the next tain comes
  2.  http://192.168.0.16/setTimeToStation/value      // GET- to set the time it takes you to your closest subway station, normally a walking distance.
  3.  http://192.168.0.16/setAdvanceTime/value        // GET- to set the time makes you comfortable before the train arrives at the platform, safty tiem also.
  4.  http://192.168.0.16/home/                       // GET- go the home page to see the full description and submit the form to do the setup.
  5.  http://192.168.0.16/showresult/                 // POST- after client hit the "calculate" button, it will show the result to the user.

  Last modified time : 17/11/2019
  by Jasper Wang.
  
*/

/* setup wifi connections*/
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
int timeToStation = 0;
int advanceTime = 0;
int buttonStatus;
int ledStatus;
unsigned long alarmCounter;

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

  /* attempt to connect to Wifi network: */
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);

    delay(5000);
  }

  /* When connected, start the server */
  server.begin();
  /* you're connected now, so print out the status: */
  printWifiStatus();
}


void loop() {

  /* listen for incoming clients */
  client = server.available();

  /* here is a way to prove that the globle variables are changed through client's request. */
  // Serial.println(timeToStation);
  // Serial.println(advanceTime);

  if (client.available()) {
    Serial.println("new client");

    while (client.connected()) {
      if (client) {
        String userRequest = client.readStringUntil(' ');      // get the request method, GET or POST
        Serial.println(userRequest);

        if (userRequest == "GET" || userRequest == "POST") {
          String lastToken = "";
          while (!lastToken.endsWith("HTTP")) {                        // limit the parse before the HTTP/1.1, chrome will add this to you.
            String currentToken = client.readStringUntil('/');         // remember to put a '/' before the keyword you want to read.

            if (lastToken == "home") {                                 // if the client is requesting the homepage. Call homepage's functions.
              Serial.println("home page");
              responsMainPage();                                       // show a web page to the client.
            }

            if (lastToken == "nexttrain") {                            // if the client is asking for the next arrival time only through GET
              Serial.println("get the next coming train");
              client.println("The next train is coming in 5 mins");    // ideally, send a request to mtaapi to get the data and parse it.
            }

            if (lastToken == "showresult") {                           // after the client submit the form, show the result page to the user.
              Serial.println("show result here");
              client.println("the tesult will be shown here.");        // call the function which doing the calculation.
            }

            if (lastToken == "setTimeToStation") {                     // if client wants to set the timeToStation Value via GET.
              timeToStation = currentToken.toInt();
              Serial.print("SetTimeToStation: ");
              Serial.println(timeToStation);
              client.print("You set your timeToStation to ");
              client.print(timeToStation);
              client.println(" mins successfully!");
            }

            if (lastToken == "setAdvanceTime") {
              advanceTime = currentToken.toInt();
              Serial.print("SetTimeStation: ");
              Serial.println(advanceTime);
              client.print("You set your advanceTime to ");
              client.print(advanceTime);
              client.println(" mins successfully!");
            }

            lastToken = currentToken;                                  // update lastToken.
          }

          client.println("HTTP 200 OK\n\n");                           // for debug only.
          delay(1);

          if (client.connected()) {                                    // close the connection.
            client.stop();
            Serial.println("client disonnected");
          }
        }                                                               // warps the request method.

      }
    }
  }

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
  client.println("<p> and how long you wish to arrive the platform before the train comes. </p><br>");
  client.print("<form action='/showresult/' method=post>");   // action is the url followed by the main ip.
  client.print("<b> CurrentLocation </b><br>");
  client.print("<input type='text' name=currentLocation value='45th'><br>");
  client.print("<b> TimeToStation </b><br>");
  client.print("<input type='number' name=timeToStation value='3'><br>");
  client.print("<b> AdvanceTime </b><br>");
  client.print("<input type='number' name=advanceTime value='1'><br>");
  client.print("<input type=submit value=calculate></form>");
  client.println("</body>");
  client.println("</html>");
  client.stop();
}
