
#include "ptz.h"
#include "ipc_hk.h"

int g_RotateSpeed = 1;  //rotate speed
int g_RotateDir_LR = 0; //dir: left / right
int g_RotateDir_UD = 0; //dir: up / down
int g_LR_AutoRotateCount = 0; //count the times of hit left & right limit switch.
int g_UD_AutoRotateCount = 0; //count the times of hit up & down limit switch.
int g_UD_StepCount = 0; //calculate the current position.
int g_LR_StepCount = 0; //calculate the current position.
int g_UDStepLength = 0; //the step number from up to down.
int g_LRStepLength = 0; //the step number from left to right.

int g_LR_LimitFlag = 0; //0: with limit switch; 1: without limit switch.
int g_UD_LimitFlag = 0; //0: with limit switch; 1: without limit switch.

int g_RotateRunning = 0;    //quit auto rotate.
int g_PtzRotateEnable = 0;  //ptz rotate flag, 1:enable rotate, 0:ptz stop.
int g_PtzRotateType = 0;    //type ==> 1:leftright; 2:updown; 3:all direction auto rotate.
int g_PtzStepType = 0;      //ptz step, 1:left, 2:right, 3:up, 4:down.
int g_PtzPresetPos = 0;     //preset position: 1 ~ 8.
unsigned long g_tmPTZStart = 0, g_tmPTZStop = 0;



/****************************************************
* Suspend execution for x milliseconds intervals.
*  @param ms Milliseconds to sleep.
****************************************************/
void delayMS(int x)
{
 	//usleep(x * 1000);
 	//usleep(x * 100);
 	//usleep(x * 10);
 	//usleep(x * 1);
 	usleep(1);
}

int Set_PTZ_PresetParams(int nValue, int xPos, int yPos)
{
	char presetXName[20]= {0};	
	char presetYName[20]= {0};
	sprintf(presetXName, "preset%dx", nValue);
	sprintf(presetYName, "preset%dy", nValue);

	conf_set_int( PTZ_CONF, presetXName, xPos);		
	conf_set_int( PTZ_CONF, presetYName, yPos);

	return 0;
}

int Get_PTZ_PresetParams(int nValue, int *xPos, int *yPos)
{
	char presetXName[20] = {0};	
	char presetYName[20] = {0};

	sprintf(presetXName, "preset%dx", nValue);	
	sprintf(presetYName, "preset%dy", nValue);

	*xPos = conf_get_int(PTZ_CONF, presetXName);		
	*yPos = conf_get_int(PTZ_CONF, presetYName);
	//printf("presetXName=%s,xPos=%d\n",presetXName,*xPos);
	//printf("presetYName=%s,yPos=%d\n",presetYName,*yPos);

	return 0;
}

int Set_PTZ_RotateSpeed(int nPtzSpeed)
{
	conf_set_int(PTZ_CONF, "ptzspeed", nPtzSpeed);
	
	return 0;
}

int Get_PTZ_RotateSpeed(void)
{
	int delay = 1, nSpeed = 0;

	nSpeed = conf_get_int(PTZ_CONF, "ptzspeed");
	switch (nSpeed)
	{
		case 1:
			delay = 18;
			break;
		case 2:
			delay = 16;
			break;
		case 3:
			delay = 14;
			break;
		case 4:
			delay = 12;
			break;
		case 5:
			delay = 10;
			break;
		case 6:
			delay = 8;
			break;
		case 7:
			delay = 6;
			break;
		case 8:
			delay = 4;
			break;
		case 9:
			delay = 2;
			break;
		case 10:
			delay = 1;
			break;
		default:
			delay = 1;
			break;
	}
	
	return delay;
}


/****************************************************************
* Stop the motor.
*  @param pins     A pointer which points to the pins number array.
*****************************************************************/
void PTZ_AutoRotate_Stop(int *pins) 
{
	int i = 0;
	int groupnum = 9;
	int bitnum = 0;
	int val_set = 0; //0:pull down; 1:pull up.

	for (i = 0; i < 4; i++) 
	{
		bitnum = pins[i];
		Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
		Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
	}
}

/****************************************************************
* Stop L&R U&D motor.
*  @param pins: A pointer which points to the pins number array.
*****************************************************************/
void PTZ_Rotate_Stop(int *pins) 
{
	int val_set = 0; //pull down.
	int groupnum = 9;
	int bitnum = 0;

	int i = 0;
	for (i = 0; i < 8; i++) 
	{
		bitnum = pins[i];
		Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_WRITE );
		Hi_SetGpio_SetBit( groupnum, bitnum, val_set );
	}
}

/**********************************************
 * func: rorate from right side to left side.
 * GPIO: 9_0 --> 9_3.
 *********************************************/
int MotorDir_Right_To_Left(int bit)
{
    int j = 0;
    int pins[4] = {0, 1, 2, 3}; //GPIO9_0 ~ GPIO9_3.
    unsigned int val_set = 0; //pull down.
    unsigned int groupnum = 9;
    unsigned int bitnum = 0;

    for (j = 0; j < 4; j++) 
    {
        if (bit == j) 
        {
            val_set = 1;
            bitnum = pins[j]; //GPIO9_0 --> GPIO9_3 output high level voltage.
            Hi_SetGpio_SetDir(groupnum, bitnum, GPIO_WRITE);
            Hi_SetGpio_SetBit(groupnum, bitnum, val_set);// output a high level 
        } 
        else
        {
            val_set = 0;					 
            bitnum = pins[j];
            Hi_SetGpio_SetDir(groupnum, bitnum, GPIO_WRITE);
            Hi_SetGpio_SetBit(groupnum, bitnum, val_set); // output a low level 
        }
    }
    return 0;
}

/**********************************************
 * func: rorate from left side to right side.
 * GPIO: 9_3 --> 9_0.
 *********************************************/
int MotorDir_Left_To_Right(int bit)
{
    int j = 0;
    int pins[4] = {3, 2, 1, 0}; //GPIO9_3 ~ GPIO9_0.
    unsigned int val_set = 0; //pull down.
    unsigned int groupnum = 9;
    unsigned int bitnum = 0;

    for (j = 0; j < 4; j++) 
    {
        if (bit == j) 
        {
            val_set = 1;
            bitnum = pins[j]; //GPIO9_3 --> GPIO9_0 output high level voltage.
            Hi_SetGpio_SetDir(groupnum, bitnum, GPIO_WRITE);
            Hi_SetGpio_SetBit(groupnum, bitnum, val_set);// output a high level 
        } 
        else
        {
            val_set = 0;					 
            bitnum = pins[j];
            Hi_SetGpio_SetDir(groupnum, bitnum, GPIO_WRITE);
            Hi_SetGpio_SetBit(groupnum, bitnum, val_set); // output a low level 
        }
    }
    return 0;
}

/**********************************************
 * func: rorate from bottom side to top side.
 * GPIO: 9_4 --> 9_7.
 *********************************************/
int MotorDir_Down_To_Up(int bit)
{
    int j = 0;
    int pins[4] = {4, 5, 6, 7}; //GPIO9_4 ~ GPIO9_7.
    unsigned int val_set = 0; //pull down.
    unsigned int groupnum = 9;
    unsigned int bitnum = 0;

    for (j = 0; j < 4; j++) 
    {
        if (bit == j) 
        {
            val_set = 1;
            bitnum = pins[j]; //GPIO9_4 --> GPIO9_7 output high level voltage.
            Hi_SetGpio_SetDir(groupnum, bitnum, GPIO_WRITE);
            Hi_SetGpio_SetBit(groupnum, bitnum, val_set);// output a high level 
        } 
        else
        {
            val_set = 0;					 
            bitnum = pins[j];
            Hi_SetGpio_SetDir(groupnum, bitnum, GPIO_WRITE);
            Hi_SetGpio_SetBit(groupnum, bitnum, val_set); // output a low level 
        }
    }
    return 0;
}

