/******************************************************************************
  Copyright (C), 2005-2009, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
  File Name     : gpiodriver.c
  Version       : Initial Draft
  Author        : Hisilicon FAE
  Created       : 2009-4-30
  Last Modified :

  Description   : Hi3511/2 GPIO common.

  Function List : 

  History       :
  1.Date        : 2009-07-29
    Author      : y56295
    Modification: Created file
  2.

******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define GPIO_SET_DIR 0x1
#define GPIO_GET_DIR 0x2
#define GPIO_READ_BIT 0x3
#define GPIO_WRITE_BIT 0x4

typedef struct {
	unsigned int  groupnumber;
	unsigned int  bitnumber;
	unsigned int  value;
}gpio_groupbit_info;


/******************* debug print ***********************/
#define GPIO_PRT(fmt...)    \
    do {                        \
        printf(" [func:%s, line:%d]  ", __FUNCTION__, __LINE__);\
        printf(fmt);            \
    }while(0)


static signed int fd = -1;
const  char* gpio_dev_name="/dev/hi_gpio";  //hi3518c.

int HI_SetGpio_Open(void)
{
    if (fd <= 0)
    {
        fd = open(gpio_dev_name, O_RDWR);
        if (fd <= 0)
        {
            GPIO_PRT("error:%s opened failed with: %d\n", gpio_dev_name, fd);
            return -1;
        }
        GPIO_PRT("%s open success!\n", gpio_dev_name);
    }

    return 0;
}


int Hi_SetGpio_Close(void)
{
    if (fd <= 0)
    {
        GPIO_PRT("file closed already!\n");
        return 0;
    }

    close(fd);
    fd = -1;
    return 0;
}


int Hi_SetGpio_SetDir(unsigned int gpio_groupnum, unsigned int gpio_bitnum, unsigned int gpio_dir )
{
    int tmp = 0;
    gpio_groupbit_info group_dir_info;

    group_dir_info.groupnumber = gpio_groupnum;
    group_dir_info.bitnumber = gpio_bitnum;
    group_dir_info.value = gpio_dir;

    if (fd <= 0)
    {
        GPIO_PRT("error:file not opened yet!\n");
        return -1;
    }

    tmp = ioctl(fd, GPIO_SET_DIR, (unsigned long)&group_dir_info);
    if (tmp != 0)
    {
        GPIO_PRT("error:set dir failed with: %d\n", tmp);
        return -1;
    }

    return 0;
}


int Hi_SetGpio_GetDir(unsigned int gpio_groupnum, unsigned int gpio_bitnum, unsigned int* gpio_dir)
{
    int tmp = 0;
    gpio_groupbit_info group_dir_info;

    group_dir_info.groupnumber = gpio_groupnum;
    group_dir_info.bitnumber = gpio_bitnum;
    group_dir_info.value = 0xffff;

    if (fd <= 0)
    {
        GPIO_PRT("error:file not opened yet!\n");
        return -1;
    }

    tmp = ioctl(fd, GPIO_GET_DIR, (unsigned long)&group_dir_info);
    if (tmp != 0)
    {
        GPIO_PRT("error:set dir failed with: %d\n", tmp);
        return -1;
    }

    *gpio_dir = group_dir_info.value;

    return 0;
}

int Hi_SetGpio_SetBit(unsigned int gpio_groupnum, unsigned int gpio_bitnum, unsigned int bitvalue)
{
    int tmp = 0;
    gpio_groupbit_info group_dir_info;

    group_dir_info.groupnumber = gpio_groupnum;
    group_dir_info.bitnumber = gpio_bitnum;
    group_dir_info.value = bitvalue;

    if (fd <= 0)
    {
        GPIO_PRT("error:file not opened yet!\n");
        return -1;
    }

    tmp = ioctl(fd, GPIO_WRITE_BIT, (unsigned long)&group_dir_info);
    if (tmp != 0)
    {
        GPIO_PRT("error:set dir failed with: %d\n", tmp);
        return -1;
    }

    return 0;
}

int Hi_SetGpio_GetBit(unsigned int gpio_groupnum, unsigned int gpio_bitnum, unsigned int *bitvalue)
{
    int tmp = 0;
    gpio_groupbit_info group_dir_info;

    group_dir_info.groupnumber = gpio_groupnum;
    group_dir_info.bitnumber = gpio_bitnum;
    group_dir_info.value = 0xffff;

    if (fd <= 0)
    {
        GPIO_PRT("error:file not opened yet!\n");
        return -1;
    }

    tmp = ioctl(fd, GPIO_READ_BIT, (unsigned long)&group_dir_info);
    if (tmp != 0)
    {
        GPIO_PRT("error:set dir failed with: %d\n", tmp);
        return -1;
    }

    *bitvalue = group_dir_info.value;
    return 0;
}

