
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <errno.h>

#include "hiir.h"
#include "hiir_codedef.h"
#include "gpiodriver.h"


/** device type **/ //zqjun
#define DEV_IRDA_XHD 1
extern void ResetToDefaultSettings(void);
extern void OnRestorationParam( );

#define HK_IR_DEBUG(fmt...)   \
    do {                      \
        printf("[%s - %d]: ", __FUNCTION__, __LINE__);\
        printf(fmt);          \
    }while(0)

#define IR_DEVICE_NAME "/dev/Hi_IR"

typedef unsigned short int U8;
typedef struct
{
    char *irkey_name;
    unsigned int irkey_value;
}IRKEY_ADAPT;

/*key value*/ //zqjun.
#if DEV_IRDA_XHD //Xin He De remote controler.
    #define REMOTE_POWER1       0xf708ff00 //xhd: power on / off.
    #define REMOTE_MOTION_REC1  0xfa05ff00 //xhd: reset to factory settings.
    #define REMOTE_SNAPSHOT1    0xf906ff00
    #define REMOTE_AUTO_REC1    0xfb04ff00
    #define REMOTE_AUDIO1       0xff00ff00

    #define REMOTE_POWER2       0xff00ff00 //xhd: power on / off.
    #define REMOTE_MOTION_REC2  0xfa05ff00 //xhd: reset to factory settings.
    #define REMOTE_SNAPSHOT2    0xf50aff00
    #define REMOTE_AUTO_REC2    0xf708ff00
    #define REMOTE_AUDIO2       0xf20dff00
 
static IRKEY_ADAPT g_irkey_adapt_array[] =
{   /*irkey_name*/ /*irkey_value*/
    {"REMOTE_POWER1     ", REMOTE_POWER1,      /*电源键1*/ },
    {"REMOTE_POWER2     ", REMOTE_POWER2,      /*电源键2*/ },
    {"REMOTE_MOTION_REC1", REMOTE_MOTION_REC1, /*移动侦测录像键1*/ },
    {"REMOTE_MOTION_REC2", REMOTE_MOTION_REC2, /*移动侦测录像键2*/ },
    {"REMOTE_SNAPSHOT1  ", REMOTE_SNAPSHOT1,   /*抓拍键1*/ },
    {"REMOTE_SNAPSHOT2  ", REMOTE_SNAPSHOT2,   /*抓拍键2*/ },
    {"REMOTE_AUTO_REC1  ", REMOTE_AUTO_REC1,   /*开机录像键1*/ },
    {"REMOTE_AUTO_REC2  ", REMOTE_AUTO_REC2,   /*开机录像键2*/ },
    {"REMOTE_AUDIO1     ", REMOTE_AUDIO1,      /*音频键1*/ },
    {"REMOTE_AUDIO2     ", REMOTE_AUDIO2,      /*音频键2*/ },
};

#else //test remote controler.
    #define REMOTE_POWER    0xba45ff00
    #define REMOTE_MODE     0xb946ff00
    #define REMOTE_MUTE     0xb847ff00
    #define REMOTE_PRT      0xe619ff00
    #define REMOTE_SCAN     0xf20dff00
    #define REMOTE_F1       0xb24df708
    #define REMOTE_F2       0xb14ef708
    #define REMOTE_F3       0xae51f708
    #define REMOTE_F4       0xad52f708