/**********************************************
 * func: rorate from top side to bottom side.
 * GPIO: 9_7 --> 9_4.
 *********************************************/
int MotorDir_Up_To_Down(int bit)
{
    int j = 0;
    int pins[4] = {7, 6, 5, 4}; //GPIO9_7 ~ GPIO9_4.
    unsigned int val_set = 0; //pull down.
    unsigned int groupnum = 9;
    unsigned int bitnum = 0;

    for (j = 0; j < 4; j++) 
    {
        if (bit == j) 
        {
            val_set = 1;
            bitnum = pins[j]; //GPIO9_7 --> GPIO9_4 output high level voltage.
            Hi_SetGpio_SetDir(groupnum, bitnum, GPIO_WRITE);
            Hi_SetGpio_SetBit(groupnum, bitnum, val_set);// output a high level 
        } 
        else
        {
            val_set = 0;					 
            bitnum = pins[j];
            Hi_SetGpio_SetDir(groupnum, bitnum, GPIO_WRITE);
            Hi_SetGpio_SetBit(groupnum, bitnum, val_set); // output a low level 
        }
    }
    return 0;
}


/***********************************************
 * func: limit switch on left side,
 *       return 0 while limit switch hit;
 *       else, return 1.
 ***********************************************/
int PTZ_LimitSwitch_Left(void)
{
	unsigned int val_read = 0;
	unsigned int groupnum = 5;
	unsigned int bitnum   = 7; //MOTOR LEFT: GPIO5_7.
	
	Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_READ );
	Hi_SetGpio_GetBit( groupnum, bitnum, &val_read );

	//if (0 == val_read)
	//{		
	//	//printf("[%s] Read GPIO%d_%d = %d....left hit....\n", __func__, groupnum, bitnum, val_read);	
    //    g_LR_AutoRotateCount ++;
    //    g_RotateDir_LR = COUNTER_CLOCKWISE; //change dir: left --> right.

    //    if (1 == g_LR_AutoRotateCount)
    //    {
    //        g_LR_StepCount = 0; //reset the step length value first time.
    //    }
    //    PTZ_DEBUG_PRT("......left hit...... g_LR_AutoRotateCount:%d, g_LRStepLength:%d, g_LR_StepCount:%d ..............\n", g_LR_AutoRotateCount, g_LRStepLength, g_LR_StepCount);

    //    return 1;
	//}
	//else
	//{
    //    if (2 <= g_LR_AutoRotateCount)
    //    {
    //        g_LR_StepCount --; //step to preset position.
    //    }
    //    //PTZ_DEBUG_PRT("g_LR_AutoRotateCount:%d, g_LRStepLength:%d, g_LR_StepCount:%d\n", g_LR_AutoRotateCount, g_LRStepLength, g_LR_StepCount);
    //    
    //    return 0;
	//}
	return val_read;
}

/***********************************************
 * func: limit switch on right side,
 *       return 0 while limit switch hit.
 *       else, return 1.
 ***********************************************/
int PTZ_LimitSwitch_Right(void)
{
	unsigned int val_read = 0;
	unsigned int groupnum = 5;
	unsigned int bitnum   = 6; //MOTOR RIGHT: GPIO5_6.
	
	Hi_SetGpio_SetDir( groupnum, bitnum, GPIO_READ );
	Hi_SetGpio_GetBit( groupnum, bitnum, &val_read );

	//if (0 == val_read)
	//{		
	//	//printf("[%s] Read GPIO%d_%d = %d....right hit....\n", __func__, groupnum, bitnum, val_read);	
    //    g_LRStepLength = g_LR_StepCount;
    //    g_LR_AutoRotateCount ++;
    //    g_RotateDir_LR = CLOCKWISE; //change dir: right --> left.
    //    PTZ_DEBUG_PRT("......right hit...... g_LR_AutoRotateCount:%d, g_LRStepLength:%d, g_LR_StepCount:%d ..............\n", g_LR_AutoRotateCount, g_LRStepLength, g_LR_StepCount);
    //    return 1;
    //}
	//else
	//{
    //    g_LR_StepCount ++; //count current motor position.
    //    //PTZ_DEBUG_PRT("g_LR_AutoRotateCount:%d, g_LRStepLength:%d, g_LR_StepCount:%d\n", g_LR_AutoRotateCount, g_LRStepLength, g_LR_StepCount);
    //    return 0;
	//}

	return val_read;
}

/***********************************************
 * func: limit switch on up side,
 *       return 0 while limit switch hit.
 *       else, return 1.
 ***********************************************/
int PTZ_LimitSwitch_Up(void)
{
	unsigned int val_read = 0;
	unsigned int groupnum = 5;
	unsigned int bitnum   = 5; //4; //MOTOR UP: GPIO5_4.
	
	Hi_SetGpio_SetDir(groupnum, bitnum, GPIO_READ);
	Hi_SetGpio_GetBit(groupnum, bitnum, &val_read);

	//if (0 == val_read)
	//{		
	//	//printf("[%s] Read GPIO%d_%d = %d....up hit....\n", __func__, groupnum, bitnum, val_read);	
    //    g_UD_AutoRotateCount ++;
    //    g_RotateDir_UD = COUNTER_CLOCKWISE; //change dir: up --> down.

    //    if (1 == g_UD_AutoRotateCount) //hit onece.
    //    {
    //        g_UD_StepCount = 0; //reset the step length value the first time.
    //    }
    //    PTZ_DEBUG_PRT("......up hit...... g_UD_AutoRotateCount:%d, g_UDStepLength:%d, g_UD_StepCount:%d ..............\n", g_UD_AutoRotateCount, g_UDStepLength, g_UD_StepCount);
    //    return 1;
	//}
	//else
	//{
    //    if (2 <= g_UD_AutoRotateCount)
    //    {
    //        g_UD_StepCount --; //step to preset position.
    //    }
    //    //PTZ_DEBUG_PRT("g_UD_AutoRotateCount:%d, g_UDStepLength:%d, g_UD_StepCount:%d\n", g_UD_AutoRotateCount, g_UDStepLength, g_UD_StepCount);
    //    return 0;
	//}
	return val_read;
}

/***********************************************
 * func: limit switch on down side,
 *       return 0 while limit switch hit,
 *       else, return 1.
 ***********************************************/
int PTZ_LimitSwitch_Down(void)
{
	unsigned int val_read = 0;
	unsigned int groupnum = 5;
	unsigned int bitnum   = 4; //5; //MOTOR DOWN: GPIO5_5.
	
	Hi_SetGpio_SetDir(groupnum, bitnum, GPIO_READ);
	Hi_SetGpio_GetBit(groupnum, bitnum, &val_read);

	//if (0 == val_read)
	//{		
	//	//printf("[%s] Read GPIO%d_%d = %d....down hit....\n", __func__, groupnum, bitnum, val_read);	
    //    g_UDStepLength = g_UD_StepCount; //save the distance from up to down.
    //    g_UD_AutoRotateCount ++;
    //    g_RotateDir_UD = CLOCKWISE; //change direction: down to up.
    //    PTZ_DEBUG_PRT("......down hit...... g_UD_AutoRotateCount:%d, g_UDStepLength:%d, g_UD_StepCount:%d ..............\n", g_UD_AutoRotateCount, g_UDStepLength, g_UD_StepCount);
    //    return 1;
	//}
	//else
	//{
    //    g_UD_StepCount ++; //count current motor position.
    //    //PTZ_DEBUG_PRT("g_UD_AutoRotateCount:%d, g_UDStepLength:%d, g_UD_StepCount:%d\n", g_UD_AutoRotateCount, g_UDStepLength, g_UD_StepCount);
    //    return 0;
	//}
	return val_read;
}



