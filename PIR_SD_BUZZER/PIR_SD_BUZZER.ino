#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <EEPROM.h>            // read and write from flash memory
#include "ESP32_MailClient.h"
#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include "SPI.h"

// define the number of bytes you want to access
#define EEPROM_SIZE 1
// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

// To send Email using Gmail use port 465 (SSL) and SMTP Server smtp.gmail.com
// You need to create an email app password
#define emailSenderAccount    "SENDER_EMAIL@gmail.com"
#define emailSenderPassword   "YOUR_EMAIL_APP_PASSWORD"
#define smtpServer            "smtp.gmail.com"
#define smtpServerPort        465
#define emailSubject          "ESP32-CAM Photo Captured"
#define emailRecipient        "YOUR_EMAIL_RECIPIENT@example.com"

int pictureNumber = 0;

const int pirSensorPin = 13; // Digital pin to which the PIR sensor is connected

/* The SMTP Session object used for Email sending */
SMTPSession smtp;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  pinMode(pirSensorPin, INPUT); // Set the PIR sensor pin as an input
  Serial.begin(115200); // Initialize serial communication at 115200 bps

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  // Configure camera settings here

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  Serial.println("Starting SD Card");
 
  delay(500);
  if(!SD_MMC.begin()){
    Serial.println("SD Card Mount Failed");
    //return;
  }
 
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD Card attached");
    return;
  }
}

void loop() {
    int motionDetected = digitalRead(pirSensorPin); // Read the state of the PIR sensor

    if (motionDetected == HIGH) {
      Serial.println("Motion detected");
      // Capture a photo
      camera_fb_t *fb = esp_camera_fb_get();
      if (fb) {
        // Save the captured image or perform further processing
        // Example: Saving the image to the SD card
        savePhotoToSDCard(fb);
        
        esp_camera_fb_return(fb); // Return the frame buffer
      }

      delay(5000);  // Delay to avoid rapid consecutive captures
    }
}


  void savePhotoToSDCard(camera_fb_t *fb) {
    // Code to save the captured photo to an SD card
    // Replace this with your actual saving logic
    // Example: You can use the SD_MMC library for saving to an SD card
      // Check if SD card is available
        if(!fb) {
      Serial.println("Camera capture failed");
      return;
    }
    // initialize EEPROM with predefined size
    EEPROM.begin(EEPROM_SIZE);
    pictureNumber = EEPROM.read(0) + 1;
  
    // Path where new picture will be saved in SD Card
    String path = "/picture" + String(pictureNumber) +".jpg";
  
    fs::FS &fs = SD_MMC;
    Serial.printf("Picture file name: %s\n", path.c_str());
  
    File file = fs.open(path.c_str(), FILE_WRITE);
    if(!file){
      Serial.println("Failed to open file in writing mode");
    }
    else {
      file.write(fb->buf, fb->len); // payload (image), payload length
      Serial.printf("Saved file to path: %s\n", path.c_str());
      EEPROM.write(0, pictureNumber);
      EEPROM.commit();
    }
    file.close();
    esp_camera_fb_return(fb);
    
    delay(1000);
    
    // Turns off the ESP32-CAM white on-board LED (flash) connected to GPIO 4
    pinMode(4, OUTPUT);
    digitalWrite(4, LOW);
    rtc_gpio_hold_en(GPIO_NUM_4);

    esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0);
  
    Serial.println("Going to sleep now");
    delay(1000);
    esp_deep_sleep_start();
    Serial.println("This will never be printed");
  }

    // Create a file on the SD card with a unique name
   
