#include "includes.h"
#include "ui.h"
#include "lcd.h"
#include "rng.h"

static const u16 X1=38,Y1=110,DX1=27,DY1=65,LFSIZE=16;
static u8 *labels1[]={"0","-10","-20","-30","-40","-50","-60","-70","-80","-90","-100","1","2","3","4","5","6","7","8","9","10","11","12","13","RSSI/dBm","Channels"};

static char adddata(colorqueue *cq,colorinfo *d,int index)
{
	if(cq->len>=CQLEN)
		return -1;
	cq->data[cq->len]=*d;
	if(index<QLEN)
		cq->rssiindex[index]=&cq->data[cq->len];
	cq->len++;
	return 0;
}

//随机生成颜色 
int getrandomcolor(colorqueue *cq)
{
	int i,r,g,b,j,flag=1;
	while(flag)
	{
		do
		{
			i=RNG_Get_RandomNum();
			r=i>>11&0x1F;
			g=(i>>5&0x3F)>>1;
			b=i&0x1F;
		}while(r+g+b>0x1F*2);
		flag=0;
		for(j=0;j<cq->len;j++)
			if(cq->data[j].color==i)
			{
				flag=1;
				break;
			}
	}
	return i;
}

//画信道图背景网格 
void drawchannelgrid()
{
	u8 i;
	LCD_Fill(0,90,479,799,WHITE);
	POINT_COLOR=LGRAY;
	for(i=0;i<=10;i++)
		LCD_DrawLine(X1,Y1+DY1*i,X1+DX1*16,Y1+DY1*i);
	for(i=0;i<=16;i++)
		LCD_DrawLine(X1+DX1*i,Y1,X1+DX1*i,Y1+DY1*10);
	POINT_COLOR=BLACK;
	for(i=0;i<=10;i++)
		LCD_ShowString(X1-strlen((char *)labels1[i])*LFSIZE/2,Y1+DY1*i-LFSIZE/2,strlen((char *)labels1[i])*LFSIZE/2,LFSIZE,LFSIZE,labels1[i]);
	for(i=2;i<=14;i++)
		LCD_ShowString(X1+DX1*i-strlen((char *)labels1[9+i])*LFSIZE/4,Y1+DY1*10,strlen((char *)labels1[9+i])*LFSIZE/2,LFSIZE,LFSIZE,labels1[9+i]);
	LCD_ShowString(5,Y1+DY1*10+LFSIZE,strlen((char *)labels1[24])*LFSIZE/2,LFSIZE,LFSIZE,labels1[24]);
	LCD_ShowString(X1+DX1*8-strlen((char *)labels1[25])*LFSIZE/4,Y1+DY1*10+LFSIZE,strlen((char *)labels1[25])*LFSIZE/2,LFSIZE,LFSIZE,labels1[25]);
}

//画信道图的梯形 
void drawtrapeziod(colorinfo *d)
{
	char *ssid=d->wifi.ssid;
	u8 channel=d->wifi.channel,rssi=d->wifi.rssi,ssidlen=strlen(ssid);
	u16 r=(256-rssi)%256,xb=X1+DX1*channel,xa=xb-DX1,xc=xb+DX1*2,xd=xc+DX1,
		ya=Y1+DY1*10,yb=Y1+DY1*r/10,yc=yb,yd=ya;
	if(!(rssi^0x80))
		return;
	POINT_COLOR=d->color;
	LCD_DrawLine(xa,ya,xb,yb);
	LCD_DrawLine(xb,yb,xc,yc);
	LCD_DrawLine(xc,yc,xd,yd);
	LCD_ShowString(xb+DX1-ssidlen*LFSIZE/4,yb-LFSIZE,ssidlen*LFSIZE/2,LFSIZE,LFSIZE,(u8 *)ssid);
}