/**************************************************
 * left or right direction auto ratate mode,
 * GPIO: 9_0(D1), 9_1(C1), 9_2(B1), 9_3(A1).
 **************************************************/
//int PTZ_AutoRotate_LeftRight(int *pins, int direction, int delay) 
int PTZ_AutoRotate_LeftRight(int direction, int nSpeed) 
{
    int i = 0;
    unsigned int groupnum = 9;
    unsigned int bitnum = 0;
    unsigned int val_set = 0; //0:pull down; 1:pull up.

	for (i = 0; i < 4; i++) 
	{
		if (CLOCKWISE == direction) //dir: R --> L.
		{		
            MotorDir_Right_To_Left(i);

            if (0 == g_LR_LimitFlag) //with limit switch.
            {
                if (0 == PTZ_LimitSwitch_Left()) //hit left limit switch.
                {
                    g_LR_StepCount = 0; //reset the step counter.
                    PTZ_DEBUG_PRT("left hit...g_LR_LimitFlag:%d, g_LRStepLength:%d, g_LR_StepCount:%d\n", g_LR_LimitFlag, g_LRStepLength, g_LR_StepCount);
                    g_RotateDir_LR = COUNTER_CLOCKWISE; //change dir: left --> right.
                    break;
                }
            }
            else if (1 == g_LR_LimitFlag) //without limit switch.
            {
                if (0 == g_LR_StepCount) 
                {
                    PTZ_DEBUG_PRT("left......g_LR_LimitFlag:%d, g_LRStepLength:%d, g_LR_StepCount:%d\n", g_LR_LimitFlag, g_LRStepLength, g_LR_StepCount);
                    g_RotateDir_LR = COUNTER_CLOCKWISE; //change dir: left --> right.
                    break;
                }
            }
            
            g_LR_StepCount --; //count current motor position.
            //PTZ_DEBUG_PRT("g_LR_LimitFlag:%d, g_LRStepLength:%d, g_LR_StepCount:%d\n", g_LR_LimitFlag, g_LRStepLength, g_LR_StepCount);
		}
		else if (COUNTER_CLOCKWISE == direction) //dir: L --> R.
		{
            MotorDir_Left_To_Right(i);

            if (0 == g_LR_LimitFlag) //with limit switch.
            {
                if (0 == PTZ_LimitSwitch_Right()) //hit right limit switch.
                {
                    PTZ_DEBUG_PRT("right hit...g_LR_LimitFlag:%d, g_LRStepLength:%d, g_LR_StepCount:%d\n", g_LR_LimitFlag, g_LRStepLength, g_LR_StepCount);
                    g_RotateDir_LR = CLOCKWISE; //change dir: right --> left.
                    break;
                }
            }
            else if (1 == g_LR_LimitFlag) //without limit switch.
            {
                if (g_LR_StepCount == PTZ_RANGE_LR) 
                {
                    PTZ_DEBUG_PRT("right......g_LR_LimitFlag:%d, g_LRStepLength:%d, g_LR_StepCount:%d\n", g_LR_LimitFlag, g_LRStepLength, g_LR_StepCount);
                    g_RotateDir_LR = CLOCKWISE; //change dir: left --> right.
                    break;
                }
            }
            
            g_LR_StepCount ++; //count current motor position.
            //PTZ_DEBUG_PRT("g_LR_LimitFlag:%d, g_LRStepLength:%d, g_LR_StepCount:%d\n", g_LR_LimitFlag, g_LRStepLength, g_LR_StepCount);
		}
		delayMS(nSpeed);
	}

    return 0;
}


/************************************************
 * up or down direction auto rotate mode,
 * GPIO: 9_4(A), 9_5(B), 9_6(C), 9_7(D).
 ************************************************/
//int PTZ_AutoRotate_UpDown(int *pins, int direction, int delay)
int PTZ_AutoRotate_UpDown(int direction, int nSpeed)
{
	int i = 0, j = 0;
	unsigned int val_set = 0; //pull down.
	unsigned int groupnum = 9;
	unsigned int bitnum = 0;
	
	for (i = 0; i < 4; i++) 
	{
		if (CLOCKWISE == direction) //dir: D --> U.
		{
            MotorDir_Down_To_Up(i);

            if (0 == g_UD_LimitFlag) //with limit switch.
            {
                if (0 == PTZ_LimitSwitch_Up()) //hit up limit switch.
                {
                    g_UD_StepCount = 0; //reset step counter.
                    PTZ_DEBUG_PRT("up hit...g_UD_LimitFlag:%d, g_UDStepLength:%d, g_UD_StepCount:%d\n", g_UD_LimitFlag, g_UDStepLength, g_UD_StepCount);
                    g_RotateDir_UD = COUNTER_CLOCKWISE; //change dir: up --> down.
                    break;
                }
            }
            else if (1 == g_UD_LimitFlag) //without limit switch.
            {
                if (0 == g_UD_StepCount) //change direction wihle 0 position.
                {
                    PTZ_DEBUG_PRT("up......g_UD_LimitFlag:%d, g_UDStepLength:%d, g_UD_StepCount:%d\n", g_UD_LimitFlag, g_UDStepLength, g_UD_StepCount);
                    g_RotateDir_UD = COUNTER_CLOCKWISE; //change dir: up --> down.
                    break;
                }
            }

            g_UD_StepCount --;
            //PTZ_DEBUG_PRT("g_UD_LimitFlag:%d, g_UDStepLength:%d, g_UD_StepCount:%d\n", g_UD_LimitFlag, g_UDStepLength, g_UD_StepCount);
		}
		else if (COUNTER_CLOCKWISE == direction) //dir: U --> D.
		{
            MotorDir_Up_To_Down(i);

            if (0 == g_UD_LimitFlag) //with limit switch.
            {
                if (0 == PTZ_LimitSwitch_Down()) //hit down limit switch.
                {
                    PTZ_DEBUG_PRT("down hit...g_UD_LimitFlag:%d, g_UDStepLength:%d, g_UD_StepCount:%d\n", g_UD_LimitFlag, g_UDStepLength, g_UD_StepCount);
                    g_RotateDir_UD = CLOCKWISE; //change dir: down --> up.
                    break;
                }
            }
            else if (1 == g_UD_LimitFlag) //without limit switch.
            {
                if (g_UD_StepCount == PTZ_RANGE_UD) 
                {
                    PTZ_DEBUG_PRT("down......g_UD_LimitFlag:%d, g_UDStepLength:%d, g_UD_StepCount:%d\n", g_UD_LimitFlag, g_UDStepLength, g_UD_StepCount);
                    g_RotateDir_UD = CLOCKWISE; //change dir: down --> up.
                    break;
                }
            }

            g_UD_StepCount ++;
            //PTZ_DEBUG_PRT("g_UD_LimitFlag:%d, g_UDStepLength:%d, g_UD_StepCount:%d\n", g_UD_LimitFlag, g_UDStepLength, g_UD_StepCount);
        }
		delayMS(nSpeed);
	}

	return 0;
}


