/*
 * ==================================================
 *   BMP180.c - Library for BMP180 pressure sensor.
 *   Created by Matia Choi, 01/08/2024
 *   v0.1
 * ==================================================
 */

#include "BMP180.h"

#include "math.h"

uint8_t BMP180_Init(BMP180 *device, I2C_HandleTypeDef *hi2c) {
    device->hi2c = hi2c;
    device->temp_C = 0.0f;

    uint8_t errNum = 0;
    uint8_t regData;

    // load device id.
    HAL_StatusTypeDef deviceIdStatus = BMP180_ReadReg(device, BMP180_REG_ID, &regData);
    device->device_id_status = deviceIdStatus;
    if (deviceIdStatus != HAL_OK) return 2;

    // Check device id
    if (regData != BMP180_DEVICE_ID) return 3;

    // load calibration data
    if (BMP180_Read_Calibration_Data(device) != HAL_OK) return 4;

    // OK
    return 1;
}

HAL_StatusTypeDef BMP180_Read_Calibration_Data(BMP180 *device) {
    // Read the calibratoin data registers
    uint8_t rawCalibratoinData[22];
    HAL_StatusTypeDef status = BMP180_ReadRegs(device, BMP180_REG_CALIB_0, rawCalibratoinData, 22);
    if (status != HAL_OK) return status;

    // Pointer to the calibration data struct
    CALIBRATION_DATA *calibration_data = &(device->calibration_data);
    int16_t *ptr = (int16_t *)calibration_data;

    // Iterate through the struct using pointer arithmetic
    for (int i = 0; i < 11; i++) {
        // Set the calibration data.
        *(ptr + i) = (rawCalibratoinData[2 * i] << 8) | rawCalibratoinData[2 * i + 1];
    }

    return status;
}

HAL_StatusTypeDef BMP180_SET_CTRL_MEAS(BMP180 *device, CONTROL_REGISTER control_register) {
    return BMP180_WriteReg(device, BMP180_REG_CTRL_MEAS, &control_register);
}

HAL_StatusTypeDef BMP180_ReadRawTemp(BMP180 *device) {
    uint16_t UT;
    uint8_t MSB, LSB;

    HAL_StatusTypeDef status;

    // Request for temperature data
    BMP180_SET_CTRL_MEAS(device, CTRL_REG_TEMPERATURE);
    HAL_Delay(5);

    // Read data
    status = BMP180_ReadReg(device, BMP180_REG_OUT_MSB, &MSB);
    if (status != HAL_OK) return status;
    status = BMP180_ReadReg(device, BMP180_REG_OUT_LSB, &LSB);
    if (status != HAL_OK) return status;

    UT = (MSB << 8) | LSB;

    // Save data
    device->raw_temperature = UT;

    return status;
}

HAL_StatusTypeDef BMP180_ReadRawPressure(BMP180 *device, CONTROL_REGISTER control_register) {
    uint32_t UP;
    uint8_t XLSB, LSB, MSB;
    HAL_StatusTypeDef status;

    int OSS = device->OSS;

    // Request for pressure data
    BMP180_SET_CTRL_MEAS(device, control_register);

    // Wait and set OSS accordingly
    if (control_register == CTRL_REG_PRESSURE_OSS_0) {
        OSS = 0;
        HAL_Delay(5);
    } else if (control_register == CTRL_REG_PRESSURE_OSS_1) {
        OSS = 1;
        HAL_Delay(8);
    } else if (control_register == CTRL_REG_PRESSURE_OSS_2) {
        OSS = 2;
        HAL_Delay(14);
    } else if (control_register == CTRL_REG_PRESSURE_OSS_3) {
        OSS = 3;
        HAL_Delay(26);
    }

    // Read data from 3 registers
    status = BMP180_ReadReg(device, BMP180_REG_OUT_MSB, &MSB);
    if (status != HAL_OK) return status;
    status = BMP180_ReadReg(device, BMP180_REG_OUT_LSB, &LSB);
    if (status != HAL_OK) return status;
    status = BMP180_ReadReg(device, BMP180_REG_OUT_XLSB, &XLSB);
    if (status != HAL_OK) return status;

    UP = ((MSB << 16) | (LSB << 8) | XLSB) >> (8 - OSS);

    // Save data
    device->OSS = OSS;
    device->raw_pressure = UP;

    return status;
}

