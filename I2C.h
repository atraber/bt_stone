#ifndef I2C_LIB_H_
#define I2C_LIB_H_

void I2C_init();
int I2C_write(unsigned char addr, unsigned char* TxData, unsigned char len);
int I2C_read(unsigned char addr, unsigned char* RxData, unsigned char len);

#endif