static const u16 X2=38,Y2=100,DX2=(479-X2)/(TLEN-1),DY2=40;
static u8 *labels2[]={"0","-10","-20","-30","-40","-50","-60","-70","-80","-90","-100","RSSI/dBm","Scan Count"};
//画时间图背景网格 
void drawtimegrid()
{
	u8 i;
	LCD_Fill(0,90,479,799,WHITE);
	POINT_COLOR=LGRAY;
	for(i=0;i<=10;i++)
		LCD_DrawLine(X2,Y2+DY2*i,X2+DX2*(TLEN-1),Y2+DY2*i);
	for(i=0;i<=TLEN-1;i++)
		LCD_DrawLine(X2+DX2*i,Y2,X2+DX2*i,Y2+DY2*10);
	POINT_COLOR=BLACK;
	for(i=0;i<=10;i++)
		LCD_ShowString(X2-strlen((char *)labels2[i])*LFSIZE/2,Y2+DY2*i-LFSIZE/2,strlen((char *)labels2[i])*LFSIZE/2,LFSIZE,LFSIZE,labels2[i]);
	LCD_ShowString(5,Y2+DY2*10+LFSIZE/2,strlen((char *)labels2[11])*LFSIZE/2,LFSIZE,LFSIZE,labels2[11]);
	LCD_ShowString(X2+DX2*(TLEN-1)/2-strlen((char *)labels2[12])*LFSIZE/4,Y2+DY2*10+LFSIZE/2,strlen((char *)labels2[12])*LFSIZE/2,LFSIZE,LFSIZE,labels2[12]);
}
//画时间图折线 
void drawpolyline(colorinfo *d,int time,int order)
{
	char ssid[38];
	int i;
	u8 ra=(256-d->strength[time])%256,rb,channel=d->wifi.channel,ssidlen=strlen(ssid);
	u16 xa,xb,ya,yb;
	sprintf(ssid,"%s (%d)",d->wifi.ssid,d->wifi.channel);
	POINT_COLOR=d->color;
	xa=X2+DX2*(TLEN-1);
	ya=Y2+DY2*ra/10;
	LCD_Fill(LFSIZE/2+order%2*240,Y2+DY2*10+LFSIZE*3/2+order/2*(LFSIZE+2)+2,LFSIZE+LFSIZE/2+order%2*240,LFSIZE+Y2+DY2*10+LFSIZE*3/2+order/2*(LFSIZE+2)+2,d->color);
	LCD_ShowString(LFSIZE+LFSIZE/2+order%2*240+2,Y2+DY2*10+LFSIZE*3/2+order/2*(LFSIZE+2)+2,(strlen(ssid)+3)*LFSIZE,LFSIZE,LFSIZE,(u8 *)ssid);
	for(i=1;i<TLEN;i++)
	{
		rb=(256-d->strength[(TLEN+time-i)%TLEN])%256;
		xb=X2+DX2*(TLEN-1-i);
		yb=Y2+DY2*rb/10;
		if(ra!=128&&rb!=128)
			LCD_DrawLine(xa,ya,xb,yb);
		xa=xb;
		ya=yb;
		ra=rb;
	}
}
	
static const u16 X3=5,Y3=90,DY3=LFSIZE*2;
//显示详细信息 
void drawform(colorinfo *d,int order)
{
	char c,tmp[24],
    *ecn[]={"OPEN","WEP","WPA_PSK","WPA2_PSK","WPA_WPA2_PSK","WPA2_Enterprise"},
    *bgn[]={"","b","g","b/g","n","b/n","g/n","b/g/n"};
	long long mac=d->wifi.mac;
	int i;
	POINT_COLOR=BLACK;
	LCD_DrawLine(0,Y3+DY3*order,479,Y3+DY3*order);
	LCD_DrawLine(0,Y3+DY3*(order+1),479,Y3+DY3*(order+1));
	LCD_ShowString(X3,Y3+DY3*order,32*LFSIZE/2,LFSIZE,LFSIZE,(u8 *)d->wifi.ssid);
	tmp[17]=0;
	for(i=16;i>=0;i--)
	{
		if(i%3==2)
		{
			tmp[i]=':';
			continue;
		}
		c=mac&0xF;
		mac>>=4;
		tmp[i]=c>9?c-10+'a':c+'0';
	}
	LCD_ShowString(479-X3-17*LFSIZE/2,Y3+DY3*order,17*LFSIZE/2,LFSIZE,LFSIZE,(u8 *)tmp);
	sprintf(tmp,"%ddBm",-(256-d->wifi.rssi)%256);
	LCD_ShowString(X3,Y3+DY3*order+LFSIZE,7*LFSIZE/2,LFSIZE,LFSIZE,(u8 *)tmp);
	sprintf(tmp,"CH%2d %d(%d-%d)MHz",d->wifi.channel,2407+d->wifi.channel*5,2407+d->wifi.channel*5-10,2407+d->wifi.channel*5+10);
	tmp[2]=tmp[2]==' '?'0':'1';
	LCD_ShowString(X3+10+7*LFSIZE/2,Y3+DY3*order+LFSIZE,23*LFSIZE/2,LFSIZE,LFSIZE,(u8 *)tmp);
	if(d->wifi.bgn)
	{
		sprintf(tmp,"%s",bgn[d->wifi.bgn]);
		LCD_ShowString(X3+10*2+(7+23)*LFSIZE/2,Y3+DY3*order+LFSIZE,5*LFSIZE/2,LFSIZE,LFSIZE,(u8 *)tmp);
	}
	if(d->wifi.ecn)
		LCD_ShowString(X3+10*3+(7+23+5)*LFSIZE/2,Y3+DY3*order+LFSIZE,15*LFSIZE/2,LFSIZE,LFSIZE,(u8 *)ecn[d->wifi.ecn]);
	if(d->wifi.wps)
		LCD_ShowString(479-X3-3*LFSIZE/2,Y3+DY3*order+LFSIZE,3*LFSIZE/2,LFSIZE,LFSIZE,"WPS");
}