HAL_StatusTypeDef BMP180_ReadTemp(BMP180 *device) {
    CALIBRATION_DATA cd = device->calibration_data;
    uint16_t UT;
    HAL_StatusTypeDef status;

    // Read raw temperature data
    status = BMP180_ReadRawTemp(device);
    if (status != HAL_OK) return status;
    UT = device->raw_temperature;

    // Convert Raw data
    int32_t X1 = (UT - cd.AC6) * cd.AC5 / 0x8000;
    int32_t X2 = cd.MC * 0x800 / (X1 + cd.MD);
    int32_t B5 = X1 + X2;
    int32_t T = (B5 + 8) / 0x10;

    // Save data
    device->B5 = B5;
    device->temp_C = (float)T / 10.0f;

    return status;
}

HAL_StatusTypeDef BMP180_ReadPressure(BMP180 *device, CONTROL_REGISTER control_register) {
    uint32_t UP;
    int32_t B5, T, B6, X1, X2, X3, B3, B7, p;
    uint32_t B4;
    int OSS;
    CALIBRATION_DATA cd = device->calibration_data;
    HAL_StatusTypeDef status;

    // Read temp to get B5 value
    BMP180_ReadTemp(device);
    B5 = device->B5;

    // Read raw pressure value
    BMP180_ReadRawPressure(device, control_register);
    OSS = device->OSS;
    UP = device->raw_pressure;

    // start conversion pressure
    B6 = B5 - 4000;
    X1 = ((int32_t)cd.B2 * ((B6 * B6) >> 12)) >> 11;
    X2 = ((int32_t)cd.AC2 * B6) >> 11;
    X3 = X1 + X2;
    B3 = (((((int32_t)cd.AC1) * 4 + X3) << OSS) + 2) >> 2;

    X1 = ((int32_t)cd.AC3 * B6) >> 13;
    X2 = ((int32_t)cd.B1 * ((B6 * B6) >> 12)) >> 16;
    X3 = ((X1 + X2) + 2) >> 2;

    B4 = ((int32_t)cd.AC4 * (uint32_t)(X3 + 32768)) >> 15;
    B7 = ((uint32_t)UP - B3) * (int32_t)(50000 >> OSS);

    if (B7 < 0x80000000)
        p = (B7 << 1) / B4;
    else
        p = (B7 / B4) << 1;

    X1 = (p >> 8);
    X1 *= X1;
    X1 = (X1 * 3038) >> 16;
    X2 = (-7357 * p) >> 16;
    p += (X1 + X2 + 3791) >> 4;

    // Save pressures in Pascals
    device->pressure_Pa = p;
    // Convert to elevation in meters
    device->elevation_M = 44330 * (1 - pow((float)p / (float)PRESSURE_SEA_LEVEL_PA, 0.19029495718));

    return status;
}

HAL_StatusTypeDef BMP180_ReadReg(BMP180 *device, uint8_t reg_addr, uint8_t *data) {
    // I2C-Handle, device-addr, loc-to-read-from, size-of-mem, output-ptr, how-many-bytes-to-read, time-out-delay
    return HAL_I2C_Mem_Read(device->hi2c, BMP180_I2C_ADDR, reg_addr, I2C_MEMADD_SIZE_8BIT, data, 1, HAL_MAX_DELAY);
}
HAL_StatusTypeDef BMP180_ReadRegs(BMP180 *device, uint8_t reg_addr, uint8_t *data, uint8_t size) {
    return HAL_I2C_Mem_Read(device->hi2c, BMP180_I2C_ADDR, reg_addr, I2C_MEMADD_SIZE_8BIT, data, size, HAL_MAX_DELAY);
}

HAL_StatusTypeDef BMP180_WriteReg(BMP180 *device, uint8_t reg_addr, uint8_t *data) {
    return HAL_I2C_Mem_Write(device->hi2c, BMP180_I2C_ADDR, reg_addr, I2C_MEMADD_SIZE_8BIT, data, 1, HAL_MAX_DELAY);
}