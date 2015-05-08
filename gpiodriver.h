#ifndef __GPIODRIVER_H__
#define __GPIODRIVER_H__

#define GPIO_DIR_IN 0
#define GPIO_DIR_OUT 1

int  HI_SetGpio_Open(void);
int  Hi_SetGpio_Close(void);

int  Hi_SetGpio_SetDir(unsigned int gpio_groupnum,unsigned int gpio_bitnum,unsigned gpio_dir );
int  Hi_SetGpio_GetDir(unsigned int gpio_groupnum,unsigned int gpio_bitnum,unsigned* gpio_dir );

int  Hi_SetGpio_SetBit(unsigned int gpio_groupnum,unsigned int gpio_bitnum, unsigned int bitvalue);
int  Hi_SetGpio_GetBit(unsigned int gpio_groupnum,unsigned int gpio_bitnum,unsigned int* bitvalue);

typedef struct {
	unsigned int  groupnumber;
	unsigned int  bitnumber;
	unsigned int  value;
}gpio_groupbit_info;


#define GPIO_READ  0
#define GPIO_WRITE 1

#define GPIO_SET_DIR 0x1
#define GPIO_GET_DIR 0x2
#define GPIO_READ_BIT 0x3
#define GPIO_WRITE_BIT 0x4

#endif/*__GPIODRIVER_H__*/