//wifi信号扫描模块调用：更新数据集 
void prepareui(colorqueue **cq0,colorqueue **cq1,wifiqueue *q)
{
		colorqueue *cq2;
		colorinfo d;
		u8 i=0,j=0,k,index,time=((*cq0)->time+1)%TLEN;
		while(i<q->len&&j<(*cq0)->len)
		{
			if(q->pointer[i]->mac<(*cq0)->data[j].wifi.mac)
			{
				d.wifi=*q->pointer[i];
				d.color=getrandomcolor(*cq0);
				memset(d.strength,0x80,TLEN*sizeof(char));
				d.strength[time]=d.wifi.rssi;
				adddata(*cq1,&d,q->pointer[i]-q->data);
				i++;
			}
			else if(q->pointer[i]->mac==(*cq0)->data[j].wifi.mac)
			{
				(*cq0)->data[j].wifi=*q->pointer[i];
				(*cq0)->data[j].strength[time]=(*cq0)->data[j].wifi.rssi;
				adddata(*cq1,&(*cq0)->data[j],q->pointer[i]-q->data);
				i++;
				j++;
			}
			else
			{
				(*cq0)->data[j].wifi.rssi=(*cq0)->data[j].strength[time]=-128;
				for(k=0;k<TLEN;k++)
					if((*cq0)->data[j].strength[k]!=128)
						break;
				if(k!=TLEN)
					adddata(*cq1,&(*cq0)->data[j],QLEN);
				j++;
			}
		}
		while(i<q->len)
		{
				d.wifi=*q->pointer[i];
				d.color=getrandomcolor(*cq0);
				memset(d.strength,0x80,TLEN*sizeof(char));
				d.strength[time]=d.wifi.rssi;
				adddata(*cq1,&d,q->pointer[i]-q->data);
				i++;
		}
		while(j<(*cq0)->len)
		{
				(*cq0)->data[j].wifi.rssi=(*cq0)->data[j].strength[time]=-128;
				for(k=0;k<TLEN;k++)
					if((*cq0)->data[j].strength[k]!=128)
						break;
				if(k!=TLEN)
					adddata(*cq1,&(*cq0)->data[j],QLEN);
				j++;
		}
		(*cq1)->indexlen=q->len;
		(*cq1)->time=time;
		cq2=*cq1;
		*cq1=*cq0;
		*cq0=cq2;
		(*cq1)->len=0;
}

//UI绘制模块调用 
void drawui(u8 mode,colorqueue **cq0,colorqueue **cq1)
{
		u8 i;
		switch(mode)
		{
			case 1:
				drawchannelgrid();
				for(i=0;i<(*cq0)->len;i++)
				{
					if(i>=CMAX)
						break;
					drawtrapeziod(&(*cq0)->data[i]);
				}
				break;
			case 2:
				drawtimegrid();
				for(i=0;i<(*cq0)->len;i++)
				{
					if(i>=CMAX)
						break;
					drawpolyline(&(*cq0)->data[i],(*cq0)->time,i);
				}
				break;
			case 3:
				LCD_Fill(0,90,479,799,WHITE);
				for(i=0;i<(*cq0)->indexlen;i++)
				{
					if(i>=DMAX)
						break;
					drawform((*cq0)->rssiindex[i],i);
				}
				break;
		}
}

