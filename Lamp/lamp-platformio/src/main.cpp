#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266httpUpdate.h>

#include <EEPROM.h>

#include <Adafruit_NeoPixel.h>

#include <WebSocketsClient.h>
#include <SocketIOclient.h>

#include <ArduinoJson.h>

#include <string.h>
using std::string;

SocketIOclient socketIO;

#define SSID "FRITZ!Box 7490"
#define PASS "30904355349185884244"

#define APSSID "Hi-Light-setup"
#define APPASS "vagarthisawesome"

#define URL "wifi-lamp.herokuapp.com"
#define PORT 80

// #define URL "192.168.178.108"
// #define PORT 5000

#define BUTTON D6
#define LIGHT D7

#define NUMPIXELS 12

/* Put IP Address details */
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

ESP8266WebServer server(80);

Adafruit_NeoPixel pixels(NUMPIXELS, LIGHT, NEO_GRB + NEO_KHZ800);

unsigned long startTime;
unsigned long endTime;
unsigned long duration;
byte timerRunning;

int wifimode = 0;
int lightStatus = 0;
bool socketStatus = 0;

String esid = "", epass = "";

char lamp_id;

void connectWifi()
{

  pixels.begin();

  pixels.fill(0);
  pixels.show();
  pixels.setBrightness(30);

  WiFi.mode(WIFI_STA);
  WiFi.begin(esid, epass);

  unsigned long startWifi = millis();

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");

    if ((millis() - startWifi) >= 30000)
    {
      Serial.println("\nWireless AP mode");
      WiFi.softAP(APSSID, APPASS);
      WiFi.softAPConfig(local_ip, gateway, subnet);
      return;
    }
  }
  Serial.println(".");

  String ip = WiFi.localIP().toString();
  Serial.printf("[SETUP] WiFi Connected %s\n", ip.c_str());
}

void handleBroadcast(uint8_t *payload)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    const char *eventNames[4] = {"broadcastRecieve", "updateAvailable", "turnOff", "turnOn"};

    Serial.printf("[IOc] Data: %s\n", payload);

    DynamicJsonDocument doc(1024);

    deserializeJson(doc, payload);

    const char *recievedEventName = doc[0];

    if (strcmp(recievedEventName, eventNames[0]) == 0)
    {
      pixels.fill(doc[1]["color"]);
      pixels.show();
      lightStatus = 1;
    }
    else if (strcmp(recievedEventName, eventNames[1]) == 0)
    {
      Serial.println("Updating");
      delay(1000);
      // Update when server sends updateAvailable
      ESPhttpUpdate.update(URL, PORT, "/firmware.bin");
    }
    else if (strcmp(recievedEventName, eventNames[2]) == 0)
    {

      const char *id = doc[1]["id"];

      //Serial.println(id);

      if (strcmp(id, &lamp_id) == 0)
      {
        pixels.fill(0);
        pixels.show();
        lightStatus = 0;
      }
    }
    else if (strcmp(recievedEventName, eventNames[3]) == 0)
    {

      const char *id = doc[1]["id"];

      //Serial.println(id);

      if (strcmp(id, &lamp_id) == 0)
      {
        pixels.fill(14011031);
        pixels.show();
        lightStatus = 1;
      }
    }
  }
}

void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case sIOtype_DISCONNECT:
  {
    //Serial.printf("[IOc] Disconnected!\n");
    socketStatus = 0;
    break;
  }
  case sIOtype_CONNECT:
  {
    Serial.printf("[IOc] Connected to url: %s\n", payload);
    // join default namespace (no auto join in Socket.IO V3)
    socketIO.send(sIOtype_CONNECT, "/");

    socketStatus = 1;

    if (WiFi.status() == WL_CONNECTED && socketStatus == 1)
    {
      delay(5000);

      Serial.println("Sending ID");

      // creat JSON message for Socket.IO (event)
      DynamicJsonDocument doc(1024);
      JsonArray array = doc.to<JsonArray>();

      // add event name
      // Hint: socket.on('event_name', ....
      array.add("lampConnected");

      // add payload (parameters) for the event
      JsonObject param1 = array.createNestedObject();
      param1["id"] = lamp_id;

      // JSON to String (serializion)
      String output;
      serializeJson(doc, output);

      // Send event
      socketIO.sendEVENT(output);
    }

    break;
  }
  case sIOtype_EVENT:
  {
    handleBroadcast(payload);
    break;
  }
  case sIOtype_ERROR:
    Serial.printf("[IOc] get error: %u\n", length);
    hexdump(payload, length);
    break;
  }
}

void connectSocket()
{

  // server address, port and URL
  socketIO.begin(URL, PORT);

  // event handler
  socketIO.onEvent(socketIOEvent);
}

