# STM32 BMP180 Driver
STM32 driver code to read pressure and temperature data from the BMP180 sensor. Tested on STM32F1

## Contents
1. [Introduction](#introduction)
2. [Installation](#installation)
3. [Usage](#usage)
4. [Examples](#examples)
5. [Struct and Enum](#struct-and-enum)
6. [Functions](#functions)
7. [Other functions](#other-functions)
8. [Note](#note)



## Introduction
BMP180 is a low-cost highly accurate barometric pressure/temperature sensor. It offers a pressure data with 4 levels of accuracy you can choose from and a temperature data.

This driver gives full control of the sensor **eg.(read temp(Celcius), read pressure(Pascals), read elevation(Meters)**. The rest of this documentation goes over the installation process and the usage of the driver with STM32.

## Installation
1. Copy `BMP180.h` to `./Core/Inc` and `BMP180.c` to `./Core/Src`.
2. In `BMP180.h`, Change the `#include "stm32f1xx_hal.h"` to your STM32 model. (Disclaimer: Only tested on STM32F1)
3. Include `BMP180.h` in `main.c`.

## Usage
1. Create a `BMP180` struct.
2. Run `BMP180_Init()`.
3. Run anything else.

## Examples
### Initializing
```c
// Create struct
BMP180 bmp180;
// Run intialization function
BMP180_Init(&bmp180, &hi2c1);
```
### Reading Temperature
Simply call `BMP180_ReadTemp()` with the address to the BMP180 struct as its argument.
Then you can access `temp_C` as a member of the BMP180 struct.
```c
while (1) {
	// Read data
	BMP180_ReadPressure(&bmp180, CTRL_REG_PRESSURE_OSS_1);
	doSomething(bmp180->elevation_M);
	HAL_Delay(250);
}
```
### Reading Pressure/Elevation
Call `BMP180_ReadPressure()` with the address to the BMP180 struct as its first argument. For its second argument choose the accuracy of the result using `CTRL_REG_PRESSURE_OSS_X`.
Then you can access `pressure_Pa` and `elevation_M` as a member of the BMP180 struct.
```c
while (1) {
	// Read data
	BMP180_ReadPressure(&bmp180, CTRL_REG_PRESSURE_OSS_0); // ultra low power
	BMP180_ReadPressure(&bmp180, CTRL_REG_PRESSURE_OSS_1); // standard
	BMP180_ReadPressure(&bmp180, CTRL_REG_PRESSURE_OSS_2); // high res
	BMP180_ReadPressure(&bmp180, CTRL_REG_PRESSURE_OSS_3); // ultra high res
	doSomething(bmp180->pressure_Pa);
	doSomething(bmp180->elevation_M);
	HAL_Delay(250);
}
```
## Struct and Enum
### `BMP180`
```c
typedef  struct {
	// I2c handle
	I2C_HandleTypeDef  *hi2c;
	// param
	HAL_StatusTypeDef  device_id_status;
	int  OSS;
	int32_t  B5;
	// Raw data
	uint32_t  raw_pressure, raw_temperature;
	// Processed data;
	uint32_t  pressure_Pa;
	float  temp_C;
	float  elevation_M;
	// Calibration data struct
	CALIBRATION_DATA  calibration_data;
} BMP180;
```
### `CALIBRATION_DATA`
```c
typedef  struct {
	int16_t  AC1;
	int16_t  AC2;
	int16_t  AC3;
	uint16_t  AC4;
	uint16_t  AC5;
	uint16_t  AC6;
	int16_t  B1;
	int16_t  B2;
	int16_t  MB;
	int16_t  MC;
	int16_t  MD;
} CALIBRATION_DATA;
```
This is used to store the calibration data of the BMP180 in the Initialization function. It can be accessed from `BMP180->calibration_data`.
### `CONTROL_REGISTER`
```c
typedef  enum {
	CTRL_REG_TEMPERATURE  =  0x2E,
	CTRL_REG_PRESSURE_OSS_0  =  0x34, // ultra low power
	CTRL_REG_PRESSURE_OSS_1  =  0x74, // standard
	CTRL_REG_PRESSURE_OSS_2  =  0xB4, // high res
	CTRL_REG_PRESSURE_OSS_3  =  0xF4, // ultra high res
} CONTROL_REGISTER;
```
Used to set the resolution of the pressure sensor. Use `CTRL_REG_PRESSURE_OSS_X` when calling `BMP180_ReadPressure()` or `BMP180_ReadRawPressure()`.

## Functions
### `BMP180_Init()`
```c
uint8_t  BMP180_Init(BMP180  *device, I2C_HandleTypeDef  *hi2c);
```
Initialization function. Takes in BMP180 struct and I2C_HandleTypeDef. Returns an error code.
```
ERROR CODES
1: OK
2: Failed to read device ID reg.
3: Device Id does not match.
4: Failed to read calibration data.
```
### `BMP180_ReadTemp()`
```c
HAL_StatusTypeDef  BMP180_ReadTemp(BMP180  *device);
```
Reads the raw temperature and saves it to `BMP180->raw_temperature`.
Reads the temperature in Celcius and saves it to `BMP180->temp_C`.
### `BMP180_ReadPressure()`
```c
HAL_StatusTypeDef  BMP180_ReadPressure(BMP180  *device, CONTROL_REGISTER  control_register);
```
Reads the raw pressure and saves it to `BMP180->raw_pressure`.
Reads the pressure in Pascals and saves it to `BMP180->pressure_Pa`. Also saves elevation to `BMP180->elevation_M` in meters.

Due to the conversion of raw pressures to pascals requiring temperature data, the raw temperature and temperature in Celcius gets saved into the BMP180 struct again. 

This means that if you need to both read temperature and pressure, you will be fine with just calling `BMP180_ReadPressure()` since it updates all temperature, pressure, and elevation.
### `BMP180_ReadRawTemp()`
```c
HAL_StatusTypeDef  BMP180_ReadRawTemp(BMP180  *device);
```
Reads the raw temperature and saves it to `BMP180->raw_temperature`.
### `BMP180_ReadRawPressure()`
```c
HAL_StatusTypeDef  BMP180_ReadRawPressure(BMP180  *device, CONTROL_REGISTER  control_register);
```
Reads the raw pressure and saves it to `BMP180->raw_pressure`.
## Other functions
### `BMP180_Read_Calibration_Data()`
```c
HAL_StatusTypeDef  BMP180_Read_Calibration_Data(BMP180  *device);
```
### `BMP180_SET_CTRL_MEAS()`
```c
HAL_StatusTypeDef  BMP180_SET_CTRL_MEAS(BMP180  *device, CONTROL_REGISTER  control_register);
```
### `BMP180_ReadReg()`
```c
HAL_StatusTypeDef  BMP180_ReadReg(BMP180  *device, uint8_t  reg_addr, uint8_t  *data);
```
Reads a single register.
### `BMP180_ReadRegs()`
```c
HAL_StatusTypeDef  BMP180_ReadRegs(BMP180  *device, uint8_t  reg_addr, uint8_t  *data, uint8_t  size);
```
### `BMP180_WriteReg()`
```c
HAL_StatusTypeDef  BMP180_WriteReg(BMP180  *device, uint8_t  reg_addr, uint8_t  *data);
```
## Note
If some of the function descriptions weren't too clear, please take a look at the comments in `BMP180.h`. 
If there are any questoins, bug reports, or suggestions, please contact me at hyunwoo6@illinois.edu	
