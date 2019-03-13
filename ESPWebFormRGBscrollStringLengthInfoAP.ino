/*
 * MessageBoard LED Matrix Webserver
 *Many thanks to Brian Lough for the setup of the MAtrix:
 *https://www.hackster.io/brian-lough/rgb-led-matrix-with-an-esp8266-a16fa9
 *
 *Thanks to hariFun (https://www.instructables.com/member/HariFun/) for the cool green animation!
 *
 *Connect to ADHOC Network "messageboard" to input wifi password and select network.
 *Password for "messageboard" is "Washington99"
 * Connect to "http://messageboard.local" or the IP Address shown on board
 * to bring up an HTML form to control the connected LED Matrix. 
 * 
 *
 * 
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <PxMatrix.h>
#include <Fonts/FreeSansBoldOblique9pt7b.h>
#include <Fonts/FreeSansBoldOblique12pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
/*Code for Matrix Display
 * This controls the display
 */

#ifdef ESP32

#define P_LAT 22
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5
#define P_E 15
#define P_OE 2
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

#endif

#ifdef ESP8266

#include <Ticker.h>
Ticker display_ticker;
#define P_LAT 16
#define P_A 5
#define P_B 4
#define P_C 15
#define P_D 12
#define P_E 0
#define P_OE 2

#endif

// Pins for LED MATRIX
PxMATRIX display(64,32,P_LAT, P_OE,P_A,P_B,P_C,P_D,P_E);

#ifdef ESP8266
// ISR for display refresh
void display_updater()
{
  //display.displayTestPattern(70);
  display.display(70);
}
#endif

#ifdef ESP32
void IRAM_ATTR display_updater(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  //isplay.display(70);
  display.displayTestPattern(70);
  portEXIT_CRITICAL_ISR(&timerMux);
}
#endif

void DisplayHariFun(String MESSAGEvalue, String color)
{
  display.fillScreen(display.color565(0, 0,0));
  
  display.setTextSize(1);
  
  //display.setFont(&FreeSansBoldOblique9pt7b);
  display.setFont(&FreeSansBoldOblique12pt7b);
  //display.setFont(&FreeMonoBold12pt7b);
  //display.setCursor(14,31-18); display.print(MESSAGEvalue);

  int textX;
  int textMin;
  textMin = sizeof(MESSAGEvalue) * -24;
  for (int textX=64; textX>textMin; textX-=6){

  delay(150);
  display.setBrightness(200);
  display.clearDisplay();
  if (color == "red"){
    display.setTextColor(display.color565(255, 0, 0));}
  else if (color == "blue"){
    display.setTextColor(display.color565(0, 0, 255));}
  else if (color == "green"){
    display.setTextColor(display.color565(0, 255, 0));}
  else {
    display.setTextColor(display.color565(255, 255, 0));}
  display.clearDisplay();
  display.setCursor(textX,-9);
  //if((--textX) < textMin) textX = 64;
  display.print(MESSAGEvalue);
  delay(150);
  }
  
  //display.clearDisplay();
  //instructions();
  //display.setFont(&FreeSansBoldOblique12pt7b);
  //display.setTextColor(display.color565(255,255,0));
  //display.setCursor(31-26,31); display.print(MESSAGEvalue);
}

void Chord(int r, float rot)
{
  int nodes = 6;
  float x[nodes];
  float y[nodes];
  for (int i=0; i<nodes; i++)
  {
    float a = rot + (PI*2*i/nodes);
    x[i] = 31+3 + cos(a)*r;
    y[i] = 16 + sin(a)*r;
  }

  display.fillScreen(display.color565(0, 0,0));
  for (int i=0; i<(nodes-1); i++)
    for (int j=i+1; j<nodes; j++)
      display.drawLine(x[i],y[i], x[j],y[j], display.color565(0, 255,0));
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster

}


MDNSResponder mdns;

ESP8266WebServer server(80);

const char INDEX_HTML[] =
"<!DOCTYPE HTML>"
"<html>"
"<head>"
"<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
"<title>Send your message!</title>"
"<style>"
"\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\""
"</style>"
"</head>"
"<body>"
"<h1>Send me a message!</h1>"
"<FORM action=\"/\" method=\"post\">"
"<P>"
"Type your message here<br>"
"<INPUT type=\"text\" name=\"message\"><BR>"
"<INPUT type=\"radio\" name=\"Font\" value=\"red\">red<BR>"
"<INPUT type=\"radio\" name=\"Font\" value=\"blue\">blue<BR>"
"<INPUT type=\"radio\" name=\"Font\" value=\"green\">green<BR>"
"<INPUT type=\"radio\" name=\"Font\" value=\"yellow\">yellow<BR>"
"<INPUT type=\"submit\" value=\"Send\"> <INPUT type=\"reset\">"
"</P>"
"</FORM>"
"</body>"
"</html>";

