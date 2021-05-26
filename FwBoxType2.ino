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


#define DEVICE_TYPE 2
#define FIRMWARE_VERSION "1.1.4"

//
// Debug definitions
//
#define FW_BOX_DEBUG 0

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
  Serial.begin(115200);

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
