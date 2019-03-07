#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "includes.h"
#include "os_app_hooks.h"
#include "led.h"
#include "lcd.h"
#include "touch.h"
#include "rng.h"
#include "ui.h"
#include "remote.h"
#include "beep.h"
#include "key.h"

//UCOSIII中以下优先级用户程序不能使用，ALIENTEK
//将这些优先级分配给了UCOSIII的5个系统内部任务
//优先级0：中断服务服务管理任务 OS_IntQTask()
//优先级1：时钟节拍任务 OS_TickTask()
//优先级2：定时任务 OS_TmrTask()
//优先级OS_CFG_PRIO_MAX-2：统计任务 OS_StatTask()
//优先级OS_CFG_PRIO_MAX-1：空闲任务 OS_IdleTask()
//技术支持：www.openedv.com
//淘宝店铺：http://eboard.taobao.com  
//广州市星翼电子科技有限公司  
//作者：正点原子 @ALIENTEK

//任务优先级
#define START_TASK_PRIO		3
//任务堆栈大小	
#define START_STK_SIZE 		512
//任务控制块
OS_TCB StartTaskTCB;
//任务堆栈	
CPU_STK START_TASK_STK[START_STK_SIZE];
//任务函数
void start_task(void *p_arg);

//任务优先级
#define LED0_TASK_PRIO		4
//任务堆栈大小	
#define LED0_STK_SIZE 		128
//任务控制块
OS_TCB Led0TaskTCB;
//任务堆栈	
CPU_STK LED0_TASK_STK[LED0_STK_SIZE];
void led0_task(void *p_arg);

//任务优先级
#define LED1_TASK_PRIO		5
//任务堆栈大小	
#define LED1_STK_SIZE 		256
//任务控制块
OS_TCB Led1TaskTCB;
//任务堆栈	
CPU_STK LED1_TASK_STK[LED1_STK_SIZE];
//任务函数
void led1_task(void *p_arg);

//任务优先级
#define FLOAT_TASK_PRIO		6
//任务堆栈大小
#define FLOAT_STK_SIZE		128
//任务控制块
OS_TCB	FloatTaskTCB;
//任务堆栈
__align(8) CPU_STK	FLOAT_TASK_STK[FLOAT_STK_SIZE];
//任务函数
void float_task(void *p_arg);

OS_SEM	MY_SEM;		//定义一个信号量，用于访问共享资源

int main(void)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	
	delay_init(168);  	//时钟初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//中断分组配置
	uart_init(115200);  //串口初始化
	usart3_init(115200);  //初始化串口3波特率为115200
	LED_Init();         //LED初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
 	LCD_Init();           //初始化LCD FSMC接口
	tp_dev.init();
	RNG_Init();
	Remote_Init();				//红外接收初始化	
	BEEP_Init();      //初始化蜂鸣器端口
	KEY_Init();       //初始化与按键连接的硬件接口
	
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
	while(1);
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
	OSSemCreate ((OS_SEM*	)&MY_SEM,
                 (CPU_CHAR*	)"MY_SEM",
                 (OS_SEM_CTR)1,		
                 (OS_ERR*	)&err);
	//创建LED0任务
	OSTaskCreate((OS_TCB 	* )&Led0TaskTCB,		
				 (CPU_CHAR	* )"led0 task", 		
                 (OS_TASK_PTR )led0_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )LED0_TASK_PRIO,     
                 (CPU_STK   * )&LED0_TASK_STK[0],	
                 (CPU_STK_SIZE)LED0_STK_SIZE/10,	
                 (CPU_STK_SIZE)LED0_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR 	* )&err);				
				 
	//创建LED1任务
	OSTaskCreate((OS_TCB 	* )&Led1TaskTCB,		
				 (CPU_CHAR	* )"led1 task", 		
                 (OS_TASK_PTR )led1_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )LED1_TASK_PRIO,     	
                 (CPU_STK   * )&LED1_TASK_STK[0],	
                 (CPU_STK_SIZE)LED1_STK_SIZE/10,	
                 (CPU_STK_SIZE)LED1_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, 
                 (OS_ERR 	* )&err);
				 
	//创建浮点测试任务
	OSTaskCreate((OS_TCB 	* )&FloatTaskTCB,		
				 (CPU_CHAR	* )"float test task", 		
                 (OS_TASK_PTR )float_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )FLOAT_TASK_PRIO,     	
                 (CPU_STK   * )&FLOAT_TASK_STK[0],	
                 (CPU_STK_SIZE)FLOAT_STK_SIZE/10,	
                 (CPU_STK_SIZE)FLOAT_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, 
                 (OS_ERR 	* )&err);				 
	OS_TaskSuspend((OS_TCB*)&StartTaskTCB,&err);		//挂起开始任务			 
	OS_CRITICAL_EXIT();	//进入临界区
}

