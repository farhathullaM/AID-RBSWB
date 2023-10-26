/*********
  Rui Santos
  Complete instructions at https://RandomNerdTutorials.com/esp32-cam-projects-ebook/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

#include "esp_camera.h"
#include "SPI.h"
#include "driver/rtc_io.h"
#include <ESP_Mail_Client.h>
#include <FS.h>
#include <WiFi.h>

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "Samsung M10";
const char* password = "00000001";

// To send Email using Gmail use port 465 (SSL) and SMTP Server smtp.gmail.com
// You need to create an email app password
#define emailSenderAccount    "popularedk@gmail.com"
#define emailSenderPassword   "jokitgkdrgttmkok"
#define smtpServer            "smtp.gmail.com"
#define smtpServerPort        465
#define emailSubject          "ESP32-CAM Photo Captured"
#define emailRecipient        "farhu123123456@gmail.com"


#define CAMERA_MODEL_AI_THINKER

// PIR Sensor input pin
const int pirPin = 14; // You can change this pin as needed

int motionDetected = 0;

#if defined(CAMERA_MODEL_AI_THINKER)
// ... (Pin definitions for camera)
#else
#error "Camera model not selected"
#endif

/* The SMTP Session object used for Email sending */
SMTPSession smtp;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

// Photo File Name to save in LittleFS
#define FILE_PHOTO "photo.jpg"
#define FILE_PHOTO_PATH "/photo.jpg"

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  
  Serial.begin(115200);
  Serial.println();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  // Print ESP32 Local IP Address
  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());

  // Init filesystem
  ESP_MAIL_DEFAULT_FLASH_FS.begin();
   
  camera_config_t config;
  // ... (Camera configuration)

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  
  pinMode(pirPin, INPUT);
}

void loop() {
  int motion = digitalRead(pirPin);
  if (motion == HIGH && motionDetected == 0) {
    // Motion detected, take a photo
    capturePhotoSaveLittleFS();
    sendPhoto();
    motionDetected = 1;
  } else if (motion == LOW) {
    motionDetected = 0;
  }
  delay(1000); // Adjust delay as needed
}

void capturePhotoSaveLittleFS() {
  // Capture and save a photo to LittleFS
  camera_fb_t* fb = NULL;
  // Skip first 3 frames (increase/decrease number as needed).
  for (int i = 0; i < 3; i++) {
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = NULL;
  }

  // Take a new photo
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Photo file name
  Serial.printf("Picture file name: %s\n", FILE_PHOTO_PATH);
  File file = LittleFS.open(FILE_PHOTO_PATH, FILE_WRITE);

  // Insert the data in the photo file
  if (!file) {
    Serial.println("Failed to open file in writing mode");
  } else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.print("The picture has been saved in ");
    Serial.print(FILE_PHOTO_PATH);
    Serial.print(" - Size: ");
    Serial.print(fb->len);
    Serial.println(" bytes");
  }
  // Close the file
  file.close();
  esp_camera_fb_return(fb);
}

void sendPhoto() {
  // Create an instance of the SMTP_Message class for the email
  SMTP_Message message;

  // Set the sender's name and email address
  message.sender.name = "ESP32-CAM";
  message.sender.email = emailSenderAccount;

  // Set the subject of the email
  message.subject = emailSubject;

  // Add a recipient with name and email address
  message.addRecipient("Recipient Name", emailRecipient);

  // Set the HTML content of the email (you can customize this)
  String htmlMsg = "<h2>Photo captured with ESP32-CAM and attached in this email.</h2>";
  message.html.content = htmlMsg.c_str();
  message.html.charSet = "utf-8";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_qp;

  // The attachment data item
  SMTP_Attachment att;

  // Set the attachment info
  att.descr.filename = FILE_PHOTO;
  att.descr.mime = "image/jpeg"; 
  att.file.path = FILE_PHOTO_PATH;
  att.file.storage_type = esp_mail_file_storage_type_flash;
  att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  // Add the attachment to the message
  message.addAttachment(att);

  // Create a Session_Config object for email configuration
  Session_Config config;

  // Set NTP server and time zone
  config.time.ntp_server = "pool.ntp.org,time.nist.gov";
  config.time.gmt_offset = 0;
  config.time.day_light_offset = 1;

  // Set SMTP server and login details
  config.server.host_name = smtpServer;
  config.server.port = smtpServerPort;
  config.login.email = emailSenderAccount;
  config.login.password = emailSenderPassword;
  config.login.user_domain = "";

  // Enable chunked data transfer with pipelining
  message.enable.chunking = true;

  // Set email priority and notification
  message.priority = esp_mail_smtp_priority_normal;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  // Create an instance of the SMTPSession
  SMTPSession smtp;

  // Set the debug level to 1 for basic debugging
  smtp.debug(1);

  // Set the callback function to handle email sending status
  smtp.callback(smtpCallback);

  // Connect to the SMTP server
  if (smtp.connect(&config)) {
    // Start sending the email and close the session
    if (!MailClient.sendMail(&smtp, &message, true)) {
      Serial.println("Error sending Email, " + smtp.errorReason());
    }
  }
}

void smtpCallback(SMTP_Status status) {
  Serial.print("SMTP Status: ");
  Serial.println(status.info());

  if (status.info() == "Email sent") {
    // Email was sent successfully, you can add your own logic here
  }
  // You can add more conditions to handle other SMTP statuses as needed
}

