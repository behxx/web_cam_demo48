#include "esp_camera.h"
#include <WiFi.h>
#include <WebSocketsServer.h>
#include "SPIFFS.h"
#include <HCSR04.h>
#include "time.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "myServoFn.h"
#include <Servo.h>

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

#include "camera_pins.h"

// variable definations
const char* ssid = "P1G01";
const char* password = "pet@p1g01";

WebSocketsServer webSocket = WebSocketsServer(81);
WebSocketsServer webSocketFunction = WebSocketsServer(82);

WiFiServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

#define BOTtoken "6920454260:AAERypPhvrQhTSKVMDyGe5XZXRmf5LejN40"
//#define CHAT_ID 
WiFiClientSecure botClient;
UniversalTelegramBot bot(BOTtoken, botClient);

//#define LED_onboard 33
#define LED 13
#define IR 14
#define servoPin 12
#define PB 2
// HCSR04 hc(15, 13); //initialisation class HCSR04 (trig pin , echo pin)

int botMsgCount = 0;
boolean ledState = true;
int interval_500 = 500;
int interval_50 = 50;
long previousMillis = 0;
long currentMillis;
long previousMillisLoop = 0;
long currentMillisLoop;

uint8_t cam_num;
bool connected = false;
bool IRonoff = false; 

int turns_val = 1;
static int lastTurn;

//bot String
String CHAT_ID = "986451973";

// current time in int
int currentHour, currentMin, currentSec;
int arraySch[10];

// String to update page via JSON txt
String JSONtxt, myCurrentTime;
String mySavedSchedule = "Delete schedule to refresh!";

// function declarations
void configCamera();
void liveCam(uint8_t num);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void webSocketEventFunction(uint8_t num, WStype_t type, uint8_t *payload, size_t welength);
void saveSchedule(int h_val, int m_val);
//String readSchedule();
void dltSchedule();

void http_resp();
void sendHtmlFile(WiFiClient client, const char* filename);
String timerFn();

void setup() {
  Serial.begin(115200);
  
  pinMode(LED, OUTPUT);
  pinMode(IR, INPUT);
  pinMode(PB, INPUT);
  
  Servo servo1;
  servo1.attach(servoPin);
  servo1.write(60);
  lastTurn = 60;

  // check SPIFFS mount
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  //WiFi initiate
  WiFi.begin(ssid, password);
  botClient.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  
  Serial.println("");
  while (WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis > interval_500) 
    {
      previousMillis = currentMillis;
      ledState = !ledState; 
      digitalWrite(LED, ledState);  
      Serial.print(".");
    }
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
  timeClient.begin();
  timeClient.setTimeOffset(28800);
  //GMT +08 * 60 * 60
  
  digitalWrite(LED, HIGH); 
//  bot.sendMessage(CHAT_ID, "Bot started up", "");
  Serial.println("Bot started!");
}

