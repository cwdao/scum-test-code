// SPI functions
void spi_write(unsigned char writeByte);
unsigned char spi_read(void);
void spi_chip_select(void);
void spi_chip_deselect(void);
// Accelerometer read functions
unsigned int read_acc_x(void);
unsigned int read_acc_y(void);
unsigned int read_acc_z(void);
unsigned int test_imu_life(void);
// IMU-specific read/write functions
unsigned char read_imu_register(unsigned char reg);
void write_imu_register(unsigned char reg, unsigned char data);
