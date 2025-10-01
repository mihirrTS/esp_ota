#ifndef _DISPLAY_EPD_W21_SPI_S3_
#define _DISPLAY_EPD_W21_SPI_S3_
#include "Arduino.h"

// ESP32-S3 N8R8 Pin definitions
// Using safe GPIO pins that don't conflict with internal flash/PSRAM
#define RST_PIN   4    // GPIO4 - Safe general purpose pin
#define DC_PIN    5    // GPIO5 - Safe general purpose pin
#define CS_PIN    6    // GPIO6 - Safe general purpose pin
#define BUSY_PIN  7    // GPIO7 - Safe general purpose pin
#define PWR_PIN   15   // GPIO15 - Safe general purpose pin
#define MOSI_PIN  11   // GPIO11 - SPI MOSI (default ESP32-S3 SPI)
#define SCK_PIN   12   // GPIO12 - SPI SCK (default ESP32-S3 SPI)

//IO settings for ESP32-S3
#define isEPD_W21_BUSY digitalRead(BUSY_PIN)  //BUSY
#define EPD_W21_RST_0 digitalWrite(RST_PIN,LOW)  //RES
#define EPD_W21_RST_1 digitalWrite(RST_PIN,HIGH)
#define EPD_W21_DC_0  digitalWrite(DC_PIN,LOW) //DC
#define EPD_W21_DC_1  digitalWrite(DC_PIN,HIGH)
#define EPD_W21_CS_0 digitalWrite(CS_PIN,LOW) //CS
#define EPD_W21_CS_1 digitalWrite(CS_PIN,HIGH)
#define EPD_W21_PWR_0 digitalWrite(PWR_PIN,LOW) //PWR
#define EPD_W21_PWR_1 digitalWrite(PWR_PIN,HIGH)

void SPI_Write(unsigned char value);
void EPD_W21_WriteDATA(unsigned char datas);
void EPD_W21_WriteCMD(unsigned char command);

#endif