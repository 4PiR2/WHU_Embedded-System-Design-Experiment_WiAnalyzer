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

//UCOSIII���������ȼ��û�������ʹ�ã�ALIENTEK
//����Щ���ȼ��������UCOSIII��5��ϵͳ�ڲ�����
//���ȼ�0���жϷ������������� OS_IntQTask()
//���ȼ�1��ʱ�ӽ������� OS_TickTask()
//���ȼ�2����ʱ���� OS_TmrTask()
//���ȼ�OS_CFG_PRIO_MAX-2��ͳ������ OS_StatTask()
//���ȼ�OS_CFG_PRIO_MAX-1���������� OS_IdleTask()
//����֧�֣�www.openedv.com
//�Ա����̣�http://eboard.taobao.com  
//������������ӿƼ����޹�˾  
//���ߣ�����ԭ�� @ALIENTEK

//�������ȼ�
#define START_TASK_PRIO		3
//�����ջ��С	
#define START_STK_SIZE 		512
//������ƿ�
OS_TCB StartTaskTCB;
//�����ջ	
CPU_STK START_TASK_STK[START_STK_SIZE];
//������
void start_task(void *p_arg);

//�������ȼ�
#define LED0_TASK_PRIO		4
//�����ջ��С	
#define LED0_STK_SIZE 		128
//������ƿ�
OS_TCB Led0TaskTCB;
//�����ջ	
CPU_STK LED0_TASK_STK[LED0_STK_SIZE];
void led0_task(void *p_arg);

//�������ȼ�
#define LED1_TASK_PRIO		5
//�����ջ��С	
#define LED1_STK_SIZE 		256
//������ƿ�
OS_TCB Led1TaskTCB;
//�����ջ	
CPU_STK LED1_TASK_STK[LED1_STK_SIZE];
//������
void led1_task(void *p_arg);

//�������ȼ�
#define FLOAT_TASK_PRIO		6
//�����ջ��С
#define FLOAT_STK_SIZE		128
//������ƿ�
OS_TCB	FloatTaskTCB;
//�����ջ
__align(8) CPU_STK	FLOAT_TASK_STK[FLOAT_STK_SIZE];
//������
void float_task(void *p_arg);

OS_SEM	MY_SEM;		//����һ���ź��������ڷ��ʹ�����Դ

int main(void)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	
	delay_init(168);  	//ʱ�ӳ�ʼ��
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�жϷ�������
	uart_init(115200);  //���ڳ�ʼ��
	usart3_init(115200);  //��ʼ������3������Ϊ115200
	LED_Init();         //LED��ʼ��
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//����ϵͳ�ж����ȼ�����2
 	LCD_Init();           //��ʼ��LCD FSMC�ӿ�
	tp_dev.init();
	RNG_Init();
	Remote_Init();				//������ճ�ʼ��	
	BEEP_Init();      //��ʼ���������˿�
	KEY_Init();       //��ʼ���밴�����ӵ�Ӳ���ӿ�
	
	OSInit(&err);		//��ʼ��UCOSIII
	OS_CRITICAL_ENTER();//�����ٽ���
	//������ʼ����
	OSTaskCreate((OS_TCB 	* )&StartTaskTCB,		//������ƿ�
				 (CPU_CHAR	* )"start task", 		//��������
                 (OS_TASK_PTR )start_task, 			//������
                 (void		* )0,					//���ݸ��������Ĳ���
                 (OS_PRIO	  )START_TASK_PRIO,     //�������ȼ�
                 (CPU_STK   * )&START_TASK_STK[0],	//�����ջ����ַ
                 (CPU_STK_SIZE)START_STK_SIZE/10,	//�����ջ�����λ
                 (CPU_STK_SIZE)START_STK_SIZE,		//�����ջ��С
                 (OS_MSG_QTY  )0,					//�����ڲ���Ϣ�����ܹ����յ������Ϣ��Ŀ,Ϊ0ʱ��ֹ������Ϣ
                 (OS_TICK	  )0,					//��ʹ��ʱ��Ƭ��תʱ��ʱ��Ƭ���ȣ�Ϊ0ʱΪĬ�ϳ��ȣ�
                 (void   	* )0,					//�û�����Ĵ洢��
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, //����ѡ��
                 (OS_ERR 	* )&err);				//��Ÿú�������ʱ�ķ���ֵ
	OS_CRITICAL_EXIT();	//�˳��ٽ���	 
	OSStart(&err);  //����UCOSIII
	while(1);
}

//��ʼ������
void start_task(void *p_arg)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	p_arg = p_arg;

	CPU_Init();
#if OS_CFG_STAT_TASK_EN > 0u
   OSStatTaskCPUUsageInit(&err);  	//ͳ������                
#endif
	
#ifdef CPU_CFG_INT_DIS_MEAS_EN		//���ʹ���˲����жϹر�ʱ��
    CPU_IntDisMeasMaxCurReset();	
#endif

#if	OS_CFG_SCHED_ROUND_ROBIN_EN  //��ʹ��ʱ��Ƭ��ת��ʱ��
	 //ʹ��ʱ��Ƭ��ת���ȹ���,ʱ��Ƭ����Ϊ1��ϵͳʱ�ӽ��ģ���1*5=5ms
	OSSchedRoundRobinCfg(DEF_ENABLED,1,&err);  
#endif		
	
	OS_CRITICAL_ENTER();	//�����ٽ���
	//����һ���ź���
	OSSemCreate ((OS_SEM*	)&MY_SEM,
                 (CPU_CHAR*	)"MY_SEM",
                 (OS_SEM_CTR)1,		
                 (OS_ERR*	)&err);
	//����LED0����
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
				 
	//����LED1����
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
				 
	//���������������
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
	OS_TaskSuspend((OS_TCB*)&StartTaskTCB,&err);		//����ʼ����			 
	OS_CRITICAL_EXIT();	//�����ٽ���
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
		OSSemPend(&MY_SEM,0,OS_OPT_PEND_BLOCKING,0,&err); 	//�����ź���
		atk_8266_search_wifi(&q,2000);
		OSSemPost (&MY_SEM,OS_OPT_POST_1,&err);				//�����ź���
		LED0=0;
		OSTimeDlyHMSM(0,0,2,0,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ
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
	POINT_COLOR=RED;      //������ɫ����ɫ
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
		OSSemPend(&MY_SEM,0,OS_OPT_PEND_BLOCKING,0,&err); 	//�����ź���
		drawui(mode0,&cq1,&cq2,&q);
		OSSemPost (&MY_SEM,OS_OPT_POST_1,&err);				//�����ź���
		if(mode0==mode)
			OSTimeDlyHMSM(0,0,2,0,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ
	}
}

//�����������
void float_task(void *p_arg)
{
	//CPU_SR_ALLOC();
	//OS_CRITICAL_ENTER();	//�����ٽ���
	//printf("float_num��ֵΪ: %.4f\r\n",float_num);
	//OS_CRITICAL_EXIT();		//�˳��ٽ���
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
