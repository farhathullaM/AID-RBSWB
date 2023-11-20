#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
/* ======================================== Variables for network. */
// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "";
const char* password = "";
String BOTtoken = "";  
String CHAT_ID = "";

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define ON HIGH
#define OFF LOW
#define FLASH_LED_PIN   4          
#define PIR_SENSOR_PIN  12          
#define EEPROM_SIZE     2        

// Checks for new messages every 1 second (1000 ms).
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;
unsigned long lastTime_countdown_Ran;
byte countdown_to_stabilize_PIR_Sensor = 30;
bool sendPhoto = false;             
bool boolPIRState = false;

void FB_MSG_is_photo_send_successfully (bool state) {
  String send_feedback_message = "";
  if(state == false) {
    bot.sendMessage(CHAT_ID, send_feedback_message, "");
  } else {
    if(boolPIRState == true) {
      Serial.println("Successfully sent photo.");
      bot.sendMessage(CHAT_ID, send_feedback_message, "");
    }
    if(sendPhoto == true) {
      Serial.println("Successfully sent photo.");
      bot.sendMessage(CHAT_ID, send_feedback_message, "");
    }
  }
}

bool PIR_State() {
  bool PRS = digitalRead(PIR_SENSOR_PIN);
  return PRS;
}

void LEDFlash_State(bool ledState) {
  digitalWrite(FLASH_LED_PIN, ledState);
}

void enable_capture_Photo_With_Flash(bool state) {
  EEPROM.write(0, state);
  EEPROM.commit();
  delay(50);
}

bool capture_Photo_With_Flash_state() {
  bool capture_Photo_With_Flash = EEPROM.read(0);
  return capture_Photo_With_Flash;
}
/* ________________________________________________________________________________ */


/* ________________________________________________________________________________ Subroutine for setting and saving settings in EEPROM for "capture photos with PIR Sensor" mode. */
void enable_capture_Photo_with_PIR(bool state) {
  EEPROM.write(1, state);
  EEPROM.commit();
  delay(50);
}
/* ________________________________________________________________________________ */


/* ________________________________________________________________________________ Function to read settings in EEPROM for "capture photos with PIR Sensor" mode.*/
bool capture_Photo_with_PIR_state() {
  bool capture_Photo_with_PIR = EEPROM.read(1);
  return capture_Photo_with_PIR;
}
/* ________________________________________________________________________________ */


/* ________________________________________________________________________________ Subroutine for camera configuration. */
void configInitCamera(){
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;


  /* ---------------------------------------- init with high specs to pre-allocate larger buffers. */
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; //--> FRAMESIZE_ + UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA
    config.jpeg_quality = 10;  
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;  
    config.fb_count = 1;
  }
  /* ---------------------------------------- */


  /* ---------------------------------------- camera init. */
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    Serial.println();
    Serial.println("Restart ESP32 Cam");
    delay(1000);
    ESP.restart();
  }

  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_SXGA);  
}

