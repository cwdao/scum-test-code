#include <stdio.h>
#include "Memory_map.h"
#include "spi.h"

void spi_write(unsigned char writeByte) {
    int j;
    int t=0;
    for (j=7;j>=0;j--) {
        if ((writeByte&(0x01<<j)) != 0) {
            GPIO_REG__OUTPUT &= 0xFFFFBFFF; // clock low
            GPIO_REG__OUTPUT |= 0x00001000; // write a 1
            GPIO_REG__OUTPUT |= 0x00004000; // clock high
        }
        else {
            GPIO_REG__OUTPUT &= 0xFFFFBFFF; // clock low
            GPIO_REG__OUTPUT &= 0xFFFFEFFF; // write a 0
            GPIO_REG__OUTPUT |= 0x00004000; // clock high
        }
    }
    
    GPIO_REG__OUTPUT &= 0xFFFFEFFF; // set data out to 0
}
unsigned char spi_read() {
    unsigned char readByte;
    int j;
    int t = 0;
    readByte=0;
    GPIO_REG__OUTPUT &= 0xFFFFBFFF; // clock low
    
    for (j=7;j>=0;j--) {
        GPIO_REG__OUTPUT |= 0x00004000; // clock high
        readByte |= ((GPIO_REG__INPUT&0x00002000)>>13)<<j;      
        GPIO_REG__OUTPUT &= 0xFFFFBFFF; // clock low        
    }
    
    return readByte;
}
void spi_chip_select() {
    int t = 0;
    // drop chip select low to select the chip
    GPIO_REG__OUTPUT &= 0xFFFF7FFF;
    for(t=0; t<50; t++);
}
void spi_chip_deselect() {
    // hold chip select high to deselect the chip
    GPIO_REG__OUTPUT |= 0x00008000;
}
unsigned int read_acc_x() {
    unsigned int acc_x;
    unsigned char read_byte;
    unsigned char write_byte = 0x2D;
    
    acc_x = (read_imu_register(write_byte))<<8;   
    write_byte = 0x2E;
    acc_x |= read_imu_register(write_byte);
    
    return acc_x;
    
}
unsigned int read_acc_y() {
    unsigned int acc_y;
    unsigned char read_byte;
    unsigned char write_byte = 0x2F;
    
    acc_y = (read_imu_register(write_byte))<<8;   
    write_byte = 0x30;
    acc_y |= read_imu_register(write_byte);
    
    return acc_y;
}
unsigned int read_acc_z() {
    unsigned int acc_z;
    unsigned char read_byte;
    unsigned char write_byte = 0x31;
    
    acc_z = (read_imu_register(write_byte))<<8;
    write_byte = 0x32;
    acc_z |= read_imu_register(write_byte);
    
    return acc_z;
}
unsigned int test_imu_life() {
    unsigned char read_byte;
    unsigned char write_byte = 0x00;
    
    read_byte = read_imu_register(write_byte);  
    if (read_byte == 0xEA) {
        printf("My IMU is alive!!!\n");
				return 123;
    }
    else {
        printf("My IMU is not working :( \n");
				return 99;
    }
}
unsigned char read_imu_register(unsigned char reg) {
    unsigned char read_byte;
    reg &= 0x7F;
    reg |= 0x80;                        // guarantee that the function input is a valid input (not necessarily a valid, and readable, register)
    
    spi_chip_select();      // drop chip select
    spi_write(reg);         // write the selected register to the port
    read_byte = spi_read(); // clock out the bits and read them
    spi_chip_deselect();    // raise chip select
    
    return read_byte;
}
void write_imu_register(unsigned char reg, unsigned char data) {
    reg &= 0x7F;                        // guarantee that the function input is valid (not necessarily a valid, and readable, register)
    
    spi_chip_select();          // drop chip select
    spi_write(reg);                 // write the selected register to the port
    spi_write(data);                // write the desired register contents
    spi_chip_deselect();        // raise chip select
    
}
