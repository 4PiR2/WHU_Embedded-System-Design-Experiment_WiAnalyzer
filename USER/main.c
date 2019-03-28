#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "includes.h"
#include "os_app_hooks.h"
#include "led.h"
#include "lcd.h"
#include "touch.h"
#include "remote.h"
#include "beep.h"
#include "rng.h"
#include "key.h"
#include "wkup.h"
#include "ui.h"

//UCOSIII中以下优先级用户程序不能使用，
//将这些优先级分配给了UCOSIII的5个系统内部任务
//优先级0：中断服务服务管理任务 OS_IntQTask()
//优先级1：时钟节拍任务 OS_TickTask()
//优先级2：定时任务 OS_TmrTask()
//优先级OS_CFG_PRIO_MAX-2：统计任务 OS_StatTask()
//优先级OS_CFG_PRIO_MAX-1：空闲任务 OS_IdleTask()

//任务优先级
#define START_TASK_PRIO		3
//任务堆栈大小	
#define START_STK_SIZE 		128
//任务控制块
OS_TCB StartTaskTCB;
//任务堆栈	
CPU_STK START_TASK_STK[START_STK_SIZE];
//任务函数
void start_task(void *p_arg);

#define WIFI_TASK_PRIO		6
#define WIFI_STK_SIZE 		256
OS_TCB WIFITaskTCB;
CPU_STK WIFI_TASK_STK[WIFI_STK_SIZE];
void wifi_task(void *p_arg);

#define UI_TASK_PRIO		5
#define UI_STK_SIZE 		256
OS_TCB UITaskTCB;	
CPU_STK UI_TASK_STK[UI_STK_SIZE];
void ui_task(void *p_arg);

#define MODE_TASK_PRIO		4
#define MODE_STK_SIZE		128
OS_TCB	MODETaskTCB;
__align(8) CPU_STK	MODE_TASK_STK[MODE_STK_SIZE];
void mode_task(void *p_arg);

OS_SEM	Q_SEM;		//定义一个信号量，用于访问共享资源

int main(void)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	delay_init(168);  	//时钟初始化
	uart_init(115200);  //串口初始化
	usart3_init(115200);  //初始化串口3波特率为115200
	LED_Init();         //LED初始化
 	LCD_Init();           //初始化LCD FSMC接口
	tp_dev.init();
	Remote_Init();				//红外接收初始化	
	BEEP_Init();      //初始化蜂鸣器端口
	RNG_Init();
	KEY_Init();       //初始化与按键连接的硬件接口
	WKUP_Init();
	OSInit(&err);		//初始化UCOSIII
	OS_CRITICAL_ENTER();//进入临界区
	//创建开始任务
	OSTaskCreate((OS_TCB 	* )&StartTaskTCB,		//任务控制块
				 (CPU_CHAR	* )"start task", 		//任务名字
                 (OS_TASK_PTR )start_task, 			//任务函数
                 (void		* )0,					//传递给任务函数的参数
                 (OS_PRIO	  )START_TASK_PRIO,     //任务优先级
                 (CPU_STK   * )&START_TASK_STK[0],	//任务堆栈基地址
                 (CPU_STK_SIZE)START_STK_SIZE/10,	//任务堆栈深度限位
                 (CPU_STK_SIZE)START_STK_SIZE,		//任务堆栈大小
                 (OS_MSG_QTY  )0,					//任务内部消息队列能够接收的最大消息数目,为0时禁止接收消息
                 (OS_TICK	  )0,					//当使能时间片轮转时的时间片长度，为0时为默认长度，
                 (void   	* )0,					//用户补充的存储区
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, //任务选项
                 (OS_ERR 	* )&err);				//存放该函数错误时的返回值
	OS_CRITICAL_EXIT();	//退出临界区	 
	OSStart(&err);  //开启UCOSIII
}

//开始任务函数
void start_task(void *p_arg)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	p_arg = p_arg;
	CPU_Init();
#if OS_CFG_STAT_TASK_EN > 0u
   OSStatTaskCPUUsageInit(&err);  	//统计任务                
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN		//如果使能了测量中断关闭时间
    CPU_IntDisMeasMaxCurReset();	
#endif

#if	OS_CFG_SCHED_ROUND_ROBIN_EN  //当使用时间片轮转的时候
	 //使能时间片轮转调度功能,时间片长度为1个系统时钟节拍，既1*5=5ms
	OSSchedRoundRobinCfg(DEF_ENABLED,1,&err);  
#endif		

	OS_CRITICAL_ENTER();	//进入临界区
	//创建一个信号量
	OSSemCreate ((OS_SEM*	)&Q_SEM,
                 (CPU_CHAR*	)"Q_SEM",
                 (OS_SEM_CTR)1,		
                 (OS_ERR*	)&err);
	//创建Wi-Fi信号扫描任务
	OSTaskCreate((OS_TCB 	* )&WIFITaskTCB,		
				 (CPU_CHAR	* )"wifi task", 		
                 (OS_TASK_PTR )wifi_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )WIFI_TASK_PRIO,     
                 (CPU_STK   * )&WIFI_TASK_STK[0],	
                 (CPU_STK_SIZE)WIFI_STK_SIZE/10,	
                 (CPU_STK_SIZE)WIFI_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR 	* )&err);				
				 
	//创建屏幕刷新任务
	OSTaskCreate((OS_TCB 	* )&UITaskTCB,		
				 (CPU_CHAR	* )"ui task", 		
                 (OS_TASK_PTR )ui_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )UI_TASK_PRIO,     	
                 (CPU_STK   * )&UI_TASK_STK[0],	
                 (CPU_STK_SIZE)UI_STK_SIZE/10,	
                 (CPU_STK_SIZE)UI_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, 
                 (OS_ERR 	* )&err);
				 
	//创建用户输入处理任务
	OSTaskCreate((OS_TCB 	* )&MODETaskTCB,		
				 (CPU_CHAR	* )"mode task", 		
                 (OS_TASK_PTR )mode_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )MODE_TASK_PRIO,     	
                 (CPU_STK   * )&MODE_TASK_STK[0],	
                 (CPU_STK_SIZE)MODE_STK_SIZE/10,	
                 (CPU_STK_SIZE)MODE_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, 
                 (OS_ERR 	* )&err);				 
	OS_TaskSuspend((OS_TCB*)&StartTaskTCB,&err);		//挂起开始任务			 
	OS_CRITICAL_EXIT();	//退出临界区
}