void sendBroadcastRequest()
{
  if (WiFi.status() == WL_CONNECTED && socketStatus == 1)
  {
    Serial.println("Sending Broadcast");

    // creat JSON message for Socket.IO (event)
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();

    // add event name
    // Hint: socket.on('event_name', ....
    array.add("broadcastRequest");

    // add payload (parameters) for the event
    JsonObject param1 = array.createNestedObject();
    param1["id"] = lamp_id;
    param1["broadcasting"] = true;

    // JSON to String (serializion)
    String output;
    serializeJson(doc, output);

    // Send event
    socketIO.sendEVENT(output);
  }
}

void selectWifiMode()
{
  if (wifimode == 0)
  {
    Serial.println("Wireless AP mode");

    WiFi.softAP(APSSID, APPASS);
    WiFi.softAPConfig(local_ip, gateway, subnet);

    wifimode = 1;
  }
  else if (wifimode == 1)
  {

    Serial.println("Wireless station mode");

    connectWifi();
    connectSocket();

    wifimode = 0;
  }
}

void toggleLights()
{
  if (lightStatus == 0)
  {
    pixels.fill(14011031);
    pixels.show();
    lightStatus = 1;
  }
  else if (lightStatus == 1)
  {
    pixels.fill(0);
    pixels.show();
    lightStatus = 0;
  }
}

void handleButtonEvents()
{
  if (timerRunning == 0 && digitalRead(BUTTON) == HIGH)
  {
    startTime = millis();
    timerRunning = 1;
  }
  if (timerRunning == 1 && digitalRead(BUTTON) == LOW)
  {
    endTime = millis();
    timerRunning = 0;
    duration = endTime - startTime;
    Serial.print("button press time in milliseconds: ");
    Serial.println(duration);

    if (duration >= 100 && duration <= 1000) // If pressed within a second then toggle warm light
    {
      toggleLights();
    }
    else if (duration <= 5000 && duration >= 3000) // If pressed for more than 3 secs and less than 7 secs then send broadcast
    {
      sendBroadcastRequest();
    }
    else if (duration >= 8000) // If pressed for longer than 8 secs then set AP mode to get ssid and password
    {
      selectWifiMode();
    }
  }
}

String SendHTML()
{
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>LED Control</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px; background-color: #fcba03;}\n";
  ptr += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<h1>Hi-Light\n";
  ptr += "<h2>WiFi Konfiguration</h2>\n";
  ptr += "<h3>Trage deine WiFi daten ein!</h3>\n";
  ptr += "</p><form method='get' action='setting'><label>Name:   </label><input name='ssid' length=32><br><br><label>Pass:    </label><input name='pass' length=64><br><br><center><input type='submit' value='Speichern'></center></form>\n";

  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}

void handle_OnConnect()
{
  server.send(200, "text/html", SendHTML());
}

void setting()
{

  String st;
  String content;
  int statusCode;

  String qsid = server.arg("ssid");
  String qpass = server.arg("pass");

  if (qsid.length() > 0 && qpass.length() > 0)
  {
    Serial.println("clearing eeprom");
    for (int i = 0; i <= 95; ++i)
    {
      EEPROM.write(i, 0);
    }
    Serial.println("writing eeprom ssid.");
    for (unsigned int i = 0; i < qsid.length(); ++i)
    {
      EEPROM.write(i, qsid[i]);
    }
    Serial.println("writing eeprom pass.");
    for (unsigned int i = 0; i < qpass.length(); ++i)
    {
      EEPROM.write(32 + i, qpass[i]);
    }
    EEPROM.commit();
    content = "<h1 style='color: green; font-size: 50px; text-align: center; margin: 50px'>Saved successfully</h1>";
    statusCode = 200;

    server.send(statusCode, "text/html", content);

    delay(3000);

    ESP.restart();
  }
  else
  {
    content = "<h1 style='color: red; font-size: 50px; text-align: center; margin: 50px'>Error! Check wifi name and password.<h1> <br><br> <a href='/'>Zurück</a>";
    statusCode = 200;
    server.send(statusCode, "text/html", content);
  }
}

void setup()
{

  Serial.begin(115200);

  EEPROM.begin(512); //Initialize EEPROM

  server.on("/", handle_OnConnect);
  server.on("/setting", setting);

  server.begin();

  //EEPROM.write(96, 'P');
  //EEPROM.commit();

  for (int i = 0; i <= 31; ++i)
  {
    esid += char(EEPROM.read(i));
  }

  for (int i = 32; i <= 95; ++i)
  {
    epass += char(EEPROM.read(i));
  }

  lamp_id = char(EEPROM.read(96));

  Serial.println(esid);
  Serial.println(epass);
  Serial.println(lamp_id);

  connectWifi();
  connectSocket();

  pinMode(D6, INPUT_PULLUP);
}

void loop()
{
  server.handleClient();
  socketIO.loop();

  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.softAP(APSSID, APPASS);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    wifimode = 1;
  }

  handleButtonEvents();
}