int PTZ_AutoRotate_AllDir(int nDir_LR, int nDir_UD, int nSpeed) 
{
	int i = 0;
	
	for (i = 0; i < 4; i++) 
	{
        /************************************************
         ****** Left & Right direction auto rotate ******
         ************************************************/
		if (CLOCKWISE == nDir_LR) //dir:R --> L.
		{
            MotorDir_Right_To_Left(i);

            if (0 == g_LR_LimitFlag) //with limit switch.
            {
                if (0 == PTZ_LimitSwitch_Left()) //hit left limit switch.
                {
                    g_LR_StepCount = 0; //reset the step counter.
                    PTZ_DEBUG_PRT("left hit...g_LR_LimitFlag:%d, g_LRStepLength:%d, g_LR_StepCount:%d\n", g_LR_LimitFlag, g_LRStepLength, g_LR_StepCount);
                    g_RotateDir_LR = COUNTER_CLOCKWISE; //change dir: left --> right.
                    break;
                }
            }
            else if (1 == g_LR_LimitFlag) //without limit switch.
            {
                if (0 == g_LR_StepCount) 
                {
                    //PTZ_DEBUG_PRT("left......g_LR_LimitFlag:%d, g_LRStepLength:%d, g_LR_StepCount:%d\n", g_LR_LimitFlag, g_LRStepLength, g_LR_StepCount);
                    g_RotateDir_LR = COUNTER_CLOCKWISE; //change dir: left --> right.
                    break;
                }
            }
            
            g_LR_StepCount --; //count current motor position.
            //PTZ_DEBUG_PRT("g_LR_LimitFlag:%d, g_LRStepLength:%d, g_LR_StepCount:%d\n", g_LR_LimitFlag, g_LRStepLength, g_LR_StepCount);
		}
		else if (COUNTER_CLOCKWISE == nDir_LR) //dir:L --> R.
		{
            MotorDir_Left_To_Right(i);

            if (0 == g_LR_LimitFlag) //with limit switch.
            {
                if (0 == PTZ_LimitSwitch_Right()) //hit right limit switch.
                {
                    PTZ_DEBUG_PRT("right hit...g_LR_LimitFlag:%d, g_LRStepLength:%d, g_LR_StepCount:%d\n", g_LR_LimitFlag, g_LRStepLength, g_LR_StepCount);
                    g_RotateDir_LR = CLOCKWISE; //change dir: right --> left.
                    break;
                }
            }
            else if (1 == g_LR_LimitFlag) //without limit switch.
            {
                if (g_LR_StepCount == PTZ_RANGE_LR) 
                {
                    //PTZ_DEBUG_PRT("right......g_LR_LimitFlag:%d, g_LRStepLength:%d, g_LR_StepCount:%d\n", g_LR_LimitFlag, g_LRStepLength, g_LR_StepCount);
                    g_RotateDir_LR = CLOCKWISE; //change dir: left --> right.
                    break;
                }
            }

            g_LR_StepCount ++; //count current motor position.
            //PTZ_DEBUG_PRT("g_LR_LimitFlag:%d, g_LRStepLength:%d, g_LR_StepCount:%d\n", g_LR_LimitFlag, g_LRStepLength, g_LR_StepCount);
		}

        /*********************************************
         ****** Up & Down direction auto rotate ******
         *********************************************/
        if (CLOCKWISE == nDir_UD) //dir: D --> U.
		{			
            MotorDir_Down_To_Up(i);

            if (0 == g_UD_LimitFlag) //with limit switch.
            {
                if (0 == PTZ_LimitSwitch_Up()) //hit up limit switch.
                {
                    g_UD_StepCount = 0; //reset step counter.
                    PTZ_DEBUG_PRT("up hit...g_UD_LimitFlag:%d, g_UDStepLength:%d, g_UD_StepCount:%d\n", g_UD_LimitFlag, g_UDStepLength, g_UD_StepCount);
                    g_RotateDir_UD = COUNTER_CLOCKWISE; //change dir: up --> down.
                    break;
                }
            }
            else if (1 == g_UD_LimitFlag) //without limit switch.
            {
                if (0 == g_UD_StepCount) 
                {
                    //PTZ_DEBUG_PRT("up......g_UD_LimitFlag:%d, g_UDStepLength:%d, g_UD_StepCount:%d\n", g_UD_LimitFlag, g_UDStepLength, g_UD_StepCount);
                    g_RotateDir_UD = COUNTER_CLOCKWISE; //change dir: up --> down.
                    break;
                }
            }

            g_UD_StepCount --;
            //PTZ_DEBUG_PRT("g_UD_LimitFlag:%d, g_UDStepLength:%d, g_UD_StepCount:%d\n", g_UD_LimitFlag, g_UDStepLength, g_UD_StepCount);
		}	
        else if (COUNTER_CLOCKWISE == nDir_UD) //dir: U --> D
		{
            MotorDir_Up_To_Down(i);

            if (0 == g_UD_LimitFlag) //with limit switch.
            {
                if (0 == PTZ_LimitSwitch_Down()) //hit down limit switch.
                {
                    PTZ_DEBUG_PRT("down hit...g_UD_LimitFlag:%d, g_UDStepLength:%d, g_UD_StepCount:%d\n", g_UD_LimitFlag, g_UDStepLength, g_UD_StepCount);
                    g_RotateDir_UD = CLOCKWISE; //change dir: down --> up.
                    break;
                }
            }
            else if (1 == g_UD_LimitFlag) //without limit switch.
            {
                if (g_UD_StepCount == PTZ_RANGE_UD) 
                {
                    //PTZ_DEBUG_PRT("down......g_UD_LimitFlag:%d, g_UDStepLength:%d, g_UD_StepCount:%d\n", g_UD_LimitFlag, g_UDStepLength, g_UD_StepCount);
                    g_RotateDir_UD = CLOCKWISE; //change dir: down --> up.
                    break;

                }
            }

            g_UD_StepCount ++;
            //PTZ_DEBUG_PRT("g_UD_LimitFlag:%d, g_UDStepLength:%d, g_UD_StepCount:%d\n", g_UD_LimitFlag, g_UDStepLength, g_UD_StepCount);
		}		
		delayMS(nSpeed);
	}
    return 0;
}

int PTZ_Rotate_Left(unsigned char count)
{
	int i = 0, k = 0;

    if ((0 == g_LR_LimitFlag) && (0 == PTZ_LimitSwitch_Left())) //hit left limit switch.
    {
		usleep(100*1000);
        return 0; 
    }
	
	for (k = 0; k < count; k ++)
	{		
		for (i = 0; i < 4; i++) 
		{
            MotorDir_Right_To_Left(i);

            if (0 == g_LR_LimitFlag) //with limit switch.
            {
                if (0 == PTZ_LimitSwitch_Left())
                {
                    g_LR_StepCount = 0; //reset step counter.
                    PTZ_DEBUG_PRT("left hit...g_LR_LimitFlag:%d g_LR_StepCount:%d\n", g_LR_LimitFlag, g_LR_StepCount);
                    return 0; 
                }
            }
            else if (1 == g_LR_LimitFlag) //without limit switch.
            {
                if (0 == g_LR_StepCount)                 
                {
                    //PTZ_DEBUG_PRT("left......g_LR_LimitFlag:%d g_LR_StepCount:%d\n", g_LR_LimitFlag, g_LR_StepCount);
                    return 0; 
                }
            }

            g_LR_StepCount --;
            //PTZ_DEBUG_PRT("g_LR_LimitFlag:%d, g_LRStepLength:%d, g_LR_StepCount:%d\n", g_LR_LimitFlag, g_LRStepLength, g_LR_StepCount);
            delayMS(10);
		}				
	}
	
	return 0;
}

int PTZ_Rotate_Right(unsigned char count)
{
	int i = 0, k = 0;

    if ((0 == g_LR_LimitFlag) && (0 == PTZ_LimitSwitch_Right())) //hit right limit switch.
    {
		usleep(100*1000);
        return 0; 
    }

	for (k = 0; k < count; k++)
	{		
		for (i = 0; i < 4; i++) 
		{
            MotorDir_Left_To_Right(i);

            if (0 == g_LR_LimitFlag) //with limit switch.
            {
                if (0 == PTZ_LimitSwitch_Right()) //hit right limit switch.
                {
                    PTZ_DEBUG_PRT("right hit...g_LR_LimitFlag:%d g_LR_StepCount:%d\n", g_LR_LimitFlag, g_LR_StepCount);
                    return 0; 
                }
            }
            else if (1 == g_LR_LimitFlag)
            {
                if (g_LR_StepCount == PTZ_RANGE_LR)                 
                {
                    //PTZ_DEBUG_PRT("right......g_LR_LimitFlag:%d g_LR_StepCount:%d\n", g_LR_LimitFlag, g_LR_StepCount);
                    return 0; 
                }
            }

            g_LR_StepCount ++;
            //PTZ_DEBUG_PRT("g_LR_LimitFlag:%d, g_LRStepLength:%d, g_LR_StepCount:%d\n", g_LR_LimitFlag, g_LRStepLength, g_LR_StepCount);
            delayMS(10);
		}
	}
	
	return 0;
}

