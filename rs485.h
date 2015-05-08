
#define MOVEUP 3
#define MOVEDOWN 2
#define MOVELEFT 1
#define MOVERIGHT 0

#define MOVESTOP 4
#define AUTOSCAN 5

#define IRISOPEN 1
#define IRISCLOSE 0

#define FOCUSFAR 1
#define FOCUSNEAR 0

#define VIEWREDUCE 0
#define VIEWEXTEND 1
	
int  ApiYuntaiInit(int address);
int ApiSetVerticalspeed(int speed);
int ApiSetParallelspeed(int speed);
int ApiSetMoveDir(int state);
int ApiClearCmd(int address);
int ApiSetAutoScan();
int ApiSetFocus(int state);
int ApiSetViewRange(int state);
int ApiSetIris(int state);
int ApiSendCmd( );
int ApiSetPreset(int number);
int ApiClearPreset(int number);
int ApiGoToPreset(int number);
int ApiGoToZero(void);
int ApiFlip(void);