String sendPhotoTelegram() {
  const char* myDomain = "api.telegram.org";
  String getAll = "";
  String getBody = "";

  Serial.println("Taking a photo...");

  if(capture_Photo_With_Flash_state() == ON) {
    LEDFlash_State(ON);
  }
  delay(1500);

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    Serial.println("Restart ESP32 Cam");
    delay(1000);
    ESP.restart();
    return "Camera capture failed";
  }  
  /* ::::::::::::::::: */


  /* ::::::::::::::::: Turn off the LED Flash after successfully taking photos. */
  if(capture_Photo_With_Flash_state() == ON) {
    LEDFlash_State(OFF);
  }
  /* ::::::::::::::::: */
  Serial.println("Successful photo taking.");
  /* ---------------------------------------- */
 


  /* ---------------------------------------- The process of sending photos. */
  Serial.println("Connect to " + String(myDomain));


  if (clientTCP.connect(myDomain, 443)) {
    Serial.println("Connection successful");
    Serial.print("Send photos");
   
    String head = "--Esp32Cam\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n";
    head += CHAT_ID;
    head += "\r\n--Esp32Cam\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--Esp32Cam--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;
 
    clientTCP.println("POST /bot"+BOTtoken+"/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=Esp32Cam");
    clientTCP.println();
    clientTCP.print(head);
 
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
   
    for (size_t n=0;n<fbLen;n=n+1024) {
      if (n+1024<fbLen) {
        clientTCP.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        clientTCP.write(fbBuf, remainder);
      }
    }  
   
    clientTCP.print(tail);
   
    esp_camera_fb_return(fb);
   
    int waitTime = 10000;   //--> timeout 10 seconds (To send photos.)
    long startTimer = millis();
    boolean state = false;

}
/* ________________________________________________________________________________ VOID SETTUP() */
void setup(){
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //--> Disable brownout detector.


   /* ---------------------------------------- Init serial communication speed (baud rate). */
  Serial.begin(115200);
  delay(1000);
  /* ---------------------------------------- */


  Serial.println();
  Serial.println();
  Serial.println("------------");


  /* ---------------------------------------- Starts the EEPROM, writes and reads the settings stored in the EEPROM. */
  EEPROM.begin(EEPROM_SIZE);


  Serial.println("Setting status :");
  if(capture_Photo_With_Flash_state() == ON) {
    Serial.println("- Capture Photo With Flash = ON");
  }
  if(capture_Photo_With_Flash_state() == OFF) {
    Serial.println("- Capture Photo With Flash = OFF");
  }
  if(capture_Photo_with_PIR_state() == ON) {
    Serial.println("- Capture Photo With PIR = ON");
    botRequestDelay = 20000;
  }
  if(capture_Photo_with_PIR_state() == OFF) {
    Serial.println("- Capture Photo With PIR = OFF");
    botRequestDelay = 1000;
  }
  /* ---------------------------------------- */


  /* ---------------------------------------- Set LED Flash as output and make the initial state of the LED Flash is off. */
  pinMode(FLASH_LED_PIN, OUTPUT);
  LEDFlash_State(OFF);
  /* ---------------------------------------- */


  /* ---------------------------------------- Config and init the camera. */
  Serial.println();
  Serial.println("Start configuring and initializing the camera...");
  configInitCamera();
  Serial.println("Successfully configure and initialize the camera.");
  Serial.println();
  /* ---------------------------------------- */


  /* ---------------------------------------- Connect to Wi-Fi. */
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT); //--> Add root certificate for api.telegram.org

  int connecting_process_timed_out = 20; //--> 20 = 20 seconds.
  connecting_process_timed_out = connecting_process_timed_out * 2;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    LEDFlash_State(ON);
    delay(250);
    LEDFlash_State(OFF);
    delay(250);
    if(connecting_process_timed_out > 0) connecting_process_timed_out--;
    if(connecting_process_timed_out == 0) {
      delay(1000);
      ESP.restart();
    }
  }
  /* ::::::::::::::::: */
 
  LEDFlash_State(OFF);
  Serial.println();
  Serial.print("Successfully connected to ");
  Serial.println(ssid);
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());
  // Serial.println();
  // Serial.println("The PIR sensor is being stabilized.");
  // Serial.printf("Stabilization time is %d seconds away. Please wait.\n", countdown_to_stabilize_PIR_Sensor);
 
  Serial.println("------------");
  Serial.println();
  /* ---------------------------------------- */
}



void loop() {
  /* ---------------------------------------- Conditions for taking and sending photos. */
  if(sendPhoto) {
    Serial.println("Preparing photo...");
    sendPhotoTelegram();
    sendPhoto = false;
  }

  if(millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      Serial.println();
      Serial.println("------------");
      Serial.println("got response");
      //handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }

  if(capture_Photo_with_PIR_state() == ON) {
    if(PIR_State() == true) {
      Serial.println("------------");
      Serial.println("The PIR sensor detects objects and movements.");
      boolPIRState = true;
      sendPhotoTelegram();
      boolPIRState = false;
    }
  }
}