int PTZ_Rotate_Up(unsigned char count)
{
	int i = 0, k = 0;
	
    if ((0 == g_UD_LimitFlag) && (0 == PTZ_LimitSwitch_Up())) //hit up limit switch.
    {
		usleep(100*1000);
        return 0;
    }
	
	for (k = 0; k < count; k++)
	{		
		for (i = 0; i < 4; i++) 
		{
            MotorDir_Down_To_Up(i);

            if (0 == g_UD_LimitFlag) //with limit switch.
            {
                if (0 == PTZ_LimitSwitch_Up()) //hit up limit switch.
                {
                    g_UD_StepCount = 0; //reset step counter.
                    PTZ_DEBUG_PRT("up hit...g_UD_LimitFlag:%d, g_UD_StepCount:%d\n", g_UD_LimitFlag, g_UD_StepCount);
                    return 0;
                }
            }
            else if (1 == g_UD_LimitFlag) //without limit switch.
            {
                if (0 == g_UD_StepCount)            
                {
                    //PTZ_DEBUG_PRT("up......g_UD_LimitFlag:%d, g_UD_StepCount:%d\n", g_UD_LimitFlag, g_UD_StepCount);
                    return 0;
                }
            }

            g_UD_StepCount --;
            //PTZ_DEBUG_PRT("g_UD_LimitFlag:%d, g_UDStepLength:%d, g_UD_StepCount:%d\n", g_UD_LimitFlag, g_UDStepLength, g_UD_StepCount);
			delayMS(10);
		}		
	}
	
	return 0;
}

int PTZ_Rotate_Down(unsigned char count)
{	
	int i = 0, k = 0;
	
    if ((0 == g_UD_LimitFlag) && (0 == PTZ_LimitSwitch_Down())) //hit down limit switch.
    {
		usleep(100*1000);
        return 0;
    }

	for (k = 0; k < count; k++)
	{		
		for (i = 0; i < 4; i++) 
		{
            MotorDir_Up_To_Down(i);

            if (0 == g_UD_LimitFlag) //with limit switch.
            {
                if (0 == PTZ_LimitSwitch_Down()) //hit down limit switch.
                {
                    PTZ_DEBUG_PRT("down hit...g_UD_LimitFlag:%d, g_UD_StepCount:%d\n", g_UD_LimitFlag, g_UD_StepCount);
                    return 0;
                }
            }
            else if (1 == g_UD_LimitFlag) //without limit switch.
            {
                if (g_UD_StepCount == PTZ_RANGE_UD)            
                {
                    //PTZ_DEBUG_PRT("down......g_UD_LimitFlag:%d, g_UD_StepCount:%d\n", g_UD_LimitFlag, g_UD_StepCount);
                    return 0;
                }
            }

            g_UD_StepCount ++;
            //PTZ_DEBUG_PRT("g_UD_LimitFlag:%d, g_UDStepLength:%d, g_UD_StepCount:%d\n", g_UD_LimitFlag, g_UDStepLength, g_UD_StepCount);
			delayMS(10);
		}		
	}
	return 0;
}

#if 1
/*****************************************************
 * func: motor rotate to setted preset position.
 *****************************************************/
//int PTZ_RotateToPresetPos(int nDir_LR, int nDir_UD, int nStepX, int nStepY, int nPos, int nSpeed) 
int PTZ_RotateToPresetPos(int nDir_LR, int nDir_UD, int nPos, int nSpeed) 
{
	int i = 0, j = 0;
    unsigned int pins[8] = {0, 1, 2, 3, 4, 5, 6, 7}; //GPIO9_0 ~ 9_7.
	unsigned int groupnum = 9;
	unsigned int bitnum = 0;
	unsigned int val_set = 0; //pull down.
    unsigned int LR_PresetFlag = 0, UD_PresetFlag = 0; //1: arrived at preset position.
    
    PTZ_DEBUG_PRT("nDir_LR:%d, nDir_UD:%d, nPos:%d, presetX:%d, presetY:%d, g_LR_StepCount:%d,  g_UD_StepCount:%d, g_LR_LimitFlag:%d, g_UD_LimitFlag:%d\n", nDir_LR, nDir_UD, nPos, g_stPtzPreset[nPos].presetX, g_stPtzPreset[nPos].presetY, g_LR_StepCount,  g_UD_StepCount, g_LR_LimitFlag, g_UD_LimitFlag);

    g_RotateRunning = 1; //enable motor rotate.

    while (g_RotateRunning)
    {
        for (i = 0; i < 4; i++) 
        {
            if ((1 == LR_PresetFlag) && (1 == UD_PresetFlag)) //arrived at preset position,stop ptz.
            {
                g_RotateRunning = 0; //disable motor rotate thread.
                g_PtzRotateEnable = 0; //stop motor rotate.
                return 0;
            }

            if (g_stPtzPreset[nPos].presetX == g_LR_StepCount)
            {
                LR_PresetFlag = 1; //left & right direction arrived at the preset position. 
            }

            if (0 == LR_PresetFlag)
            {
                if (CLOCKWISE == nDir_LR) //dir:R-->L.
                {
                    MotorDir_Right_To_Left(i);

                    if (0 == g_LR_LimitFlag) //with limit switch.
                    {
                        if (0 == PTZ_LimitSwitch_Left()) //hit left limit switch.
                        {
                            break; 
                        }
                    }
                    else if (1 == g_LR_LimitFlag) //without limit switch.
                    {
                    
                    }

                    g_LR_StepCount ++; //count motor position.
                    //PTZ_DEBUG_PRT("g_LR_LimitFlag:%d, g_LRStepLength:%d, g_LR_StepCount:%d\n", g_LR_LimitFlag, g_LRStepLength, g_LR_StepCount);
                }
                else if (COUNTER_CLOCKWISE == nDir_LR) //dir:L-->R.
                {
                    MotorDir_Left_To_Right(i);

                    if (0 == g_LR_LimitFlag) //with limit switch.
                    {
                        if (0 == PTZ_LimitSwitch_Right()) //hit right limit switch.
                        {
                            break; 
                        }
                    }
                    else if (1 == g_LR_LimitFlag) //without limit switch.
                    {
                    
                    }
                }
            }

            if (g_stPtzPreset[nPos].presetY == g_UD_StepCount)
            {
                UD_PresetFlag = 1; //up & down direction arrived at the preset position. 
            }

            if (0 == UD_PresetFlag)
            {
                if (CLOCKWISE == nDir_UD) //dir:D-->U.
                {
                    MotorDir_Down_To_Up(i);

                    if (1 == PTZ_LimitSwitch_Up()) //hit down limit switch.
                    {
                        break; 
                    }
                }	
                else if (COUNTER_CLOCKWISE == nDir_UD) //dir:U-->D.
                {			
                    MotorDir_Up_To_Down(i);

                    if (1 == PTZ_LimitSwitch_Down()) //hit up limit switch.
                    {
                        break; 
                    }
                }
            }
            delayMS(nSpeed);
        }
        delayMS(nSpeed);
    }
    return 0;
}
#endif


