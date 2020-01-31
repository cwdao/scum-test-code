#include <stdint.h>

// SPI functions
void spi_write(uint8_t writeByte);
uint8_t spi_read(void);
void spi_chip_select(void);
void spi_chip_deselect(void);
// Accelerometer read functions
uint32_t read_acc_x(void);
uint32_t read_acc_y(void);
uint32_t read_acc_z(void);
// Temperature read function
uint32_t read_temp(void);
uint8_t test_imu_life(void);
// IMU-specific read/write functions
uint8_t read_imu_register(uint8_t reg);
void write_imu_register(uint8_t reg, uint8_t data);
