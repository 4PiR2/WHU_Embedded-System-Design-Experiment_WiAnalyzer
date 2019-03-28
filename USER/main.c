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

//UCOSIII���������ȼ��û�������ʹ�ã�
//����Щ���ȼ��������UCOSIII��5��ϵͳ�ڲ�����
//���ȼ�0���жϷ������������� OS_IntQTask()
//���ȼ�1��ʱ�ӽ������� OS_TickTask()
//���ȼ�2����ʱ���� OS_TmrTask()
//���ȼ�OS_CFG_PRIO_MAX-2��ͳ������ OS_StatTask()
//���ȼ�OS_CFG_PRIO_MAX-1���������� OS_IdleTask()

//�������ȼ�
#define START_TASK_PRIO		3
//�����ջ��С	
#define START_STK_SIZE 		128
//������ƿ�
OS_TCB StartTaskTCB;
//�����ջ	
CPU_STK START_TASK_STK[START_STK_SIZE];
//������
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

OS_SEM	Q_SEM;		//����һ���ź��������ڷ��ʹ�����Դ

int main(void)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//����ϵͳ�ж����ȼ�����2
	delay_init(168);  	//ʱ�ӳ�ʼ��
	uart_init(115200);  //���ڳ�ʼ��
	usart3_init(115200);  //��ʼ������3������Ϊ115200
	LED_Init();         //LED��ʼ��
 	LCD_Init();           //��ʼ��LCD FSMC�ӿ�
	tp_dev.init();
	Remote_Init();				//������ճ�ʼ��	
	BEEP_Init();      //��ʼ���������˿�
	RNG_Init();
	KEY_Init();       //��ʼ���밴�����ӵ�Ӳ���ӿ�
	WKUP_Init();
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
	OSSemCreate ((OS_SEM*	)&Q_SEM,
                 (CPU_CHAR*	)"Q_SEM",
                 (OS_SEM_CTR)1,		
                 (OS_ERR*	)&err);
	//����Wi-Fi�ź�ɨ������
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
				 
	//������Ļˢ������
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
				 
	//�����û����봦������
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
	OS_TaskSuspend((OS_TCB*)&StartTaskTCB,&err);		//����ʼ����			 
	OS_CRITICAL_EXIT();	//�˳��ٽ���
}

static wifiqueue q;
static colorqueue cqa,cqb,*cq0=&cqa,*cq1=&cqb;
static u8 mode=3;

//Wi-Fi�ź�ɨ������
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
		OSSemPend(&Q_SEM,0,OS_OPT_PEND_BLOCKING,0,&err); 	//�����ź���
		prepareui(&cq0,&cq1,&q);
		OSSemPost (&Q_SEM,OS_OPT_POST_1,&err);				//�����ź���
		OSTimeDlyHMSM(0,0,3,0,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ
	}
}

//��Ļˢ������
void ui_task(void *p_arg)
{
	OS_ERR err;
	u8 mode0;
	p_arg = p_arg;
	POINT_COLOR=RED;      //������ɫ����ɫ
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
		OSSemPend(&Q_SEM,0,OS_OPT_PEND_BLOCKING,0,&err); 	//�����ź���
		drawui(mode0,&cq0,&cq1);
		OSSemPost (&Q_SEM,OS_OPT_POST_1,&err);				//�����ź���
		if(mode0==mode)
			OSTimeDlyHMSM(0,0,3,0,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ
	}
}

//�û����봦������
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