void loop() {
  http_resp();
  
  webSocket.loop();
  if(connected == true){
    liveCam(cam_num);
  }
  webSocketFunction.loop();
  if(connected == true){
    
    String myCurrentTime = timerFn();
    if(digitalRead(PB) == LOW)
    {
      lastTurn = dispenseFn(turns_val, lastTurn);
    }
  
    if(digitalRead(IR) == HIGH) 
    {
      IRonoff = true;
      if (botMsgCount < 3)
      { 
        String msg = "Pet food is running low!!";
        bot.sendMessage(CHAT_ID, msg, "");
        botMsgCount++;
      }
    } else IRonoff = false;
    //-----------------------------------------------

    // String usonicStatus = "";
    String IRstatus = "OFF";
    
    // float distance = hc.dist();
    
    if(IRonoff == true) IRstatus = "ON";
    
    // distance > 40.0 ? usonicStatus = "ON" : usonicStatus = "OFF";

    JSONtxt = "{\"myTIME\":\""+myCurrentTime+"\",";
//    JSONtxt = "{\"IRonoff\":\""+IRstatus+"\",";
    JSONtxt += "\"IRonoff\":\""+IRstatus+"\",";
    JSONtxt += "\"mySCH\":\""+mySavedSchedule+"\"}";
    
    webSocketFunction.broadcastTXT(JSONtxt);

    //LED blinking
    currentMillisLoop = millis();
    if (currentMillisLoop - previousMillisLoop > interval_50) 
    {
      previousMillisLoop = currentMillisLoop;
      ledState = !ledState; 
      digitalWrite(LED, ledState);
    }
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
  config.jpeg_quality = 10;
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
    
     if (var == "dispense")
     {
       lastTurn = dispenseFn(turns_val, lastTurn);
     }

     if (var == "setTurns")
     {
       turns_val = val.toInt();
       Serial.print("turns is set to: ");
       Serial.println(turns_val);
     }

     if (var == "schedule")
     { 
       mySavedSchedule == NULL ? mySavedSchedule = val : mySavedSchedule = mySavedSchedule + ',' + val; 
       byte timeSeperator = payloadString.indexOf(':'); 
       String h_str = payloadString.substring(separator+1, timeSeperator);
       String m_str = payloadString.substring(timeSeperator+1);
      
       int h_val = h_str.toInt();
       Serial.print("h_val= ");
       Serial.println(h_val);
      
       int m_val = m_str.toInt();
       Serial.print("m_val= ");
       Serial.println(m_val);

       Serial.print("mySavedSchedule= ");
       Serial.println(mySavedSchedule);
       saveSchedule(h_val, m_val);
     }

     if (var == "del_sch")
     {
       //call dlt fn.  
       dltSchedule();
       mySavedSchedule = "";
       Serial.print("mySavedSchedule is:");
       Serial.println(mySavedSchedule);
     }

     if (var == "chatID")
     {
        CHAT_ID = val;
        Serial.println("CHAT_ID is now");
        Serial.println(val);
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

void saveSchedule(int h_val, int m_val) {
      if (!(SPIFFS.exists("/schedule.txt"))){
        File fileToWrite = SPIFFS.open("/schedule.txt", FILE_WRITE);
        if(!fileToWrite){
          Serial.println("There was an error opening the file for writing");
          return;
        }
        if(fileToWrite){
          fileToWrite.println(h_val);
          fileToWrite.println(m_val);
          Serial.println("File was written");;
        }else {
          Serial.println("File write failed");
        }
        fileToWrite.close();
      }else {
          File fileToAppend = SPIFFS.open("/schedule.txt", FILE_APPEND);
          if(!fileToAppend){
            Serial.println("There was an error opening the file for appending");
            return;
          }
          if(fileToAppend){
              fileToAppend.println(h_val);
              fileToAppend.println(m_val);
              Serial.println("File content was appended");
          } else {
              Serial.println("File append failed");
          }
          fileToAppend.close();
     }

     File fileToRead = SPIFFS.open("/schedule.txt");
     if(!fileToRead){
        Serial.println("Failed to open file for reading");
        return;
      }
     Serial.println("File Content:");
      int i = 0;
      char buffer[64];
      
      while (fileToRead.available()){
        int l = fileToRead.readBytesUntil('\n', buffer, sizeof(buffer)-1);
        buffer[l] = 0;
        
        arraySch[i] = atoi(buffer);
        Serial.println(arraySch[i]);
        i++;
      }
      fileToRead.close();
}

void dltSchedule()
{
  if (SPIFFS.remove("/schedule.txt"))
  {
    Serial.println("File deleted");
    return;
  }else {
    Serial.println("Delete failed");
  }
}

String timerFn()
{
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  currentHour = timeClient.getHours();
  currentMin = timeClient.getMinutes();
  currentSec = timeClient.getSeconds();

  checkMinFn();

  String currentHour_str = String(currentHour);
  currentHour < 10 ? currentHour_str = '0' + currentHour_str : currentHour_str;
  
  String currentMin_str  = String(currentMin);
  currentMin < 10 ? currentMin_str = '0' + currentMin_str : currentMin_str;
  
  String currentSec_str  = String(currentSec);
  currentSec < 10 ? currentSec_str = '0' + currentSec_str : currentSec_str;   

  myCurrentTime = "";
  myCurrentTime += currentHour_str;
  myCurrentTime += ":";
  myCurrentTime += currentMin_str;
  myCurrentTime += ":";
  myCurrentTime += currentSec_str;
  
  return myCurrentTime;
}

// check done, further verify 
void checkMinFn()
{
  if (!(currentSec == 0)) return;
  for (int i = 1; i < 11; i += 2)
  {
    if (currentMin == arraySch[i])
    {
      Serial.print("min is same at: ");
      Serial.println(arraySch[i]);
      if (currentHour == arraySch[i-1])
      {
        Serial.print("Hour is same at: ");
        Serial.println(arraySch[i-1]);
        lastTurn = dispenseFn(turns_val, lastTurn);
      }
    }
  }
  return;
}
