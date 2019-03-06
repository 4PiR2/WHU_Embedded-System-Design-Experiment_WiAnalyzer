#include "includes.h"
#include "ui.h"
#include "lcd.h"
#include "rng.h"

u16 X0=38,Y0=110,DX=27,DY=65,LFSIZE=16;
u8 *labels[]={"0","-10","-20","-30","-40","-50","-60","-70","-80","-90","-100","1","2","3","4","5","6","7","8","9","10","11","12","13","RSSI/dBm","Channels"};

static char adddata(colorqueue *cq,colorinfo *d)
{
	if(cq->len>=CQLEN)
		return -1;
	cq->data[cq->len]=*d;
	cq->len++;
	return 0;
}
static short max(colorqueue *cq,short n,short i,short j,short k)
{
	short m=i;
	if(j<n&&cq->data[j].mac>cq->data[m].mac)
		m=j;
	if(k<n&&cq->data[k].mac>cq->data[m].mac)
		m=k;
	return m;
}
static void downheap(colorqueue *cq,short n,short i)
{
	colorinfo t;
	while(1)
	{
		short j=max(cq,n,i,2*i+1,2*i+2);
		if(j==i)
			break;
		t=cq->data[i];
		cq->data[i]=cq->data[j];
		cq->data[j]=t;
		i=j;
	}
}
static void heapsort(colorqueue *cq,short n)
{
	colorinfo t;
	short i;
	if(n<2)
		return;
	for(i=(n-2)/2;i>=0;i--)
		downheap(cq,n,i);
	for(i=0;i<n;i++)
	{
		t=cq->data[n-i-1];
		cq->data[n-i-1]=cq->data[0];
		cq->data[0]=t;
		downheap(cq,n-i-1,0);
	}
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
void drawtrapeziod(u8 channel,u8 rssi,char *ssid)
{
	u16 r=256-rssi,xb=X0+DX*channel,xa=xb-DX,xc=xb+DX*2,xd=xc+DX,
		ya=Y0+DY*10,yb=Y0+DY*r/10,yc=yb,yd=ya;
	u8 ssidlen=strlen(ssid);
	LCD_DrawLine(xa,ya,xb,yb);
	LCD_DrawLine(xb,yb,xc,yc);
	LCD_DrawLine(xc,yc,xd,yd);
	LCD_ShowString(xb+DX-ssidlen*LFSIZE/4,yb-LFSIZE,ssidlen*LFSIZE/2,LFSIZE,LFSIZE,(u8 *)ssid);
}

void drawui(colorqueue **cq0,colorqueue **cq1,wifiqueue *q)
{
		colorqueue *cq2;
		colorinfo d;
		u8 i,j;
		LCD_Fill(0,Y0-LFSIZE,480,Y0+DY*10,WHITE);
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
		i=j=0;
		while(i<q->len&&j<(*cq0)->len)
		{
			if(q->pointer[i]->mac<(*cq0)->data[j].mac)
			{
				d.mac=q->pointer[i]->mac;
				POINT_COLOR=d.color=getrandomcolor();
				drawtrapeziod(q->pointer[i]->channel,q->pointer[i]->rssi,q->pointer[i]->ssid);
				adddata(*cq1,&d);
				i++;
			}
			else if(q->pointer[i]->mac==(*cq0)->data[j].mac)
			{
				POINT_COLOR=(*cq0)->data[j].color;
				drawtrapeziod(q->pointer[i]->channel,q->pointer[i]->rssi,q->pointer[i]->ssid);
				adddata(*cq1,&(*cq0)->data[j]);
				i++;j++;
			}
			else
				j++;
		}
		while(i<q->len)
		{
				d.mac=q->pointer[i]->mac;
				POINT_COLOR=d.color=getrandomcolor();
				drawtrapeziod(q->pointer[i]->channel,q->pointer[i]->rssi,q->pointer[i]->ssid);
				adddata(*cq1,&d);
				i++;
		}
		cq2=*cq1;
		*cq1=*cq0;
		*cq0=cq2;
		(*cq1)->len=0;
}
