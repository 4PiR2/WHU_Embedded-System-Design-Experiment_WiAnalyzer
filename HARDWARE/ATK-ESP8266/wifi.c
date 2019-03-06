#include "stdio.h"
#include "delay.h"
#include "wifi.h"
#include "sys.h"
#include "delay.h"
#include "usart.h"

static char adddata(wifiqueue *q,wifiinfo *d)
{
	if(q->len>=QLEN)
		return -1;
	q->data[q->len]=*d;
	q->pointer[q->len]=q->data+q->len;
	q->len++;
	return 0;
}
static short max(wifiqueue *q,short n,short i,short j,short k)
{
	short m=i;
	if(j<n&&q->pointer[j]->mac>q->pointer[m]->mac)
		m=j;
	if(k<n&&q->pointer[k]->mac>q->pointer[m]->mac)
		m=k;
	return m;
}
static void downheap(wifiqueue *q,short n,short i)
{
	wifiinfo *t;
	while(1)
	{
		short j=max(q,n,i,2*i+1,2*i+2);
		if(j==i)
			break;
		t=q->pointer[i];
		q->pointer[i]=q->pointer[j];
		q->pointer[j]=t;
		i=j;
	}
}
static void heapsort(wifiqueue *q,short n)
{
	wifiinfo *t;
	short i;
	if(n<2)
		return;
	for(i=(n-2)/2;i>=0;i--)
		downheap(q,n,i);
	for(i=0;i<n;i++)
	{
		t=q->pointer[n-i-1];
		q->pointer[n-i-1]=q->pointer[0];
		q->pointer[0]=t;
		downheap(q,n-i-1,0);
	}
}
static char check(char *str)
{
	switch(*str)
	{
		case '+':
			if(str[1]=='C'&&str[2]=='W'&&str[3]=='L'&&str[4]=='A'&&str[5]=='P'&&str[6]==':')
				return 1;
			else
				return 0;
		case 'O':
			if(str[1]=='K'&&str[2]=='\r'&&str[3]=='\n')
				return 2;
			else
				return 0;
		case 0:
				return 3;
		default:
			return 0;	
	}
}
static void movetonextline(char **str)
{
	do
	{
		while(**str!='\r')
		{
			if(!**str)
				return;
			(*str)++;
		}
		(*str)++;
	} while(**str!='\n');
	(*str)++;
}
static short readint(char **str)
{
	short result=0;
	char flag=0;
	while(**str=='-')
	{
		flag=!flag;
		(*str)++;
	}
	while(**str>='0'&&**str<='9')
	{
		result=result*10+**str-'0';
		(*str)++;
	}
	return flag?-result:result;
}
static char readstring(char **str,char *dst)
{
	char count=0;
	if(**str=='"')
		(*str)++;
	else
		return -1;
	while(**str!='"')
	{
		*dst++=*(*str)++;
		if(++count>32)
			return -2;
	}
	(*str)++;
	return count;
}
static char readmac(char **str,long long *mac)
{
	char tmp[17],i;
	*mac=0;
	if(readstring(str,tmp)!=17)
		return -1;
	for(i=0;i<17;i++)
	{
		switch(tmp[i])
		{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				*mac=*mac<<4|tmp[i]-'0';
				break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
				*mac=*mac<<4|tmp[i]-'A'+10;
				break;
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
				*mac=*mac<<4|tmp[i]-'a'+10;
			case ':':
				break;
			default:
				return -1;
		}
	}
	return 0;
}
static char readinfo(wifiqueue *q,char *str)
{
	wifiinfo d;
	u8 ssidlen;
	while(1)
	{
		//printf("p%d\n%s\n",check(str),str);
		switch(check(str))
		{
			case 1:
				str+=7;
				if(*str++!='(') return -1;
				d.ecn=readint(&str);
				if(*str++!=',') return -1;
				ssidlen=readstring(&str,d.ssid);
				if(ssidlen>32||*str++!=',') return -1;
				d.ssid[ssidlen]=0;
				d.rssi=readint(&str);
				if(*str++!=','||readmac(&str,&d.mac)||*str++!=',') return -1;
				d.channel=readint(&str);
				if(*str++!=',') return -1;
				d.freqoffset=readint(&str);
				if(*str++!=',') return -1;
				d.freqcali=readint(&str);
				if(*str++!=',') return -1;
				d.pairwisecipher=readint(&str);
				if(*str++!=',') return -1;
				d.groupcipher=readint(&str);
				if(*str++!=',') return -1;
				d.bgn=readint(&str);
				if(*str++!=',') return -1;
				d.wps=readint(&str);
				if(*str++!=')') return -1;
				adddata(q,&d);
			case 0:
				movetonextline(&str);
				break;
			case 2:
				return 0;
			default:
				return -1;
		}
	}
}

//向ATK-ESP8266发送命令
//waittime:等待时间(单位:10ms)
//返回值:0,发送成功(得到了期待的应答结果)
//       1,发送失败
u8 atk_8266_search_wifi(wifiqueue *q,short waittime)
{
	USART3_RX_STA=0;
	u3_printf("%s\r\n","AT+CWLAP");	//发送命令
	while(--waittime)	//等待倒计时
	{
		delay_ms(10);
		if(USART3_RX_STA&0X8000)		//接收到一次数据了
		{ 
			USART3_RX_BUF[USART3_RX_STA&0X7FFF]=0;//添加结束符
			q->len=0;
			if(readinfo(q,USART3_RX_BUF))
				USART3_RX_STA=0;
			else
			{
				heapsort(q,q->len);
				break;
			}
		}
	}
	if(waittime==0) return 1;
	return 0;
} 
