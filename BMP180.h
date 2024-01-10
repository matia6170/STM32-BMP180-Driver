/*
 * ==================================================
 *   BMP180.h - Library for BMP180 pressure sensor.
 *   Created by Matia Choi, 01/08/2024
 *   v0.1
 * ==================================================
 */
#ifndef BMP180_H
#define BMP180_H

// for I2C
#include "stm32f1xx_hal.h"

/* ===========
 *   DEFINES
 * ===========
 */
#define BMP180_I2C_ADDR 0xEE  // 0x77 << 1
#define BMP180_DEVICE_ID 0x55

#define PRESSURE_SEA_LEVEL_PA 101325

/* =============
 *   REGISTERS
 * =============
 */
#define BMP180_REG_OUT_XLSB 0xF8
#define BMP180_REG_OUT_LSB 0xF7
#define BMP180_REG_OUT_MSB 0xF6
#define BMP180_REG_CTRL_MEAS 0xF4
#define BMP180_REG_SOFT_RESET 0xE0
#define BMP180_REG_ID 0xD0
#define BMP180_REG_CALIB_21 0xBF
#define BMP180_REG_CALIB_0 0xAA

#define BMP180_CTRL_MEAS_TEMP 0x2E

/* =================
 *   Sensor struct
 * =================
 */
typedef struct {
    int16_t AC1;
    int16_t AC2;
    int16_t AC3;
    uint16_t AC4;
    uint16_t AC5;
    uint16_t AC6;
    int16_t B1;
    int16_t B2;
    int16_t MB;
    int16_t MC;
    int16_t MD;
} CALIBRATION_DATA;

typedef struct {
    // I2c handle
    I2C_HandleTypeDef *hi2c;

    // param
    HAL_StatusTypeDef device_id_status;
    int OSS;
    int32_t B5;

    // Raw data
    uint32_t raw_pressure, raw_temperature;

    // Processed data;
    uint32_t pressure_Pa;
    float temp_C;
    float elevation_M;

    // Calibration data struct
    CALIBRATION_DATA calibration_data;

} BMP180;

/* =========
 *   enum
 * =========
 */

typedef enum {
    CTRL_REG_TEMPERATURE = 0x2E,
    CTRL_REG_PRESSURE_OSS_0 = 0x34,  // ultra low power
    CTRL_REG_PRESSURE_OSS_1 = 0x74,  // standard
    CTRL_REG_PRESSURE_OSS_2 = 0xB4,  // high res
    CTRL_REG_PRESSURE_OSS_3 = 0xF4,  // ultra high res
} CONTROL_REGISTER;

/* ==================
 *   Initialization
 * ==================
 */

/*
 * ERROR CODES
 * 1: OK
 * 2: Failed to read device ID reg.
 * 3: Device Id does not match.
 * 4: Failed to read calibration data.
 */
// Initialize BMP180.
uint8_t BMP180_Init(BMP180 *device, I2C_HandleTypeDef *hi2c);
// Read the calibration datas from the BMP180 EEPROM.
HAL_StatusTypeDef BMP180_Read_Calibration_Data(BMP180 *device);

/* ==================
 *   Set Parameters
 * ==================
 */

// Set the measurement control.
HAL_StatusTypeDef BMP180_SET_CTRL_MEAS(BMP180 *device, CONTROL_REGISTER control_register);

/* =============
 *   Read Data
 * =============
 */

// Read the raw temperature data.
HAL_StatusTypeDef BMP180_ReadRawTemp(BMP180 *device);
// Read the raw pressure data.
HAL_StatusTypeDef BMP180_ReadRawPressure(BMP180 *device, CONTROL_REGISTER control_register);
// Read the temperature in Celcius.
HAL_StatusTypeDef BMP180_ReadTemp(BMP180 *device);
// Read the pressure in Pascals.
HAL_StatusTypeDef BMP180_ReadPressure(BMP180 *device, CONTROL_REGISTER control_register);
// Read a single register.
HAL_StatusTypeDef BMP180_ReadReg(BMP180 *device, uint8_t reg_addr, uint8_t *data);
// Read multiple registers.
HAL_StatusTypeDef BMP180_ReadRegs(BMP180 *device, uint8_t reg_addr, uint8_t *data, uint8_t size);
// Write to a register.
HAL_StatusTypeDef BMP180_WriteReg(BMP180 *device, uint8_t reg_addr, uint8_t *data);

#endif