int PTZ_Rotate_Init(void)
{
    int i = 0;

    g_tmPTZStart = 0;
    g_tmPTZStop = 0;  

    g_PtzRotateEnable = 0;
    g_PtzRotateType = 0;
    g_PtzStepType = 0;
    g_PtzPresetPos = 0;

    g_LR_LimitFlag = 0; //default with limit switch.
    g_UD_LimitFlag = 0;

    g_RotateRunning = 0;
    g_LR_AutoRotateCount = 0;
    g_UD_AutoRotateCount = 0;
    g_UD_StepCount = 0; //calculate the whole length from top to bottom.
    g_LR_StepCount = 0; //calculate the whole lenght from left to right.
    g_UDStepLength = 0; //up to down step distance.
    g_LRStepLength = 0; //left to right step distance.
	g_RotateDir_LR = CLOCKWISE; //dir: left first.
	g_RotateDir_UD = CLOCKWISE; //dir: up first.

	g_RotateSpeed = Get_PTZ_RotateSpeed(); //motor speed.

    /**get ptz preset params**/
    memset((void *)g_stPtzPreset, 0, sizeof(st_PtzPreset)*PRESETNUM);
    for (i = 1; i < PRESETNUM + 1; i++)
    {
        //memset(&g_stPtzPreset[i], 0, sizeof(st_PtzPreset));
        Get_PTZ_PresetParams(i, &g_stPtzPreset[i].presetX, &g_stPtzPreset[i].presetY);
        PTZ_DEBUG_PRT("g_stPtzPreset[%d].presetX:%d, g_stPtzPreset[%d].presetY:%d\n", i, g_stPtzPreset[i].presetX, i, g_stPtzPreset[i].presetY);
    }

    return 0;
}

/************************************************************************
 * func: PTZ motor auto rotate on direction left & right & up & down.
 *       left & right: GPIO9_0 ~ GPIO9_3.
 *       up & down   : GPIO9_4 ~ GPIO9_7.
 *       default direction: from right to left, and bottom to top side.
 ***********************************************************************/
int PTZ_UDLR_AutoRotate(int nPos, int nSpeed)
{		
    int i = 0;
	int pins[8] = {0, 1, 2, 3, 4, 5, 6, 7};	
    unsigned int groupnum = 9;
    unsigned int bitnum = 0;
    unsigned int val_set = 0; //0: pull down; 1: pull up.
    unsigned int LR_StopFlag = 0;
    unsigned int UD_StopFlag = 0;
    //int Dev_WI8812 = 0;

    if ((g_stPtzPreset[nPos].presetX <= 0) || (g_stPtzPreset[nPos].presetX > PTZ_RANGE_LR))
    {
        g_stPtzPreset[nPos].presetX = PTZ_RANGE_LR / 2; //default L&R preset position.
    }
    if ((g_stPtzPreset[nPos].presetY <= 0) || (g_stPtzPreset[nPos].presetY > PTZ_RANGE_UD))
    {
        g_stPtzPreset[nPos].presetY = PTZ_RANGE_UD / 2; //default U&D preset position.
    }
    printf("...motor auto rotate start, nPos:%d, presetX:%d, presetY:%d, g_LR_LimitFlag:%d, g_UD_LimitFlag:%d\n", nPos, g_stPtzPreset[nPos].presetX, g_stPtzPreset[nPos].presetY, g_LR_LimitFlag, g_UD_LimitFlag);

    g_RotateRunning = 1; //enable auto rotate.

	while (g_RotateRunning)
	{	
		if ((1 == UD_StopFlag) && (1 == LR_StopFlag))
		{
        #if 0
            if ((1 == g_UD_LimitFlag) && (0 == g_LR_LimitFlag)) //for WI8812.
            {
                Dev_WI8812 = 1;
                printf(".........Dev_WI8812: %d.........\n", Dev_WI8812);
                while(Dev_WI8812)
                {
                    for (i = 0; i < 4; i++) 
                    {
                        MotorDir_Up_To_Down(i);
                        g_UD_StepCount ++; //count motor position.
                        if ((g_stPtzPreset[nPos].presetY + 200) == g_UD_StepCount)
                        {
                            Dev_WI8812 = 0;
                            PTZ_DEBUG_PRT("Dev_WI8812:%d, g_UD_AutoRotateCount:%d, g_UD_LimitFlag:%d, g_UDStepLength:%d, g_UD_StepCount:%d\n", Dev_WI8812, g_UD_AutoRotateCount, g_UD_LimitFlag, g_UDStepLength, g_UD_StepCount);
                            break;
                        }
                    }
                }
            }
            Dev_WI8812 = 0;
        #endif

            g_PtzPresetPos = 0; //disable preset rotate.
            g_RotateRunning = 0; //quit auto rotate.
            PTZ_AutoRotate_Stop(pins);
            PTZ_AutoRotate_Stop(pins + 4);
            //printf("......camera auto rotate done, g_LR_StepCount:%d, g_UD_StepCount:%d......\n", g_LR_StepCount, g_UD_StepCount);
			break;
		}

        for (i = 0; i < 4; i++) 
        {
            /*********************************************************
             ****** left & right auto rotate: GPIO9_0 ~ GPIO9_3 ******
             *********************************************************/
            //if ((3 == g_LR_AutoRotateCount) && (g_stPtzPreset[nPos].presetX == g_LR_StepCount))
            if ((2 == g_LR_AutoRotateCount) && (g_stPtzPreset[nPos].presetX == g_LR_StepCount))
            {
                LR_StopFlag = 1;
            }

            if (0 == LR_StopFlag)
            {
                if (CLOCKWISE == g_RotateDir_LR) //cur dir: right --> left. 
                {
                    MotorDir_Right_To_Left(i);

                    if (0 == g_LR_LimitFlag)
                    {
                        if (0 == PTZ_LimitSwitch_Left()) //hit left limit switch.
                        {
                            g_LR_LimitFlag = 0; //there exists limit switch. 
                            g_LR_AutoRotateCount ++;
                            g_RotateDir_LR = COUNTER_CLOCKWISE; //change dir: left --> right.

                            if (1 == g_LR_AutoRotateCount)
                            {
                                g_LR_StepCount = 0; //reset the step length the first hit time.
                            }
                            PTZ_DEBUG_PRT("left hit...g_LR_AutoRotateCount:%d, g_LRStepLength:%d, g_LR_StepCount:%d, g_LR_LimitFlag:%d\n", g_LR_AutoRotateCount, g_LRStepLength, g_LR_StepCount, g_LR_LimitFlag);

                            break;
                        }
                    }

                    //if ((0 == g_LR_LimitFlag) && (0 == g_LR_AutoRotateCount))
                    if (0 == g_LR_AutoRotateCount)
                    {
                        g_LR_StepCount ++; //count current motor position.
                        //PTZ_DEBUG_PRT("g_LR_AutoRotateCount:%d, g_LR_LimitFlag:%d, g_LRStepLength:%d, g_LR_StepCount:%d\n", g_LR_AutoRotateCount, g_LR_LimitFlag, g_LRStepLength, g_LR_StepCount);

                        if (g_LR_StepCount >= PTZ_RANGE_LR)
                        {
                            printf(".......... L&R without limit switch ...........\n");
                            g_LR_LimitFlag = 1; //there's no limit switch existed. 
                            g_LR_StepCount = 0;
                            g_LR_AutoRotateCount = 2; //3;
                            g_RotateDir_LR = COUNTER_CLOCKWISE; //change dir: left --> right.
                            break;
                        }
                    }
                    //else if ((0 == g_LR_LimitFlag) && (2 <= g_LR_AutoRotateCount))
                    else if (2 <= g_LR_AutoRotateCount)
                    {
                        g_LR_StepCount --; //count motor position.
                    }
                }
                else if (COUNTER_CLOCKWISE == g_RotateDir_LR) //cur dir: left --> right.
                {
                    MotorDir_Left_To_Right(i);

                    if (0 == g_LR_LimitFlag)
                    {
                        if (0 == PTZ_LimitSwitch_Right()) //hit right limit switch.
                        {	
                            g_LR_LimitFlag = 0; //there exists limit switch. 
                            g_LRStepLength = g_LR_StepCount; //save the distance from left to right.
                            g_stPtzPreset[nPos].presetX = g_LRStepLength / 2;
                            g_LR_AutoRotateCount ++;
                            g_RotateDir_LR = CLOCKWISE; //change dir: right --> left.
                            PTZ_DEBUG_PRT("right hit...g_LR_AutoRotateCount:%d, g_LRStepLength:%d, g_LR_StepCount:%d, g_LR_LimitFlag:%d, g_stPtzPreset[nPos].presetX:%d\n", g_LR_AutoRotateCount, g_LRStepLength, g_LR_StepCount, g_LR_LimitFlag, g_stPtzPreset[nPos].presetX);
                            break; 
                        }
                    }

                    g_LR_StepCount ++; //count motor position.
                    //PTZ_DEBUG_PRT("g_LR_AutoRotateCount:%d, g_LR_LimitFlag:%d, g_LRStepLength:%d, g_LR_StepCount:%d\n", g_LR_AutoRotateCount, g_LR_LimitFlag, g_LRStepLength, g_LR_StepCount);
                }
            }

            /******************************************************
             ****** up & down auto rotate: GPIO9_4 ~ GPIO9_7 ******
             ******************************************************/
            //if ((3 == g_UD_AutoRotateCount) && (g_stPtzPreset[nPos].presetY == g_UD_StepCount))
            if ((2 == g_UD_AutoRotateCount) && (g_stPtzPreset[nPos].presetY == g_UD_StepCount))
            {
                UD_StopFlag = 1;
            }

            if (0 == UD_StopFlag)
            {
                if (CLOCKWISE == g_RotateDir_UD) //cur dir: down --> up.
                {
                    MotorDir_Down_To_Up(i);

                    if (0 == g_UD_LimitFlag)
                    {
                        if (0 == PTZ_LimitSwitch_Up()) //hit down limit switch.
                        {
                            g_UD_LimitFlag = 0; //there exists limit switch.
                            g_UD_AutoRotateCount ++;
                            g_RotateDir_UD = COUNTER_CLOCKWISE; //change dir: up --> down.

                            if (1 == g_UD_AutoRotateCount) 
                            {
                                g_UD_StepCount = 0; //reset the step length value the first hit time.
                            }
                            PTZ_DEBUG_PRT("up hit...g_UD_AutoRotateCount:%d, g_UDStepLength:%d, g_UD_StepCount:%d, g_UD_LimitFlag:%d\n", g_UD_AutoRotateCount, g_UDStepLength, g_UD_StepCount, g_UD_LimitFlag);
                            break; 
                        }
                    }

                    //if ((0 == g_UD_LimitFlag) && (0 == g_UD_AutoRotateCount)) 
                    if (0 == g_UD_AutoRotateCount) 
                    {
                        g_UD_StepCount ++; //count motor position.
                        //PTZ_DEBUG_PRT("g_UD_AutoRotateCount:%d, g_UD_LimitFlag:%d, g_UDStepLength:%d, g_UD_StepCount:%d\n", g_UD_AutoRotateCount, g_UD_LimitFlag, g_UDStepLength, g_UD_StepCount);

                        if (g_UD_StepCount >= PTZ_RANGE_UD)
                        {
                            printf(".......... U&D without limit switch ...........\n");
                            g_UD_LimitFlag = 1; //there's no limit switch.
                            g_UD_StepCount = 0;
                            g_UD_AutoRotateCount = 2; //3;
                            g_RotateDir_UD = COUNTER_CLOCKWISE; //change dir: up --> down.
                            break;
                        }
                    }
                    //else if ((0 == g_UD_LimitFlag) && (2 <= g_UD_AutoRotateCount))
                    else if (2 <= g_UD_AutoRotateCount)
                    {
                        g_UD_StepCount --; //count motor position.
                    }
                }	
                else if (COUNTER_CLOCKWISE == g_RotateDir_UD) //cur dir: up --> down.
                {			
                    MotorDir_Up_To_Down(i);

                    if (0 == g_UD_LimitFlag)
                    {
                        if (0 == PTZ_LimitSwitch_Down()) //hit up limit switch.
                        {
                            g_UD_LimitFlag = 0; //there exists limit switch.
                            g_UDStepLength = g_UD_StepCount; //save the distance from up to down.
                            g_stPtzPreset[nPos].presetY = g_UDStepLength / 2;
                            g_UD_AutoRotateCount ++;
                            g_RotateDir_UD = CLOCKWISE; //change direction: down to up.
                            PTZ_DEBUG_PRT("down hit...g_UD_AutoRotateCount:%d, g_UDStepLength:%d, g_UD_StepCount:%d, g_UD_LimitFlag:%d, g_stPtzPreset[nPos].presetY:%d\n", g_UD_AutoRotateCount, g_UDStepLength, g_UD_StepCount, g_UD_LimitFlag, g_stPtzPreset[nPos].presetY);
                            break; 
                        }
                    }

                    g_UD_StepCount ++; //count motor position.
                    //PTZ_DEBUG_PRT("g_UD_AutoRotateCount:%d, g_UD_LimitFlag:%d, g_UDStepLength:%d, g_UD_StepCount:%d\n", g_UD_AutoRotateCount, g_UD_LimitFlag, g_UDStepLength, g_UD_StepCount);
                }
            }
            delayMS(nSpeed);
        }
        delayMS(nSpeed);
    }
    return 0;
}