static IRKEY_ADAPT g_irkey_adapt_array[] =
{   /*irkey_name*/ /*irkey_value*/
    {"REMOTE_POWER       ", REMOTE_POWER,/*电源*/   },
    {"REMOTE_MODE        ", REMOTE_MODE, /*模式*/   },
    {"REMOTE_MUTE        ", REMOTE_MUTE, /*静音*/   },
    {"REMOTE_PRT         ", REMOTE_PRT,  /*显示*/   },
    {"REMOTE_SCAN        ", REMOTE_SCAN, /*搜索*/   },   
    {"REMOTE_LEFT        ", 0xbf40ff00,  /*上一首*/ },
    {"REMOTE_RIGHT       ", 0xbc43ff00,  /*下一首*/ },
    {"REMOTE_CH+         ", 0xbb44ff00,  /*频道 + */},
    {"REMOTE_CH-         ", 0xf807ff00,  /*频道 - */},
    {"REMOTE_VOL+        ", 0xf609ff00,  /*音量 + */},
    {"REMOTE_VOL-        ", 0xea15ff00,  /*音量 - */},
    {"REMOTE_KEY_ZERO    ", 0xe916ff00,  /*0*/      },
    {"REMOTE_KEY_ONE     ", 0xf30cff00,  /*1*/      },
    {"REMOTE_KEY_TWO     ", 0xe718ff00,  /*2*/      },
    {"REMOTE_KEY_THREE   ", 0xa15eff00,  /*3*/      },
    {"REMOTE_KEY_FOUR    ", 0xf708ff00,  /*4*/      },
    {"REMOTE_KEY_FIVE    ", 0xe31cff00,  /*5*/      },
    {"REMOTE_KEY_SIX     ", 0xa55aff00,  /*6*/      },
    {"REMOTE_KEY_SEVEN   ", 0xbd42ff00,  /*7*/      },
    {"REMOTE_KEY_EIGHT   ", 0xad52ff00,  /*8*/      },
    {"REMOTE_KEY_NINE    ", 0xb54aff00,  /*9*/      },
};
#endif

static unsigned int g_PowerKeyState = 0;
static int g_irkey_adapt_count = sizeof(g_irkey_adapt_array) / sizeof(IRKEY_ADAPT);
static int powerkey_down = 0;
static int mutekey_down = 0;
static int f1key_down = 0;
static int f2key_down = 0;
static int f3key_down = 0;
static int f4key_down = 0;

#if 0
static void huawei_report_irkey(irkey_info_s rcv_irkey_info)
{
    int i = 0;
    for(i = 0; i < g_irkey_adapt_count; i++)
    {
        if( (rcv_irkey_info.irkey_datah == 0) &&
            (rcv_irkey_info.irkey_datal == g_irkey_adapt_array[i].irkey_value) )
        {
        	printf("keyvalue=H/L 0x%x/0x%x\n",rcv_irkey_info.irkey_datah,rcv_irkey_info.irkey_datal);
            break;
        }
    }
    if(i>=g_irkey_adapt_count)
    {
        printf("Error. get a invalid code. irkey_datah=0x%.8x,irkey_datal=0x%.8x.\n", 
               (int)rcv_irkey_info.irkey_datah, (int)rcv_irkey_info.irkey_datal);
    }
    else
    {
        printf("RECEIVE ---> %s\t", g_irkey_adapt_array[i].irkey_name);
        if(rcv_irkey_info.irkey_state_code == 1)
        {
            printf("KEYUP...");
        }
        printf("\n");
        
        if((rcv_irkey_info.irkey_datah == 0) && 
           (rcv_irkey_info.irkey_state_code == 0) &&
           (rcv_irkey_info.irkey_datal == REMOTE_POWER)) /*POWER*/
        {
            powerkey_down = 1;
        }
        else if((rcv_irkey_info.irkey_datah == 0) && 
                (rcv_irkey_info.irkey_state_code == 0) &&
                (rcv_irkey_info.irkey_datal == REMOTE_MUTE)) /*MUTE*/
        {
            mutekey_down = 1;
        }
        else if((rcv_irkey_info.irkey_datah == 0) && 
                (rcv_irkey_info.irkey_state_code == 0) &&
                (rcv_irkey_info.irkey_datal == REMOTE_F1)) /*F1*/
        {
            f1key_down = 1;
        }
        else if((rcv_irkey_info.irkey_datah == 0) && 
                (rcv_irkey_info.irkey_state_code == 0) &&
                (rcv_irkey_info.irkey_datal == REMOTE_F2)) /*F2*/
        {
            f2key_down = 1;
        }
        else if((rcv_irkey_info.irkey_datah == 0) && 
                (rcv_irkey_info.irkey_state_code == 0) &&
                (rcv_irkey_info.irkey_datal == REMOTE_F3)) /*F3*/
        {
            f3key_down = 1;
        }
        else if((rcv_irkey_info.irkey_datah == 0) && 
                (rcv_irkey_info.irkey_state_code == 0) &&
                (rcv_irkey_info.irkey_datal == REMOTE_F4)) /*F4*/
        {
            f4key_down = 1;
        }
    }
}
#endif

