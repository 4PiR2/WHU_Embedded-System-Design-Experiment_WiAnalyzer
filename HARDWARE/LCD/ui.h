#include "wifi.h"
#define CQLEN 30
#define TLEN 20

typedef struct
{
	wifiinfo wifi;
	char strength[TLEN];
	int color;
} colorinfo;
typedef struct
{
	colorinfo data[CQLEN];
	colorinfo *rssiindex[QLEN];
	short len,indexlen;
	char time;
} colorqueue;

void prepareui(colorqueue **cq0,colorqueue **cq1,wifiqueue *q);
void drawui(u8 mode,colorqueue **cq0,colorqueue **cq1);