void * HK_Ptz()
{
    int i = 0;
	int pins[8] = {0, 1, 2, 3, 4, 5, 6, 7};	
    printf("========> PTZ thread start <========\n");

    PTZ_Rotate_Init();
    PTZ_DEBUG_PRT("g_RotateDir_LR:%d, g_RotateDir_UD:%d, g_RotateSpeed:%d, g_LR_AutoRotateCount:%d, g_UD_AutoRotateCount:%d, g_UDStepLength:%d, g_LRStepLength:%d\n", g_RotateDir_LR, g_RotateDir_UD, g_RotateSpeed, g_LR_AutoRotateCount, g_UD_AutoRotateCount, g_UDStepLength, g_LRStepLength);
 
    /**rotate to preset position on system restart**/
    g_PtzPresetPos = 1; //preset1.
	PTZ_UDLR_AutoRotate(g_PtzPresetPos, g_RotateSpeed); //motor auto cruise on system restart.

    /**rotate control by client**/
	g_RotateDir_LR = CLOCKWISE;
	g_RotateDir_UD = CLOCKWISE;
    PTZ_DEBUG_PRT("......camera auto rotate done, g_RotateRunning:%d, g_RotateDir_LR:%d, g_RotateDir_UD:%d, g_RotateSpeed:%d, g_LR_AutoRotateCount:%d, g_UD_AutoRotateCount:%d, g_UDStepLength:%d, g_LRStepLength:%d, g_PtzRotateEnable:%d, g_PtzRotateType:%d, g_PtzStepType:%d, g_PtzPresetPos:%d, g_LR_LimitFlag:%d, g_UD_LimitFlag:%d, g_LR_StepCount:%d, g_UD_StepCount:%d\n", g_RotateRunning, g_RotateDir_LR, g_RotateDir_UD, g_RotateSpeed, g_LR_AutoRotateCount, g_UD_AutoRotateCount, g_UDStepLength, g_LRStepLength, g_PtzRotateEnable, g_PtzRotateType, g_PtzStepType, g_PtzPresetPos, g_LR_LimitFlag, g_UD_LimitFlag, g_LR_StepCount, g_UD_StepCount);

    while (1)
    {	
        if (1 == g_PtzRotateEnable) //enable ptz rotate.
        {
            switch (g_PtzRotateType)//type ==> 1:leftright; 2:updown; 3:all direction auto rotate.
            {
                case 1:					
                    PTZ_AutoRotate_LeftRight(g_RotateDir_LR, g_RotateSpeed);
                    break;
                case 2:					
                    PTZ_AutoRotate_UpDown(g_RotateDir_UD, g_RotateSpeed);
                    break;
                case 3:					
                    PTZ_AutoRotate_AllDir(g_RotateDir_LR, g_RotateDir_UD, g_RotateSpeed);					
                    break;
                default:
                    break;
            }

            switch (g_PtzStepType)//step by step rorate.
            {
                case 1:
                    PTZ_Rotate_Left(3);
                    break;
                case 2:
                    PTZ_Rotate_Right(3);
                    break;
                case 3:
                    PTZ_Rotate_Up(3);
                    break;
                case 4:
                    PTZ_Rotate_Down(3);
                    break;
                default:
                    break;
            }

            switch (g_PtzPresetPos)//goto preset position.
            {
                case 1:	
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                    {
                        /**get ptz preset params**/
                    #if 0
                        memset((void *)g_stPtzPreset, 0, sizeof(st_PtzPreset)*PRESETNUM);
                        for (i = 1; i < PRESETNUM + 1; i++)
                        {
                            //memset(&g_stPtzPreset[i], 0, sizeof(st_PtzPreset));
                            Get_PTZ_PresetParams(i, &g_stPtzPreset[i].presetX, &g_stPtzPreset[i].presetY);
                            PTZ_DEBUG_PRT("g_stPtzPreset[%d].presetX:%d, g_stPtzPreset[%d].presetY:%d\n", i, g_stPtzPreset[i].presetX, i, g_stPtzPreset[i].presetY);
                        }
                    #else
                        Get_PTZ_PresetParams(g_PtzPresetPos, &g_stPtzPreset[g_PtzPresetPos].presetX, &g_stPtzPreset[g_PtzPresetPos].presetY);
                        PTZ_DEBUG_PRT("g_stPtzPreset[%d].presetX:%d, g_stPtzPreset[%d].presetY:%d, g_LR_StepCount:%d, g_UD_StepCount:%d\n", g_PtzPresetPos, g_stPtzPreset[g_PtzPresetPos].presetX, g_PtzPresetPos, g_stPtzPreset[g_PtzPresetPos].presetY, g_LR_StepCount, g_UD_StepCount);
                    #endif

                        if (g_stPtzPreset[g_PtzPresetPos].presetX > g_LR_StepCount)
                        {
                            g_RotateDir_LR = COUNTER_CLOCKWISE;	//change dir:L-->R, g_LR_StepCount++.
                        }
                        else if (g_stPtzPreset[g_PtzPresetPos].presetX < g_LR_StepCount)
                        {				
                            g_RotateDir_LR = CLOCKWISE;	//change dir:R-->L, g_LR_StepCount--.
                        }

                        if (g_stPtzPreset[g_PtzPresetPos].presetY > g_UD_StepCount)
                        {
                            g_RotateDir_UD = COUNTER_CLOCKWISE;	//change dir:U-->D, g_UD_StepCount++.
                        }
                        else if (g_stPtzPreset[g_PtzPresetPos].presetY < g_UD_StepCount)
                        {				
                            g_RotateDir_UD = CLOCKWISE;	//change dir: D-->U, g_UD_StepCount--.
                        }
                        //PTZ_DEBUG_PRT("g_PtzPresetPos:%d, g_RotateDir_LR:%d, g_RotateDir_UD:%d, presetX:%d, presetY:%d, g_LR_StepCount:%d,  g_UD_StepCount:%d\n", g_PtzPresetPos, g_RotateDir_LR, g_RotateDir_UD, g_stPtzPreset[g_PtzPresetPos].presetX, g_stPtzPreset[g_PtzPresetPos].presetY, g_LR_StepCount,  g_UD_StepCount);
                    #if 0
                        PTZ_RotateToPresetPos(g_RotateDir_LR, g_RotateDir_UD, g_PtzPresetPos, g_RotateSpeed);
                    #else
                        g_LR_AutoRotateCount = 2; //3;
                        g_UD_AutoRotateCount = 2; //3;
                        PTZ_UDLR_AutoRotate(g_PtzPresetPos, g_RotateSpeed);
                    #endif
                    }
                    break;

                default:
                    break;
            }
        }
        else if (0 == g_PtzRotateEnable)
        {
            printf("-------------ptz stop-------------\n");
            PTZ_Rotate_Stop(pins);
            g_PtzRotateEnable = -1; //disable ptz rotate.
            usleep(100*1000);
        }
        else
        {			
            usleep(100*1000);
        }		
    }

    return NULL;
}