static void normal_report_irkey(irkey_info_s rcv_irkey_info)
{
    printf("RECEIVE ---> irkey_datah=0x%.8x, irkey_datal=0x%.8x\t", (int)(rcv_irkey_info.irkey_datah), (int)(rcv_irkey_info.irkey_datal));

    printf("irkey_state_code: %d...\n", rcv_irkey_info.irkey_state_code);
    if (rcv_irkey_info.irkey_state_code == 1)
    {
        printf("..KEYUP...\n");
    }

    return;
}


static inline void ir_config_fun(int fp, hiir_dev_param dev_param)
{
    int tmp[2] = {0};

    ioctl(fp, IR_IOC_SET_CODELEN, dev_param.code_len);
    
    ioctl(fp, IR_IOC_SET_FORMAT, dev_param.codetype);

    ioctl(fp, IR_IOC_SET_FREQ, dev_param.frequence);

    tmp[0] = dev_param.leads_min;
    tmp[1] = dev_param.leads_max;
    ioctl(fp, IR_IOC_SET_LEADS, tmp);

    tmp[0] = dev_param.leade_min;
    tmp[1] = dev_param.leade_max;
    ioctl(fp, IR_IOC_SET_LEADE, tmp);

    tmp[0] = dev_param.sleade_min;
    tmp[1] = dev_param.sleade_max;
    ioctl(fp, IR_IOC_SET_SLEADE, tmp);

    tmp[0] = dev_param.cnt0_b_min;
    tmp[1] = dev_param.cnt0_b_max;
    ioctl(fp, IR_IOC_SET_CNT0_B, tmp);

    tmp[0] = dev_param.cnt1_b_min;
    tmp[1] = dev_param.cnt1_b_max;
    ioctl(fp, IR_IOC_SET_CNT1_B, tmp);
}

void Hi_IR_FUNC_TEST()
{
    int filp;
    int i;
    hiir_dev_param tmp;

    printf("Hi_IR_FUNC_TEST start...\n");
    printf("REMOTE codetype ...NEC with simple repeat code - uPD6121G\n");
    
    if( -1 == (filp = open("/dev/"HIIR_DEVICE_NAME, O_RDWR) ) )
    {
        printf("Hi_IR_FUNC_TEST_004 ERROR:can not open %s device. read return %d\n", HIIR_DEVICE_NAME, filp);
        return;
    }

    for (i = 0; i < 16; i++)
    {
        ir_config_fun(filp, static_dev_param[i]);
        ioctl(filp, IR_IOC_GET_CONFIG, &tmp);
        if( 0 != memcmp(&tmp, &(static_dev_param[i]), sizeof(hiir_dev_param)) )
        {
            printf("Hi_IR_FUNC_TEST_004 ERROR. need check ioctl fun.\n");
            return;
        }
    }
    printf("Hi_IR_FUNC_TEST pass.\n\n");
    close(filp);
}


/*******************************************************
 * func: do event to check remote key value.
 ******************************************************/