// GPIO#0 is for Adafruit ESP8266 HUZZAH board. Your board LED might be on 13.
//const int LEDPIN = 0;

void handleRoot()
{
  if (server.hasArg("message")) {
    handleSubmit();
  }
  else {
    server.send(200, "text/html", INDEX_HTML);
  }
}

void returnFail(String msg)
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(500, "text/plain", msg + "\r\n");
}

void handleSubmit()
{
  String MESSAGEvalue;
  String FONTvalue;
  
  if (!server.hasArg("message")) return returnFail("BAD ARGS");
  MESSAGEvalue = server.arg("message");
  if (!server.hasArg("Font")){
    FONTvalue = "red";
  }
  FONTvalue = server.arg("Font");
  //insert code for display here
  
  delay(1500);
 
  float rot;
  float rotationSpeed = PI/15;
  for (int r=1; r<44; r+=3) {
    Chord(r, rot+=rotationSpeed);
    delay(50);
  }

  for (int r=1; r<44; r+=3) {
    Chord(44-r, rot-=rotationSpeed);
    delay(30);
  }
  DisplayHariFun(MESSAGEvalue, FONTvalue);
  instructions();
  server.send(200, "text/html", INDEX_HTML);

  
  
}

void returnOK()
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK\r\n");
}


void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void instructions(){
  display.setFont(NULL);
  display.fillScreen(display.color565(0, 0,0));
  display.clearDisplay();
  display.setBrightness(200);
  display.setTextColor(display.color565(255, 0, 255));
  display.setCursor(2,0);
  display.print("Message");
  display.setTextColor(display.color565(0, 255, 255));
  display.setCursor(2,8);
  display.print("Board");
  display.setTextColor(display.color565(255, 255, 0));
  display.setCursor(2,16);
  display.print(WiFi.localIP());
  display.setTextColor(display.color565(255, 0, 255));
}
void failedConn(){
  display.setFont(NULL);
  display.fillScreen(display.color565(0, 0,0));
  display.clearDisplay();
  display.setBrightness(200);
  display.setTextColor(display.color565(255, 0, 255));
  display.setCursor(2,0);
  display.print("Connection");
  display.setTextColor(display.color565(0, 255, 255));
  display.setCursor(2,8);
  display.print("failed.");
  display.setTextColor(display.color565(255, 255, 0));
  display.setCursor(2,24);
  display.print("Restarting");
  display.setTextColor(display.color565(255, 0, 255));
}

void openWifi(){
  display.setFont(NULL);
  display.fillScreen(display.color565(0, 0,0));
  display.clearDisplay();
  display.setBrightness(200);
  display.setTextColor(display.color565(255, 0, 255));
  display.setCursor(2,0);
  display.print("Open Wifi");
  display.setTextColor(display.color565(0, 255, 255));
  display.setCursor(2,8);
  display.print("and enter");
  display.setTextColor(display.color565(255, 255, 0));
  display.setCursor(2,16);
  display.print("password");
  display.setTextColor(display.color565(255, 0, 255));
}

void successConn(){
  display.setFont(NULL);
  display.fillScreen(display.color565(0, 0,0));
  display.clearDisplay();
  display.setBrightness(200);
  display.setTextColor(display.color565(255, 0, 255));
  display.setCursor(2,0);
  display.print("Connection");
  display.setTextColor(display.color565(0, 255, 255));
  display.setCursor(2,8);
  display.print("success!");
  display.setTextColor(display.color565(255, 255, 0));
  display.setCursor(2,24);
  display.print("-------");
  display.setTextColor(display.color565(255, 0, 255));
}

//try removing the word void from the function parameters!
void setup(void)
{ // Initialize RGB matrix
  Serial.begin(9600);
  display.begin(16);
  
  #ifdef ESP8266
    display_ticker.attach(0.002, display_updater);
  #endif

  #ifdef ESP32
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &display_updater, true);
    timerAlarmWrite(timer, 2000, true);
    timerAlarmEnable(timer);
  #endif

  delay(1000);
 
  
  
  
//WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("MessageBoard", "Washington99")) {
    Serial.println("failed to connect and hit timeout");
    failedConn();
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  successConn();
  
/*setup for server-led script
   * Here the Web server is set up
   *
   */
  //Serial.begin(115200);
  //WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ?");
  //Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (mdns.begin("messageboard", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  //server.on("/ledon", handleLEDon);
  //server.on("/ledoff", handleLEDoff);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.print("Connect to http://messageboard.local or IP shown on display");
  Serial.println(WiFi.localIP());
  instructions();
}

void loop(void)
{
  
  server.handleClient();
}
