/*
IoT cushion code

This code is largely based on WebSocketServer_LEDcontrol.ino
https://github.com/Links2004/arduinoWebSockets

Run ESP-8266 with this sketch, access this URL from your Mac or iPhone
http://esp-cushion.local
You would see live update of data with plot

Author: Shunya Sato

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Hash.h>

#include "Timer.h"

#define USE_SERIAL Serial

ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server = ESP8266WebServer(80);
WebSocketsServer webSocket = WebSocketsServer(81);

int sensorPin = A0;
int sensorValue = 0;
int ledPin = 0;
Timer t;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {

    switch(type) {
        case WStype_DISCONNECTED:
            USE_SERIAL.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

            // send message to client
            webSocket.sendTXT(num, "Connected");
        }
            break;
        case WStype_TEXT:
            USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);
            break;
    }

}

void readADC(){
  sensorValue = analogRead(sensorPin);
  Serial.println(sensorValue);
  String tmp = String(sensorValue);
  webSocket.broadcastTXT(tmp);
  if (judge() == true){
    takeAction();
  }
}

bool judge(){
  if (sensorValue < 450){
    digitalWrite(ledPin, LOW);
    return true;
  }
  else{
    digitalWrite(ledPin, HIGH);  
    return false;
  }
}

void takeAction(){
  Serial.println("takeAction!");
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();
  for(uint8_t t = 4; t > 0; t--) {
      USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
      USE_SERIAL.flush();
      delay(1000);
  }
  WiFiMulti.addAP(“SSID", “password");
  WiFiMulti.addAP(“SSID2", “password");
  
  while(WiFiMulti.run() != WL_CONNECTED) {
      delay(100);
  }

  // start webSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  if(MDNS.begin("esp-cushion")) {
      USE_SERIAL.println("MDNS responder started");
  }

  // handle index
  server.on("/", []() {
      // send index.html
      server.send(200, "text/html", "<html><head><script type='text/javascript' src='https://cdnjs.cloudflare.com/ajax/libs/smoothie/1.27.0/smoothie.js'></script></head><body>Current sensor reading: <span id='nowvalue'></span><br/><br/><canvas id='mycanvas' width='400' height='100'></canvas><script>var smoothie = new SmoothieChart({  grid: { strokeStyle:'rgb(125, 125, 125)', fillStyle:'rgb(60, 0, 0)',          lineWidth: 1, millisPerLine: 250, verticalSections: 6, },  labels: { fillStyle:'rgb(255, 255, 0)' }});smoothie.streamTo(document.getElementById('mycanvas'), 100 /*delay*/);var line1 = new TimeSeries();smoothie.addTimeSeries(line1,  { strokeStyle:'rgb(0, 255, 0)', fillStyle:'rgba(0, 255, 0, 0.4)', lineWidth:3 });var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);connection.onopen = function () {  connection.send('Connect ' + new Date());};connection.onerror = function (error) {  console.log('WebSocket Error ', error);};connection.onmessage = function (e) {  console.log('Server: ', e.data);  line1.append(new Date().getTime(), parseFloat(e.data));  document.getElementById('nowvalue').innerHTML = e.data;};</script></body></html>");
  });

  server.begin();

  // Add service to MDNS
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);

  pinMode(sensorPin, INPUT);
  pinMode(ledPin, OUTPUT);
  // t.every(10, readADC); // 10ms interval = 100Hz
  // t.every(20, readADC); // 20ms interval = 50Hz
  t.every(100, readADC); // 100ms interval = 10Hz

}

void loop() {
  webSocket.loop();
  server.handleClient();
  t.update();
}
