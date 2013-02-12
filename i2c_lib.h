#ifndef I2C_LIB_H_
#define I2C_LIB_H_

void initi2c();
int sendi2c(unsigned char addr, unsigned char TxData[], unsigned char len);
int geti2c(unsigned char addr, unsigned char RxData[], unsigned char len);

#endif
