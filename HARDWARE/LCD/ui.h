#include "wifi.h"
#define CQLEN 20

typedef struct
{
	long long mac;
	int color;
} colorinfo;
typedef struct
{
	colorinfo data[CQLEN];
	short len;
} colorqueue;

void drawui(colorqueue **cq0,colorqueue **cq1,wifiqueue *q);
