#ifndef __GPIODRIVER_H__
#define __GPIODRIVER_H__

#define GPIO_DIR_IN 0
#define GPIO_DIR_OUT 1

#define GPIO_READ  0
#define GPIO_WRITE 1

int  HI_SetGpio_Open(void);
int  Hi_SetGpio_Close(void);

int  Hi_SetGpio_SetDir(unsigned int gpio_groupnum,unsigned int gpio_bitnum,unsigned gpio_dir );
int  Hi_SetGpio_GetDir(unsigned int gpio_groupnum,unsigned int gpio_bitnum,unsigned* gpio_dir );

int  Hi_SetGpio_SetBit(unsigned int gpio_groupnum,unsigned int gpio_bitnum, unsigned int bitvalue);
int  Hi_SetGpio_GetBit(unsigned int gpio_groupnum,unsigned int gpio_bitnum,unsigned int* bitvalue);

#endif/*__GPIODRIVER_H__*/

