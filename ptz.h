#ifndef __PTZ_H__
#define __PTZ_H__


extern int g_RotateSpeed;		//rotate speed
extern int g_UD_StepCount;		//calculate the UP down current position.
extern int g_LR_StepCount;		//calculate the left rightcurrent position.
extern int g_PtzRotateEnable;	//ptz rotate flag, 1:enable rotate, 0:ptz stop
extern int g_PtzRotateType;		//type ==> 1:leftright; 2:updown; 3:all direction auto rotate.
extern int g_PtzStepType;		//ptz step, 1:left, 2:right, 3:up, 4:down.
extern int g_PtzPresetPos;		//preset position: 1 ~ 8.	
extern unsigned long g_tmPTZStart;	//PTZ start time;

int HK_PtzMotor();
int HK_PTZ_AutoRotate_Stop(int nPos);
int Set_PTZ_RotateSpeed(int nPtzSpeed);
int Get_PTZ_RotateSpeed(void);
int Set_PTZ_PresetParams(int nValue, int xPos, int yPos);
int Get_PTZ_PresetParams(int nValue, int *xPos, int *yPos);


#endif  /* ptz.h */

