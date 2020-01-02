#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <FastLED.h>

ESP8266WebServer server(80);

String ssid = "Lighting_Controller";
String password = "led12345";
String hostName = "host";
String code = "0000";
String tokenServer = "token12345";
String tokenCliente = "";
String statusPower = "0";
String ipHost="192.168.4.66";
int s1 = 2;

unsigned long count=0;
int bright=0;
int numberGroups=0;


#define NUM_LEDS 200
#define DATA_PIN 3
CRGB leds[NUM_LEDS];

DynamicJsonDocument doc(1024);


unsigned long previousMillis = 0;
unsigned long interval = 1000;


void setup() {

  FastLED.addLeds<WS2812, DATA_PIN, RGB>(leds, NUM_LEDS);
  pinMode(s1, OUTPUT);
  digitalWrite(s1, HIGH);

  Serial.begin(115200);
  Serial.println("   ");
  connectWiFiAP(ssid, password);
  serverRoutes();
}

void loop() {

  
  ledsLoop();

}

void connectWiFiAP(String confSsid, String confPassword) {
  IPAddress local_IP(192,168,4,10);
  IPAddress gateway(192,168,4,1);
  IPAddress subnet(255,255,255,0);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(1000);
  WiFi.mode(WIFI_AP);
  
  delay(100);
  Serial.print("Setting soft-AP configuration ... ");
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");
  delay(100);
  Serial.print("Setting soft-AP ... ");
  Serial.println(WiFi.softAP(confSsid, confPassword) ? "Ready" : "Failed!");
  delay(100);
  
  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());
}

void serverRoutes() {

  server.on("/power", HTTP_POST, power);
  server.on("/wifiConnect", HTTP_POST, wifiConnect);
  server.on("/wifiUpdate", HTTP_POST, wifiUpdate);
  server.on("/powerStatus", HTTP_GET, powerStatus);
  server.on("/wifiReset", HTTP_GET, wifiReset);
  server.on("/getConfig", HTTP_GET, getConfig);
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");


}

void handleRoot() {
  server.send(200, "text/plain", "Custom Homepage to come with svg logo and instruction manual link");
}

void handleNotFound() {
  server.send(404, "text/plain", "404: The function you are looking for does not exit.");
}

void power() {

  tokenCliente = server.arg(String("token")) ;
  statusPower = server.arg(String("status"));
  Serial.println(tokenCliente);
  Serial.println(statusPower);

  if (tokenServer == tokenCliente) {
    if (statusPower == "0") {
      digitalWrite(s1, HIGH);
      server.send(200, "text/plain", "{'result':0}");
    }
    if (statusPower == "1") {
      digitalWrite(s1, LOW);
      server.send(200, "text/plain", "{'result':1}");
    }

    if (statusPower != "0" && statusPower != "1"  ) {
      digitalWrite(s1, LOW);
      server.send(200, "text/plain", "{'result':'Not status'}");
    }

  }
  else {
    server.send(200, "text/plain", "{'result':'Token not valid'}");
  }

}

void powerStatus() {

  tokenCliente = server.arg(String("token")) ;

  if (tokenServer == tokenCliente) {

    server.send(200, "text/plain", "{'result':" + statusPower + "}");

  }
  else {

    server.send(200, "text/plain", "{'result':'Token not valid'}");

  }

}

void wifiConnect() {

  String newSsid = server.arg(String("newssid")) ;
  String newPassword  = server.arg(String("newpassword")) ;
  hostName = server.arg(String("newnamehost"));
  String newCode = server.arg(String("newcode"));

  if (code == "0000") {

    code = newCode;
    server.send(200, "text/plain", "{'result':'OK'}");
    delay(3000);
    connectWiFiAP(newSsid, newPassword);


  }
  else {
    server.send(200, "text/plain", "{'result':'Not function'}");
  }


}

void wifiUpdate() {

  String newSsid = server.arg(String("newssid")) ;
  String newPassword  = server.arg(String("newpassword")) ;
  hostName = server.arg(String("newnamehost"));
  String newCode = server.arg(String("code"));

  if (code == newCode) {
    server.send(200, "text/plain", "{'result':'OK'}");
    delay(3000);
    connectWiFiAP(newSsid, newPassword);


  }
  else {
    server.send(200, "text/plain", "{'result':'Invalid code'}");
  }


}

void wifiReset() {

  String newCode = server.arg(String("code"));

  if (code == newCode) {

    code = "0000";
    server.send(200, "text/plain", "{'result':'OK'}");
    delay(3000);
    connectWiFiAP(ssid, password);


  }
  else {
    server.send(200, "text/plain", "{'result':'Invalid code'}");
  }

}

void getConfig() {
    
   
   String payload="";
   HTTPClient http;  
 
    http.begin("http://"+ipHost+"/activeConfig/?id="+hostName); 
    int httpCode = http.GET();                                                           

    if (httpCode > 0) { 
       payload= http.getString();   
      Serial.println(payload);                 
      server.send(200, "text/plain", payload);
    }
    http.end();  

    if(payload!=""){
      
      char payloadChar[payload.length()];
      payload.toCharArray(payloadChar, payload.length());
      DeserializationError error = deserializeJson(doc,payloadChar);

      
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
      }

       deserializeJson(doc, payloadChar);

      count = doc["count"];
      bright = doc["bright"]; 
      numberGroups = doc["numberGroups"]; 

      interval= count;
      
      
      }
 
  }

 void ledsLoop(){
  
  
 server.handleClient();

         if(numberGroups>0){
             int i=0;
             int j=0;
             

             while(i<numberGroups){
              server.handleClient();
             int numberLedsGroups=doc["numberLedsGroups"][i];
             unsigned long currentMillis = millis();
             JsonArray groups = doc["groups"];
             
              while(j<numberLedsGroups){
                server.handleClient();
               
                if (currentMillis - previousMillis >= interval) { 
                   
                   JsonArray colors = doc["groups"][i][0][j];
                   int ledsIn= doc["groups"][i][1][j];
                   int R=colors[0];
                   int G=colors[1];
                   int B=colors[2];
                      leds[ledsIn] = CHSV(0,0,0);
                      FastLED.show();
                      leds[ledsIn] = CHSV(R, G, B);
                      FastLED.show();
                      
                   j++;
                  
                  
                  }

                  
                
                }
              i++;
      
             }
          
          
          }

       
      // if (currentMillis - previousMillis >= interval) { }
   
   //JsonArray colors = doc["groups"][i][0][j];
  //int leds= doc["groups"][i][1][j];
   
  
  
  }
