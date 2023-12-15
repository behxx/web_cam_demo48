#include "esp_camera.h"
#include <WiFi.h>
#include <WebSocketsServer.h>
#include "SPIFFS.h"
#include <HCSR04.h>
#include "time.h"

#include "camera_pins.h"

// variable definations
const char* ssid = "MI 6";
const char* password = "beh01234";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 8*60*60;
const int   daylightOffset_sec = 0;

WebSocketsServer webSocket = WebSocketsServer(81);
WebSocketsServer webSocketFunction = WebSocketsServer(82);

WiFiServer server(80);

#define LED 33
#define IR 14
HCSR04 hc(15, 13); //initialisation class HCSR04 (trig pin , echo pin)

uint8_t cam_num;
bool connected = false;
bool LEDonoff = false;
bool IRonoff = false; 
String JSONtxt;

// function declarations
void configCamera();
void liveCam(uint8_t num);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void webSocketEventFunction(uint8_t num, WStype_t type, uint8_t *payload, size_t welength);
String myLocalTime();

void http_resp();
void sendHtmlFile(WiFiClient client, const char* filename);

void setup() {
  Serial.begin(115200);
  
  //setup pinMODE
  pinMode(LED, OUTPUT);
  pinMode(IR, INPUT);

  // check SPIFFS mount
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  //WiFi initiate
  WiFi.begin(ssid, password);
  Serial.println("");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  String IP = WiFi.localIP().toString();
  Serial.println("IP address: " + IP);

  //begin server and websockets
  server.begin();
  Serial.println("Server started");

  //websocket initiate and event handler for stream
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("Stream Websocket started");

  //websocket initiate and event handler for LED
  webSocketFunction.begin();
  webSocketFunction.onEvent(webSocketEventFunction);
  Serial.println("Websocket function started");
  
  configCamera();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {
  http_resp();
  
  webSocket.loop();
  if(connected == true){
    liveCam(cam_num);
  }
  webSocketFunction.loop();
    if(connected == true){
      if(LEDonoff == false) digitalWrite(LED, HIGH);
      else digitalWrite(LED, LOW);
    
      if(digitalRead(IR) == HIGH) IRonoff = true;
      else IRonoff = false;

      

    
  //-----------------------------------------------
    String LEDstatus = "OFF";
    String USONIC_ValString = String(hc.dist());
    String IRstatus = "OFF";

    String myCurrentTime = myLocalTime();
    
    if(LEDonoff == true) LEDstatus = "ON";
    if(IRonoff == true) IRstatus = "ON";
    
    JSONtxt = "{\"LEDonoff\":\""+LEDstatus+"\",";
    JSONtxt += "\"IRonoff\":\""+IRstatus+"\",";
    JSONtxt += "\"myTIME\":\""+myCurrentTime+"\",";
    JSONtxt += "\"DIST\":\""+USONIC_ValString+"\"}";
    
    webSocketFunction.broadcastTXT(JSONtxt);
  }
}

//config CAM OV2540
void configCamera(){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 4;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}

//streaming function, capture frame send as BINARY to html
void liveCam(uint8_t num){
  //capture a frame
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
      Serial.println("Frame buffer could not be acquired");
      return;
  }
  //replace this with your own function
  webSocket.sendBIN(num, fb->buf, fb->len);

  //return the frame buffer back to be reused
  esp_camera_fb_return(fb);
}

//websocket EVENT handlers
void webSocketEventFunction(uint8_t num, WStype_t type, uint8_t *payload, size_t welength)
{
  String payloadString = (const char *)payload;
  Serial.print("payloadString= ");
  Serial.println(payloadString);

  
  
  if(type == WStype_TEXT) //receive text from client
  {
    byte separator=payloadString.indexOf('=');
    String var = payloadString.substring(0,separator);
    Serial.print("var= ");
    Serial.println(var);
    String val = payloadString.substring(separator+1);
    Serial.print("val= ");
    Serial.println(val);
    Serial.println(" ");

    
    if (var == "LEDonoff")
    {
      LEDonoff = false;
      if (val == "ON") LEDonoff = true;
    }

    if (var == "schedule")
    {
      byte timeSeperator = payloadString.indexOf(':'); 
      String h_val = payloadString.substring(separator+1, timeSeperator);
      String m_val = payloadString.substring(timeSeperator+1);
      Serial.print("h_val= ");
      Serial.println(h_val);
      Serial.print("m_val= ");
      Serial.println(m_val);
    }
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            cam_num = num;
            connected = true;
            break;
        case WStype_TEXT:
        case WStype_BIN:
        case WStype_ERROR:      
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            break;
    }
}

void http_resp(){
  WiFiClient client = server.available();
  if (client.connected() && client.available()) {                   
    client.flush();
              
    // Send a standard HTTP response header
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();

    // Read and send HTML code from the external file
    sendHtmlFile(client, "/index.html");
    
    client.stop();
  }
}

void sendHtmlFile(WiFiClient client, const char* filename) {
  // Open the HTML file
  File htmlFile = SPIFFS.open(filename);
  
  if (htmlFile) {
    // Read and send each line of the HTML file
    while (htmlFile.available()) {
      client.write(htmlFile.read());
    }
    htmlFile.close();
  } else {
    Serial.println("Error opening HTML file");
  }
}

String myLocalTime(){
  struct tm timeinfo;
  String myErrorMsg = "Error obtaining time!";
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return myErrorMsg;
  }

  String myTime;
  
  char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);
  char timeMinute[3];
  strftime(timeMinute,3, "%M", &timeinfo);
  char timeSecond[3];
  strftime(timeSecond,3, "%S", &timeinfo);
  
  myTime += timeHour;
  myTime += ':';
  myTime += timeMinute;
  myTime += ':';
  myTime += timeSecond;
  
  return myTime;
  }