static wifiqueue q;
static colorqueue cqa,cqb,*cq0=&cqa,*cq1=&cqb;
static u8 mode=3;

//Wi-Fi信号扫描任务
void wifi_task(void *p_arg)
{
	OS_ERR err;
	p_arg = p_arg;
	atk_8266_set(2000);
	cq0->len=cq1->len=0;
	while(1)
	{
		printf("Scanning\n");
		LED1=0;
		atk_8266_search_wifi(&q,2000);
		LED1=1;
		OSSemPend(&Q_SEM,0,OS_OPT_PEND_BLOCKING,0,&err); 	//请求信号量
		prepareui(&cq0,&cq1,&q);
		OSSemPost (&Q_SEM,OS_OPT_POST_1,&err);				//发送信号量
		OSTimeDlyHMSM(0,0,3,0,OS_OPT_TIME_HMSM_STRICT,&err); //延时
	}
}

//屏幕刷新任务
void ui_task(void *p_arg)
{
	OS_ERR err;
	u8 mode0;
	p_arg = p_arg;
	POINT_COLOR=RED;      //画笔颜色：红色
	LCD_ShowString(24,7,480,24,24,"Wi-Fi Analyzer by CJL(2016301500014)");
	while(1)
	{
		mode0=mode;
		POINT_COLOR=BLACK;
		LCD_Fill(12,40,12+145,80,mode0==3?YELLOW:0X6FF);
		LCD_ShowString(12+20,40+12,13*16,16,16,"ACCESS POINTS");
		LCD_Fill(12+145+10,40,12+145+10+145,80,mode0==1?YELLOW:0X6FF);
		LCD_ShowString(12+145+10+20,40+12,13*16,16,16,"CHANNEL GRAPH");
		LCD_Fill(12+145+10+145+10,40,12+145+10+145+10+145,80,mode0==2?YELLOW:0X6FF);
		LCD_ShowString(12+145+10+145+10+32,40+12,10*16,16,16,"TIME GRAPH");
		OSSemPend(&Q_SEM,0,OS_OPT_PEND_BLOCKING,0,&err); 	//请求信号量
		drawui(mode0,&cq0,&cq1);
		OSSemPost (&Q_SEM,OS_OPT_POST_1,&err);				//发送信号量
		if(mode0==mode)
			OSTimeDlyHMSM(0,0,3,0,OS_OPT_TIME_HMSM_STRICT,&err); //延时
	}
}

//用户输入处理任务
void mode_task(void *p_arg)
{
	OS_ERR err;
	p_arg = p_arg;
	u16 x1,y1,x2,x10,x20;	  
	u8 mode0,flag=0,minl=50;
	while(1)
	{
		mode0=mode;
		switch(KEY_Scan(0))
		{
			case KEY0_PRES:
				mode=2;
				break;
			case KEY1_PRES:
				mode=1;
				break;
			case KEY2_PRES:
				mode=3;
				break;
			default:
			switch(Remote_Scan())
			{
				case 104:
					mode=3;
					break;
				case 152:
					mode=1;
					break;
				case 176:
					mode=2;
					break;
				case 34:
					if(mode==1)
						mode=3;
					else if(mode==2)
						mode=1;
					break;
				case 194:
					if(mode==1)
						mode=2;
					else if(mode==3)
						mode=1;
					break;
				default:
				tp_dev.scan(0);
				switch(tp_dev.sta&3)
				{
					case 3:
						x1=tp_dev.x[0];
						x2=tp_dev.x[1];
						if(flag)
						{
							if(x1<x10-minl&&x2<x20-minl)
							{
								if(mode==1)
									mode=2;
								else if(mode==3)
									mode=1;
								x10=x1;
								x20=x2;
							}
							else if(x1>x10+minl&&x2>x20+minl)
							{
								if(mode==1)
									mode=3;
								else if(mode==2)
									mode=1;
								x10=x1;
								x20=x2;
							}
						}
						else
						{
							x10=x1;
							x20=x2;
							flag=1;
						}
						break;
					case 1:
						x1=tp_dev.x[0];
						y1=tp_dev.y[0];
						if(y1>=40&&y1<=80)
						{
							if(x1>=12&&x1<=12+145)
								mode=3;
							else if(x1>=12+145+10&&x1<=12+145+10+145)
								mode=1;
							else if(x1>=12+145+10+145+10&&x1<=12+145+10+145+10+145)
								mode=2;
						}
					default:
						flag=0;
				}
			}
		}
		if(mode0!=mode)
		{
			BEEP=1;
			OSTimeDlyResume((OS_TCB *)&UITaskTCB,&err);
		}
		OSTimeDlyHMSM(0,0,0,5,OS_OPT_TIME_HMSM_STRICT,&err);
		BEEP=0;
	}	
}
