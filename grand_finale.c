
#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"

#define TOUCH_PORT_ID NXT_PORT_S4
#define PRIO_UNCALIBRATED 99
#define PRIO_IDLE 10
#define PRIO_LIGHT 20
#define LOST_RESET 0

/*--------------------------------------------------------------------------*/
/* OSEK declarations                                                        */
/*--------------------------------------------------------------------------*/
DeclareCounter(SysTimerCnt);
DeclareResource(DC);
DeclareTask(MotorControlTask);
DeclareTask(LightSensorTask);
DeclareTask(DisplayTask);
const int RIGHT = 0;
const int LEFT = 1;
int black = 400;
int reflect=1;
int test=0;

/*--------------------------------------------------------------------------*/
/* Definitions                                                              */
/*--------------------------------------------------------------------------*/
struct dc_t {
	S32 duration;
	S32 speed;
	int priority;
	int dir;
	int prevDir;
	int lost;
} dc = {0, 0, PRIO_UNCALIBRATED, 0, 0,0};

void change_driving_command(int priority, int speed, int duration, int dir, int prevDir, int lost)
{
	if(priority > dc.priority)
	{
		dc.speed = speed;
		dc.duration = duration;
		dc.priority = priority;
		dc.dir = dir;
		dc.prevDir = prevDir;
		dc.lost = lost; //while lost, turning is locked in one direction until line is found. 
		
	}
}

void ecrobot_device_initialize(void)
{
	ecrobot_init_nxtcolorsensor(NXT_PORT_S1, NXT_LIGHTSENSOR_RED); // initialize a sensor
	ecrobot_set_nxtcolorsensor(NXT_PORT_S1, NXT_LIGHTSENSOR_RED);
	ecrobot_init_sonar_sensor(NXT_PORT_S2);
}

void driveRightMotor(int speed)
{
	//nxt_motor_set_count(NXT_PORT_C, 20);
	nxt_motor_set_speed(NXT_PORT_C, speed, 1);
}
void stopRightMotor()
{
	nxt_motor_set_speed(NXT_PORT_C, 0, 0);
}

void driveLeftMotor(int speed)
{
	//nxt_motor_set_count(NXT_PORT_A, 20);
	nxt_motor_set_speed(NXT_PORT_A, speed, 1);
}
void stopLeftMotor()
{
	nxt_motor_set_speed(NXT_PORT_A, 0, 0);
}

/*--------------------------------------------------------------------------*/
/* Function to be invoked from a category 2 interrupt                       */
/*--------------------------------------------------------------------------*/
void user_1ms_isr_type2()
{
	SignalCounter(SysTimerCnt);
}

/*--------------------------------------------------------------------------*/
/* Task information:                                                        */
/* -----------------                                                        */
/* Name    : MotorControlTask                                               */
/* Priority: 2                                                              */
/* Typ     : EXTENDED TASK                                                  */
/* Schedule: preemptive                                                     */
/*--------------------------------------------------------------------------*/
TASK(MotorControlTask) 
{
	GetResource(DC);
	
	if(dc.duration > 0)
	{
		
		if(dc.dir == 0)
		{
			driveRightMotor(dc.speed);
			driveLeftMotor(-dc.speed/2);
			//stopLeftMotor();
		}
		else if(dc.dir ==1)
		{
			driveLeftMotor(dc.speed);
			driveRightMotor(-dc.speed/2);
			//stopRightMotor();
		}
		else
		{
			driveLeftMotor(dc.speed);
			driveRightMotor(dc.speed);
		
		}
		
		dc.duration -= 15;
	}
	else
	{
		stopRightMotor();
		stopLeftMotor();
		if(dc.priority!=PRIO_UNCALIBRATED)
		dc.priority = PRIO_IDLE;
		//test+=1;
	}
	
	ReleaseResource(DC);

    TerminateTask(); /* or ChainTask() */
}


TASK(LightSensorTask)
{
	ecrobot_process_bg_nxtcolorsensor();
	int prevReflect = reflect;
	reflect = ecrobot_get_nxtcolorsensor_light(NXT_PORT_S1);
	
	//number of direction switches allowed while in white space
	int lost_chances = 4;//keep this even for best results
	
	GetResource(DC);

	//On the line for two cycles
	if(reflect < black && prevReflect <black)
	{
		change_driving_command(PRIO_LIGHT+1,50,100,3, dc.prevDir,LOST_RESET);//line is found, go straight
	}
	//Is lost
	else if(dc.lost>=lost_chances)//car is lost if it changes direction X times without finding line
	{
		change_driving_command(PRIO_LIGHT,50,1000*(dc.lost-lost_chances),(dc.dir+1)%2,(dc.dir+1)%2,dc.lost+1);
	}
	//Is white was black was going left
	else if((reflect >= black) && dc.prevDir ==LEFT) 
	{
		change_driving_command(PRIO_LIGHT,50,150,RIGHT,RIGHT,dc.lost+1);//set status to lost
	}
	//Is white was black was going right
	else if((reflect >=black) && dc.prevDir ==RIGHT)
	{
		change_driving_command(PRIO_LIGHT,50,150,LEFT, LEFT,dc.lost+1);//set status to lost
	}
	else
	{
		//change_driving_command(PRIO_LIGHT,50,100,4, dc.prevDir,0);//go straight, show error 4
		change_driving_command(PRIO_LIGHT-1,dc.speed,dc.duration+10,dc.dir, dc.prevDir,dc.lost);
	}
	
	ReleaseResource(DC);
		
	TerminateTask();
}

TASK(DisplayTask)
{
//display useful system data (speed, duration, etc...)
   display_clear(0);
   display_goto_xy(0, 0);
   display_string("Speed: ");
   display_int(dc.speed,0);
   display_goto_xy(0, 1);
   display_string("Duration: ");
   display_int(dc.duration,0);
   display_goto_xy(0, 2);
   display_string("CMD priority: ");
   display_int(dc.priority,0);
   display_goto_xy(0, 3);
   display_string("Light ");
   display_int(reflect,0);
   
   display_goto_xy(0, 4);
   display_string("PreDir: ");
   display_int(dc.prevDir,0);
   display_goto_xy(0, 5);
   display_string("dir: ");
   display_int(dc.dir,0);
   display_update(); 
	
	
	TerminateTask();
}

TASK(ButtonPressTask)
{
	if(ecrobot_get_touch_sensor(TOUCH_PORT_ID))
	{
		GetResource(DC);

		black = reflect+100;
		
		dc.priority = PRIO_IDLE;
		ReleaseResource(DC);
	
	}
	
	TerminateTask();
}