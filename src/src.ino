/*
 *  Based on: http://homematic-forum.de/forum/viewtopic.php?t=29321 by user m.yoda
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <RCSwitch.h>

// ******* Network settings *******
const char* ssid     = "ssid";
const char* password = "password";

ESP8266WebServer server(80); // Initialize server on port 80
String response = "";

RCSwitch rcswitch = RCSwitch();

String timestamp() { // Uptime as hour:minute:second
  char t[10];
  int ms = millis();
  int h = ms/3600000;
  int m = ms/60000-h*60;
  int s = ms/1000-h*3600-m*60;
  sprintf (t,"%03d:%02d:%02d", h, m, s);
  return t;
}

void root_handle() { // when requesting '/'
  String temp = timestamp();
  response = "rc-switch via wifi\n\n";
  response = response + "Uptime: " + temp + " (h:m:s)\n";
  response = response + "Network: " + ssid + "\n";
  response = response + "Signal strength: " + String(WiFi.RSSI()) + " dBm\n\n";
  response = response + "HTTP pages:\n";
  response = response + "\"<IP address>/switch\" control a switch.\n";
  response = response + "\tParameter 'channel': channel 0-31\n";
  response = response + "\tParameter 'device': device 0-31\n";
  response = response + "\tParameter 'state': '1' is on, else off\n";
  response = response + "\tParameter 'b': '0' to interpret 'channel' and 'device' as int, else binary\n";
  response = response + "\n\tExamples: \n";
  response = response + "\t\t<IP address>/switch?channel=11110&device=11110&state=1\n";
  response = response + "\t\t<IP address>/switch?channel=30&device=30&state=1&b=0\n";
  server.send(300, "text/plain", response);
  delay(150);
  Serial.println(temp + " unspecific HTTP request");
}

void switch_handle() { // when requesting '/switch/'
  String chan = server.arg("channel");
  String dev = server.arg("device");
  String state = server.arg("state");
  String binaryMode = server.arg("b");
  char c_chan[10];
  char c_dev[10];

  if(chan.length() > 9 || dev.length() > 9) {
    server.send(200, "text/plain", "Request rejected: Args too long");
    return;
  }
  
  response = "Switched device " + chan + "/" + dev;
    
  if(binaryMode == "0") {
    // Convert int-strings to binary strings
    int i_chan = chan.toInt();
    int i_dev = dev.toInt();

    if(i_chan < 0 || i_chan > 31 || i_dev < 0 || i_chan > 31) {
      server.send(200, "text/plain", "Request rejected: malformed integer arguments");
      return;
    }

    for(int i = 0; i < 5; i++) {
      c_chan[4-i] = bitRead(i_chan, i) == 1 ? '1' : '0';
      c_dev[4-i] = bitRead(i_dev, i) == 1 ? '1' : '0';
    }
    
  } else {
    chan.toCharArray(c_chan, 10);
    dev.toCharArray(c_dev, 10);
  }
  
  if(state == "1") {
    rcswitch.switchOn(c_chan, c_dev);
    response += " on.";
  } else {
    rcswitch.switchOff(c_chan, c_dev);
    response += " off.";
  }

  server.send(200, "text/plain", response);
}

void setup() {
  // Open serial connection for debug information
  Serial.begin(9600);
  Serial.println("");
  Serial.println("WiFi RC-switch");
  
  // Connect to wireless
  WiFi.begin(ssid, password);
  Serial.print("Connecting");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" successful!");
  Serial.println("");
  Serial.print("Connected with: ");
  Serial.println(ssid);
  Serial.print("Signal strength: ");
  int rssi = WiFi.RSSI();
  Serial.print(rssi);
  Serial.println(" dBm");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("");

  // Connect requests with handlers
  server.on("/", root_handle);
  server.on("/switch", switch_handle);
  
  // Start Server
  server.begin();
  Serial.println(timestamp() + " HTTP server online.");

  rcswitch.enableTransmit(D3);
  Serial.println("433 MHz module online.");
}

void loop() {
  server.handleClient(); // wait for requests
}
