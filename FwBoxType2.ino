//
// Copyright (c) 2021 Fw-Box (https://fw-box.com)
// Author: Hartman Hsieh
//
// Description :
//   None
//
// Connections :
//
// Required Library :
//   https://github.com/Risele/SHT3x
//   https://github.com/claws/BH1750
//

#include "FwBox.h"
#include <SHT3x.h>
#include <BH1750.h> // Light Sensor (BH1750)
#include "FwBox_UnifiedLcd.h"
#include <U8g2lib.h>


#define DEVICE_TYPE 2
#define FIRMWARE_VERSION "1.1"

//
// Debug definitions
//
#define FW_BOX_DEBUG 1

#if FW_BOX_DEBUG == 1
  #define DBG_PRINT(VAL) Serial.print(VAL)
  #define DBG_PRINTLN(VAL) Serial.println(VAL)
  #define DBG_PRINTF(FORMAT, ARG) Serial.printf(FORMAT, ARG)
#else
  #define DBG_PRINT(VAL)
  #define DBG_PRINTLN(VAL)
  #define DBG_PRINTF(FORMAT, ARG)
#endif

//
// Global variable
//

//
// LCD 1602
//
FwBox_UnifiedLcd* UnifiedLcd = 0;

//
// OLED 128x128
//
U8G2_SSD1327_MIDAS_128X128_1_HW_I2C* u8g2 = 0;

//
// SHT3x
//
SHT3x SensorSht;

//
// Light Sensor (BH1750)
//
BH1750 SensorLight;

//
// The sensor's values
//
float HumidityValue = 0.0;
float TemperatureValue = 0.0;
float LightValue = 0.0;

unsigned long ReadingTime = 0;

void setup()
{
  Wire.begin();  // Join IIC bus for Light Sensor (BH1750).
  Serial.begin(9600);

  //
  // Initialize the fw-box core (early stage)
  //
  fbEarlyBegin(DEVICE_TYPE, FIRMWARE_VERSION);

  //
  // Initialize the LCD1602
  //
  UnifiedLcd = new FwBox_UnifiedLcd(16, 2);
  if(UnifiedLcd->begin() != 0) {
    //
    // LCD1602 doesn't exist, delete it.
    //
    delete UnifiedLcd;
    UnifiedLcd = 0;
    DBG_PRINTLN("LCD1602 initialization failed.");
  }

  //
  // Detect the I2C address of OLED.
  //
  Wire.beginTransmission(0x78>>1);
  uint8_t data8 = Wire.endTransmission();
  if (data8 == 0) {
    //
    // Initialize the OLED
    //
    u8g2 = new U8G2_SSD1327_MIDAS_128X128_1_HW_I2C(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);  /* Uno: A4=SDA, A5=SCL, add "u8g2.setBusClock(400000);" into setup() for speedup if possible */
    u8g2->begin();
    u8g2->enableUTF8Print();
    u8g2->setFont(u8g2_font_unifont_t_chinese1);  // use chinese2 for all the glyphs of "你好世界"
  }
  else {
    DBG_PRINTLN("U8G2_SSD1327_MIDAS_128X128_1_HW_I2C is not found.");
    u8g2 = 0;
  }

  //
  // Display the sensors
  //
  display();

  //
  // Initialize the fw-box core
  //
  fbBegin(DEVICE_TYPE, FIRMWARE_VERSION);

  //
  // Initialize the SHT Sensor
  //
  SensorSht.Begin();

  //
  // Initialize the Light Sensor
  //
  SensorLight.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

  //
  // Update the reading time.
  //
  ReadingTime = millis();

} // void setup()

void loop()
{
  if((ReadingTime == 0) || ((millis() - ReadingTime) > 2000)) {
    DBG_PRINT("Device UUID is ");
    DBG_PRINTLN(FwBoxIns.getDeviceConfig()->Uuid);
    DBG_PRINT("Device Type is ");
    DBG_PRINTLN(FwBoxIns.getDeviceConfig()->Type);

    //
    // Read sensors
    //
    read();
  
    //
    // Display sensors
    //
    display();

    //
    // Check if any reads failed.
    //
    if(isnan(HumidityValue) || isnan(TemperatureValue)) {
    }
    else {
      //
      // Filter the wrong values.
      //
      if( (TemperatureValue > 1) &&
          (TemperatureValue < 70) && 
          (HumidityValue > 10) &&
          (HumidityValue < 95) ) {
  
        FwBoxIns.setValue(0, TemperatureValue);
        FwBoxIns.setValue(1, HumidityValue);
      }
    }

    if(LightValue > 0) {
      FwBoxIns.setValue(2, LightValue);
    }

    ReadingTime = millis();
  } // END OF "if((ReadingTime == 0) || ((millis() - ReadingTime) > 2000))"

  //
  // Run the handle
  //
  fbHandle();

} // END OF "void loop()"

void read()
{
  //
  // Update the values of SHT3X
  //
  SensorSht.UpdateData();

  //
  // Read humidity(Unit:%)
  //
  HumidityValue = SensorSht.GetRelHumidity();
  DBG_PRINTF("Humidity : %f %%\n", HumidityValue);

  //
  // Read temperature as Celsius (the default)
  //
  TemperatureValue = SensorSht.GetTemperature();
  DBG_PRINTF("Temperature : %f C\n", TemperatureValue);

  //
  // Read Light level (Unit:Lux)
  //
  LightValue = SensorLight.readLightLevel();
  if(LightValue > 0) {
    DBG_PRINTF("Light : %f LUX\n", LightValue);
  }
}

void display()
{
  //
  // Draw the LCD 1602
  //
  if(UnifiedLcd != 0) {
    char buff[32];

    memset(&(buff[0]), 0, 32);
    sprintf(buff, "%.2fC %.2f%%", TemperatureValue, HumidityValue);

    //
    // Center the string.
    //
    UnifiedLcd->printAtCenter(0, buff);

    memset(&(buff[0]), 0, 32);
    sprintf(buff, "%.2fLUX", LightValue);

    //
    // Center the string.
    //
    UnifiedLcd->printAtCenter(1, buff);
  }

  //
  // Draw the OLED
  //
  if(u8g2 != 0) {
    u8g2->setFont(u8g2_font_unifont_t_chinese2);
    u8g2->firstPage();
    do {
      String line0 = String(TemperatureValue) + " " + FwBoxIns.getValUnit(0);
      u8g2->setCursor(5, 20);
      u8g2->print(line0);

      String line1 = String(HumidityValue) + " " + FwBoxIns.getValUnit(1);
      u8g2->setCursor(5, 55);
      u8g2->print(line1);

      String line2 = String(LightValue) + " " + FwBoxIns.getValUnit(2);
      u8g2->setCursor(5, 90);
      u8g2->print(line2);
    } while ( u8g2->nextPage() );
  }
}
