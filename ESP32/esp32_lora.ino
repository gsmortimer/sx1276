
/* ESP32 Lora Control
 *  Connect as follows
 *  ESP32     RFM95W(Lora Module)
 *  Gnd       Gnd
 *  3.3V      3.3v
 *  GPIO12    MISO
 *  GPIO13    MOSI
 *  GPIO14    SCK
 *  GPIO15    NSS
 *  
 *  Caution: Commands LORA directly. This is NOT LORAWAN
 *  
 * created June 2020 by George Mortimer
 */

/* Only tested on an ESP32. For other devices, you may
 * Need to uncomment the following, but this may have undesired results
 */
// #define ESP32
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include "SX1276.h"

/* Set the IP Addresses */
IPAddress staticIP(192,168,1,40);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

// Network Details
const char* ssid = "SSID";
const char* password = "PASSWORD";

// Start webserver on localhost
WebServer server(80);
String webPage = "";

SX1276 * lora = NULL;


void setup() {
  //Lora Options
  Serial.begin(115200); 
  lora = new SX1276();   
  Serial.println("Setup");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password); //Start Wifi

  //set up slave select pins as outputs as the Arduino API
  //doesn't handle automatically pulling SS low


  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(WiFi.status());
   // WiFi.printDiag(Serial);
  }
  while (WiFi.config(staticIP, gateway, subnet, gateway) != true) { 
    Serial.print("Failed to set IP");
  }
  delay(500);
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  lora->Init(1,1);

   // Set up Webserver Handles
  server.on("/",      page_root);
  server.on("/tx",      page_tx);
  server.on("/rx",      page_rx);
  server.on("/cad",      page_cad);
  server.on("/relay",      page_relay);
  server.on("/set",      page_set);
  server.on("/get",      page_get);
  server.on("/init",      page_init);
  server.onNotFound(  page_notfound);

  server.begin();
}


void loop() {
  server.handleClient();

  delay(100);
}