static wifiqueue q;
void led0_task(void *p_arg)
{
	OS_ERR err;
	p_arg = p_arg;
	atk_8266_set(2000);
	while(1)
	{
		LED0=1;
		OSSemPend(&MY_SEM,0,OS_OPT_PEND_BLOCKING,0,&err); 	//请求信号量
		atk_8266_search_wifi(&q,2000);
		OSSemPost (&MY_SEM,OS_OPT_POST_1,&err);				//发送信号量
		LED0=0;
		OSTimeDlyHMSM(0,0,2,0,OS_OPT_TIME_HMSM_STRICT,&err); //延时
	}
}

static colorqueue cqa,cqb;
static u8 mode=3;
void led1_task(void *p_arg)
{
	OS_ERR err;
	p_arg = p_arg;
	u8 mode0;
	colorqueue *cq1=&cqa,*cq2=&cqb;
	cq1->len=cq2->len=0;
	//LCD_Display_Dir(1);
			//LCD_Clear(BLUE);
		//gui_fill_circle(200,200,100,GREEN);
		//LCD_DrawRectangle(100,100,470,790);
	POINT_COLOR=RED;      //画笔颜色：红色
	LCD_ShowString(24,7,480,24,24,"Wi-Fi Analyzer by CJL(2016301500014)");
	while(1)
	{
		mode0=mode;
		POINT_COLOR=BLACK;
		LCD_Fill(12,40,12+145,80,mode0==3?YELLOW:LIGHTGREEN);
		LCD_ShowString(12+20,40+12,13*16,16,16,"Access Points");
		LCD_Fill(12+145+10,40,12+145+10+145,80,mode0==1?YELLOW:LIGHTGREEN);
		LCD_ShowString(12+145+10+20,40+12,13*16,16,16,"Channel Graph");
		LCD_Fill(12+145+10+145+10,40,12+145+10+145+10+145,80,mode0==2?YELLOW:LIGHTGREEN);
		LCD_ShowString(12+145+10+145+10+32,40+12,10*16,16,16,"Time Graph");
		OSSemPend(&MY_SEM,0,OS_OPT_PEND_BLOCKING,0,&err); 	//请求信号量
		drawui(mode0,&cq1,&cq2,&q);
		OSSemPost (&MY_SEM,OS_OPT_POST_1,&err);				//发送信号量
		if(mode0==mode)
			OSTimeDlyHMSM(0,0,2,0,OS_OPT_TIME_HMSM_STRICT,&err); //延时
	}
}

//浮点测试任务
void float_task(void *p_arg)
{
	//CPU_SR_ALLOC();
	//OS_CRITICAL_ENTER();	//进入临界区
	//printf("float_num的值为: %.4f\r\n",float_num);
	//OS_CRITICAL_EXIT();		//退出临界区
	OS_ERR err;
	p_arg = p_arg;
	u16 x,y;	  
	u8 mode0;	
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
				default:
				tp_dev.scan(0);
				//for(t=0;t<OTT_MAX_TOUCH;t++)
					//if((tp_dev.sta)&(1<<t));
				if(tp_dev.sta&1)
				{
					x=tp_dev.x[0];
					y=tp_dev.y[0];
					if(y>=40&&y<=80)
					{
						if(x>=12&&x<=12+145)
							mode=3;
						else if(x>=12+145+10&&x<=12+145+10+145)
							mode=1;
						else if(x>=12+145+10+145+10&&x<=12+145+10+145+10+145)
							mode=2;
					}
				}
			}
		}
		if(mode0!=mode)
			BEEP=1;
		OSTimeDlyHMSM(0,0,0,10,OS_OPT_TIME_HMSM_STRICT,&err);
		BEEP=0;
	}	
}
