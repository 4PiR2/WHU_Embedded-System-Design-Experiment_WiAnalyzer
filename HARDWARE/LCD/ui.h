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
	short len;
	char time;
} colorqueue;

void drawui(u8 mode,colorqueue **cq0,colorqueue **cq1,wifiqueue *q);
