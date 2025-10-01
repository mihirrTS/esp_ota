#ifndef _DISPLAY_EPD_W21_SPI_
#define _DISPLAY_EPD_W21_SPI_
#include "Arduino.h"

// ESP32 DevKit V1 Pin definitions (Default configuration)
#define RST_PIN   25   // GPIO25 - Reset
#define DC_PIN    26   // GPIO26 - Data/Command
#define CS_PIN    5    // GPIO5 - Chip Select
#define BUSY_PIN  27   // GPIO27 - Busy Status
#define PWR_PIN   33   // GPIO33 - Power Control
#define MOSI_PIN  23   // GPIO23 - SPI Data
#define SCK_PIN   18   // GPIO18 - SPI Clock

//IO settings for ESP32
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