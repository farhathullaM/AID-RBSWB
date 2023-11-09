#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <EEPROM.h>            // read and write from flash memory
#include "ESP32_MailClient.h"
// define the number of bytes you want to access
#define EEPROM_SIZE 1


const int pirSensorPin = 13; // Digital pin to which the PIR sensor is connected

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

    delay(10000);  // Delay to avoid rapid consecutive captures
  }
}


void savePhotoToSDCard(camera_fb_t *fb) {
  // Code to save the captured photo to an SD card
  // Replace this with your actual saving logic
  // Example: You can use the SD_MMC library for saving to an SD card
    // Check if SD card is available
  if(!SD_MMC.begin()){
    Serial.println("SD Card Mount Failed");
    return;
  }

  // Create a file on the SD card with a unique name
  String fileName = "/photo" + String(millis()) + ".jpg";
  File file = SD_MMC.open(fileName.c_str(), FILE_WRITE);

  if(!file){
    Serial.println("Failed to create file on SD card");
    return;
  }

  // Write the image data to the file
  file.write(fb->buf, fb->len);

  // Close the file
  file.close();

  Serial.println("Photo saved to SD card: " + fileName);

}
