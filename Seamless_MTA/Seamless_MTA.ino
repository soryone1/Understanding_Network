
/*
  WiFi Web Server

  A simple web server that shows the value of the analog input pins.

  This example is written for a network using WPA encryption. For
  WEP or WPA, change the Wifi.begin() call accordingly.

  Circuit:
   Analog inputs attached to pins A0 through A5 (optional)

  HTTP requests list:

  1. GET http://172.20.10.13/check/nexttrain HTTP/1.1   // to get the time of when will the next tain comes

*/

/* setup wifi connections*/
#include <SPI.h>
#include <WiFiNINA.h>
//#include <ArduinoHttpClient.h>

/* include all the config files*/
#include "config.h"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
//int keyIndex = 0;                 // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;

WiFiServer server(80);

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
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

    // wait 5 seconds for connection:
    delay(5000);
  }

  /* When connected, start the server */
  server.begin();
  /* you're connected now, so print out the status: */
  printWifiStatus();
}


void loop() {

  // listen for incoming clients
  WiFiClient client = server.available();

  if (client.available()) {
    Serial.println("new client");

    // an http request ends with a blank line
//    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client) {
        String userRequest = client.readStringUntil(' ');      // get the request method
        Serial.println(userRequest);
        if (userRequest == "GET" || userRequest == "POST") {
          String lastToken = "";                     // last token the board reads
          while (!lastToken.endsWith("HTTP")) {
            Serial.println("readtoken");
            String currentToken = client.readStringUntil('/');
            if (lastToken == "nexttrain") {
              Serial.println("66666");
            }
            lastToken = currentToken;
          }
          client.println("HTTP 200 OK\n\n");
          if (client.connected()) {
            client.stop();
          }
        }

        //        char c = client.read();
        //        Serial.write(c);
        //        // if you've gotten to the end of the line (received a newline
        //        // character) and the line is blank, the http request has ended,
        //        // so you can send a reply
        //        if (c == '\n' && currentLineIsBlank) {
        //          // send a standard http response header
        //          client.println("HTTP/1.1 200 OK");
        //          client.println("Content-Type: text/html");
        //          client.println("Connection: close");  // the connection will be closed after completion of the response
        //          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
        //          client.println();
        //          client.println("<!DOCTYPE HTML>");
        //          client.println("<html>");
        //          // output the value of each analog input pin
        //          for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
        //            int sensorReading = analogRead(analogChannel);
        //            client.print("analog input ");
        //            client.print(analogChannel);
        //            client.print(" is ");
        //            client.print(sensorReading);
        //            client.println("<br />");
        //          }
        //          client.println("</html>");
        //          break;
        //        }
        //        if (c == '\n') {
        //          // you're starting a new line
        //          currentLineIsBlank = true;
        //        } else if (c != '\r') {
        //          // you've gotten a character on the current line
        //          currentLineIsBlank = false;
        //        }
        //      }
        //    }
        //    // give the web browser time to receive the data
        //    delay(1);
        //
        //    // close the connection:
        //    client.stop();
        //    Serial.println("client disonnected");
      }
    }
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