int HK_PTZ_AutoRotate_Stop(int nPos)
{
    if ((1 == g_PtzRotateEnable) && (g_PtzRotateType != 0))
    {
        time(&g_tmPTZStop);
        //printf("...%s...nPos:%d, g_tmPTZStop:%u, g_tmPTZStart:%u\n", __func__, nPos, g_tmPTZStop, g_tmPTZStart);
        //if (g_tmPTZStop - g_tmPTZStart > 1*60) //ptz auto rotate between 3 minutes. //test
        if (g_tmPTZStop - g_tmPTZStart > 30*60) //ptz auto rotate between 30 minutes.
        {
            printf("=================== stop =======================\n");
            g_tmPTZStart = 0;
            g_tmPTZStop = 0;  
            g_PtzRotateEnable = 0;
            g_PtzRotateType = 0;
            g_PtzStepType = 0;	

            Get_PTZ_PresetParams(nPos, &g_stPtzPreset[nPos].presetX, &g_stPtzPreset[nPos].presetY);
            PTZ_DEBUG_PRT("g_stPtzPreset[%d].presetX:%d, g_stPtzPreset[%d].presetY:%d, g_LR_StepCount:%d, g_UD_StepCount:%d\n", nPos, g_stPtzPreset[nPos].presetX, nPos, g_stPtzPreset[nPos].presetY, g_LR_StepCount, g_UD_StepCount);

            if (g_stPtzPreset[nPos].presetX >= g_LR_StepCount)
            {
                g_RotateDir_LR = COUNTER_CLOCKWISE;	//change dir:L-->R, g_LR_StepCount++.
            }
            else if (g_stPtzPreset[nPos].presetX <= g_LR_StepCount)
            {				
                g_RotateDir_LR = CLOCKWISE;	//change dir:R-->L, g_LR_StepCount--.
            }

            if (g_stPtzPreset[nPos].presetY >= g_UD_StepCount)
            {
                g_RotateDir_UD = COUNTER_CLOCKWISE;	//change dir:U-->D, g_UD_StepCount++.
            }
            else if (g_stPtzPreset[nPos].presetY <= g_UD_StepCount)
            {				
                g_RotateDir_UD = CLOCKWISE;	//change dir: D-->U, g_UD_StepCount--.
            }
            //PTZ_DEBUG_PRT("nPos:%d, g_RotateDir_LR:%d, g_RotateDir_UD:%d, presetX:%d, presetY:%d, g_LR_StepCount:%d,  g_UD_StepCount:%d\n", nPos, g_RotateDir_LR, g_RotateDir_UD, g_stPtzPreset[nPos].presetX, g_stPtzPreset[nPos].presetY, g_LR_StepCount,  g_UD_StepCount);

            g_LR_AutoRotateCount = 2; //3;
            g_UD_AutoRotateCount = 2; //3;
            PTZ_UDLR_AutoRotate(nPos, g_RotateSpeed);
        }
    }
}


int HK_PtzMotor()
{
	int nRet = -1;
	 
	HK_CREATE_THREADEX(HK_Ptz, NULL, nRet);

	return nRet;
}