static int IR_DoEvent(irkey_info_s IRKeyInfo)
{
    int i = 0;
    short nRet = 0;
    //HK_IR_DEBUG("ir key info, datah:0x%.8x, datal:0x%.8x...\n", (int)(IRKeyInfo.irkey_datah), (int)(IRKeyInfo.irkey_datal));

    for (i = 0; i < g_irkey_adapt_count; i++)
    {
        //printf("...coun: %d, i: %d...\n", g_irkey_adapt_count, i);
        if ( (IRKeyInfo.irkey_datah == 0) && (IRKeyInfo.irkey_datal == g_irkey_adapt_array[i].irkey_value) )
        {
            printf("key_name:%s, key_value: H/L 0x%x/0x%x...\n", g_irkey_adapt_array[i].irkey_name, IRKeyInfo.irkey_datah, IRKeyInfo.irkey_datal);
            if ((IRKeyInfo.irkey_datah == 0) && 
               ((IRKeyInfo.irkey_datal == g_irkey_adapt_array[0].irkey_value) || (IRKeyInfo.irkey_datal == g_irkey_adapt_array[1].irkey_value)))//POWER.
            {
                //printf("[%s] power key down !!!\n", __func__);
                nRet = 1;
            }
            else if ((IRKeyInfo.irkey_datah == 0) && 
                    ((IRKeyInfo.irkey_datal == g_irkey_adapt_array[2].irkey_value) || (IRKeyInfo.irkey_datal == g_irkey_adapt_array[3].irkey_value))) //DEFAULT.
            {
                //printf("[%s] reset to default params !!!\n", __func__);
                nRet = 2;
            }
            else
            {
                nRet = 0;
            }
            break;
        }
    }
    return nRet;
}


/***********************************************************
 * func: receive infrared remoter signal, and decode it.
 ***********************************************************/
#if 0
void *Handle_IR_Key_Event(void* arg)
{
    printf("Handle IR key event thread start !\n");

    //Hi_IR_FUNC_TEST();

    int ret = 0, i = 0, count = 0, delay = 0;
    int irfd = 0; 
    fd_set rfds;
    struct timeval tv;
	unsigned long devBufLen = 200;

    irkey_info_s rcv_irkey_info;

    if (-1 == (irfd = open(IR_DEVICE_NAME, O_RDONLY, 0)))
    {
        printf("open %s error: %d, %s\n", HIIR_DEVICE_NAME, errno, strerror(errno));
        return;
    }

    if (0 == ioctl(irfd, IR_IOC_SET_BUF, (unsigned long)devBufLen) )
    {
        printf("SUCCESS: ioctl(Hi_IR, IR_IOC_SET_BUF, %u)\n", devBufLen);
    }
    else
    {
        printf("FAILED: ioctl(Hi_IR, IR_IOC_SET_BUF, %u)\n", devBufLen);
    }

    while (1)
    {
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO(&rfds);
        FD_SET(irfd, &rfds);
        ret = select(irfd+1 , &rfds, NULL, NULL, &tv);
        if (ret <= 0)
        {
            printf("get irkey info time out!\n");
            usleep(1000);
            continue;
        }

        if (FD_ISSET(irfd, &rfds))
        {
            memset(&rcv_irkey_info, '\0', sizeof(rcv_irkey_info));
            ret = read(irfd, &rcv_irkey_info, sizeof(rcv_irkey_info));
            printf(".........ret: %d..........\n", ret);
            if ( (ret > 0) && (ret <= sizeof(rcv_irkey_info)) )
            {
                normal_report_irkey(rcv_irkey_info);
                continue;
            }
            else
            {
                //printf("test Error. read irkey device error. ret=%d.\n", ret);
            }
        }
    }
    close(irfd);
    return 0;
}
#else
void *Handle_IR_Key_Event(void* arg)
{
    HK_IR_DEBUG("Handle IR key event thread start !\n");

    int ret = 0, i = 0;
    int irfd = 0; 
    irkey_info_s rcv_irkey_info;
    int KeyStateCode = 0;

    if (-1 == (irfd = open(IR_DEVICE_NAME, O_RDONLY, 0))) //BLOCK.
    {
        HK_IR_DEBUG("open %s error: %d, %s\n", HIIR_DEVICE_NAME, errno, strerror(errno));
        return NULL;
    }

    while (1)
    {
        memset(&rcv_irkey_info, '\0', sizeof(rcv_irkey_info));
        ret = read(irfd, &rcv_irkey_info, sizeof(rcv_irkey_info));
        //printf(".........ret: %d..........\n", ret);
        if ( (ret > 0) && (ret <= sizeof(rcv_irkey_info)) )
        {
            //normal_report_irkey(rcv_irkey_info); //key value.
            
            if (1 == rcv_irkey_info.irkey_state_code)
            {
                //printf(".......KEY UP........\n");
                KeyStateCode = IR_DoEvent(rcv_irkey_info);
                printf(".......KeyStateCode: %d, g_PowerKeyState: %d.\n", KeyStateCode, g_PowerKeyState);

                if (1 == KeyStateCode)
                {
                    g_PowerKeyState += KeyStateCode;
                    if (2 == g_PowerKeyState)
                    {
                        g_PowerKeyState = 0;
                    }
                }
                else if (2 == KeyStateCode)
                {
                #if 1
                    //ResetToDefaultSettings( ); 
                    OnRestorationParam( ); //reset to factory settings.
                #endif
                }
            }
        }
    }

    memset(&rcv_irkey_info, '\0', sizeof(rcv_irkey_info));
    close(irfd);
    return 0;
}
#endif

