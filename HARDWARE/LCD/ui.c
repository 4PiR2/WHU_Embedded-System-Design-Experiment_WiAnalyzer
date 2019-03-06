#include "includes.h"
#include "ui.h"
#include "lcd.h"
#include "rng.h"

static const u16 X0=38,Y0=110,DX=27,DY=65,LFSIZE=16;
static u8 *labels[]={"0","-10","-20","-30","-40","-50","-60","-70","-80","-90","-100","1","2","3","4","5","6","7","8","9","10","11","12","13","RSSI/dBm","Channels"};

static char adddata(colorqueue *cq,colorinfo *d)
{
	if(cq->len>=CQLEN)
		return -1;
	cq->data[cq->len]=*d;
	cq->len++;
	return 0;
}
int getrandomcolor()
{
	int i,r,g,b;
	do
	{
		i=RNG_Get_RandomNum();
		r=i>>11&0x1F;
		g=(i>>5&0x3F)>>1;
		b=i&0x1F;
	}while(r+g+b>0x1F*2);
	return i;
}
void drawchannelgrid()
{
	u8 i;
	LCD_Fill(0,90,480,800,WHITE);
	POINT_COLOR=LGRAY;
	for(i=0;i<=10;i++)
		LCD_DrawLine(X0,Y0+DY*i,X0+DX*16,Y0+DY*i);
	for(i=0;i<=16;i++)
		LCD_DrawLine(X0+DX*i,Y0,X0+DX*i,Y0+DY*10);
	POINT_COLOR=BLACK;
	for(i=0;i<=10;i++)
		LCD_ShowString(X0-strlen((char *)labels[i])*LFSIZE/2,Y0+DY*i-LFSIZE/2,strlen((char *)labels[i])*LFSIZE/2,LFSIZE,LFSIZE,labels[i]);
	for(i=2;i<=14;i++)
		LCD_ShowString(X0+DX*i-strlen((char *)labels[9+i])*LFSIZE/4,Y0+DY*10,strlen((char *)labels[9+i])*LFSIZE/2,LFSIZE,LFSIZE,labels[9+i]);
	LCD_ShowString(5,Y0+DY*10+LFSIZE,strlen((char *)labels[24])*LFSIZE/2,LFSIZE,LFSIZE,labels[24]);
	LCD_ShowString(X0+DX*8-strlen((char *)labels[25])*LFSIZE/4,Y0+DY*10+LFSIZE,strlen((char *)labels[25])*LFSIZE/2,LFSIZE,LFSIZE,labels[25]);
}
void drawtrapeziod(colorinfo *d)
{
	char *ssid=d->wifi.ssid;
	u8 channel=d->wifi.channel,rssi=d->wifi.rssi,ssidlen=strlen(ssid);
	u16 r=256-rssi,xb=X0+DX*channel,xa=xb-DX,xc=xb+DX*2,xd=xc+DX,
		ya=Y0+DY*10,yb=Y0+DY*r/10,yc=yb,yd=ya;
	if(!(rssi^0x80))
		return;
	POINT_COLOR=d->color;
	LCD_DrawLine(xa,ya,xb,yb);
	LCD_DrawLine(xb,yb,xc,yc);
	LCD_DrawLine(xc,yc,xd,yd);
	LCD_ShowString(xb+DX-ssidlen*LFSIZE/4,yb-LFSIZE,ssidlen*LFSIZE/2,LFSIZE,LFSIZE,(u8 *)ssid);
}

void drawui(u8 mode,colorqueue **cq0,colorqueue **cq1,wifiqueue *q)
{
		colorqueue *cq2;
		colorinfo d;
		u8 i=0,j=0,k;
		static u8 time=0;
		while(i<q->len&&j<(*cq0)->len)
		{
			if(q->pointer[i]->mac<(*cq0)->data[j].wifi.mac)
			{
				d.wifi=*q->pointer[i];
				d.color=getrandomcolor();
				memset(d.strength,0,TLEN*sizeof(char));
				d.strength[time]=d.wifi.rssi;
				adddata(*cq1,&d);
				i++;
			}
			else if(q->pointer[i]->mac==(*cq0)->data[j].wifi.mac)
			{
				(*cq0)->data[j].wifi=*q->pointer[i];
				(*cq0)->data[j].strength[time]=(*cq0)->data[j].wifi.rssi;
				adddata(*cq1,&(*cq0)->data[j]);
				i++;
				j++;
			}
			else
			{
				(*cq0)->data[j].wifi.rssi=(*cq0)->data[j].strength[time]=0x80;
				for(k=0;k<TLEN;k++)
					if((*cq0)->data[j].strength[time]^0x80)
						break;
				if(k!=TLEN)
					adddata(*cq1,&(*cq0)->data[j]);
				j++;
			}
		}
		while(i<q->len)
		{
				d.wifi=*q->pointer[i];
				d.color=getrandomcolor();
				memset(d.strength,0,TLEN*sizeof(char));
				d.strength[time]=d.wifi.rssi;
				adddata(*cq1,&d);
				i++;
		}
		while(j<(*cq0)->len)
		{
				(*cq0)->data[j].wifi.rssi=(*cq0)->data[j].strength[time]=0x80;
				for(k=0;k<TLEN;k++)
					if((*cq0)->data[j].strength[time]^0x80)
						break;
				if(k!=TLEN)
					adddata(*cq1,&(*cq0)->data[j]);
				j++;
		}
		switch(mode)
		{
			case 1:
				drawchannelgrid();
				for(i=0;i<(*cq1)->len;i++)
					drawtrapeziod(&(*cq1)->data[i]);
				break;
		}
		cq2=*cq1;
		*cq1=*cq0;
		*cq0=cq2;
		(*cq1)->len=0;
		time=(time+1)%TLEN;
}