void *Handle_Light_Control(void* arg)
{
    unsigned int val_read = 0, val_set = 0, curState = 0;
    unsigned int groupnum = 0, bitnum = 0;
    short nRet = 0;
    //unsigned int statusCount = 0;

    groupnum = 5;
    bitnum = 3; //GPIO:5_3.
    nRet = Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_READ );
    nRet = Hi_SetGpio_GetBit( groupnum, bitnum, &val_read );
    HK_IR_DEBUG("...Get GPIO %d_%d  read Value: %d...\n", groupnum, bitnum, val_read);

    while (1)
    {
        //HK_IR_DEBUG("g_Powerkey: %d, g_PowerKeyState: %d.\n", g_PowerKey, g_PowerKeyState);
        if (1 == g_PowerKeyState) //switch on.
        {
            //statusCount ++; //count the times of press power key.
            //if (2 == statusCount) //2s.
            {
                //statusCount = 0;
                val_set = 1;
                Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
                Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
                //HK_IR_DEBUG("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set);
            }
        }
        else if (0 == g_PowerKeyState) //switch off.
        {
            val_set = 0;
            Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
            Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
            //HK_IR_DEBUG("....Set GPIO %d_%d  set Value: %d....\n", groupnum, bitnum, val_set);
        }

        sleep(1);
    }

    return;
}


int HK_Infrared_Decode(void)
{
    int ret = 0;
    pthread_t Key_Event, Light_Event;
    void *thread_result;

    system("himm 0x200F00C4 0x0"); //set register func: IR_IN.
    system("himm 0x200F00C0 0x0"); //GPIO5_3, LED light.

    /** Infrared Remoter Key check **/
    ret = pthread_create(&Key_Event, NULL, (void *)Handle_IR_Key_Event, NULL);
    if (0 != ret)
    {
        printf("pthread_create failed with:%d, %s\n", errno, strerror(errno));
        pthread_detach(Key_Event);
        return -1;
    }
    pthread_detach(Key_Event);

    /** Key control light on / off **/
    ret = pthread_create(&Light_Event, NULL, (void *)Handle_Light_Control, NULL);
    if (0 != ret)
    {
        printf("pthread_create failed with:%d, %s\n", errno, strerror(errno));
        pthread_detach(Light_Event);
        return -1;
    }
    pthread_detach(Light_Event);

    return 0;
}

