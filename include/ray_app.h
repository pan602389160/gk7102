#ifndef __RAY_APP_H__
#define __RAY_APP_H__

typedef enum app_t{
	MUSICPLAYER = 1,
	DMR,
	DMC,
	DMS,
	AIRPLAY,
	LOCALPLAYER,
	BT,
	VR,
	INGENICPLAYER,
	LAPSULE,
	MULROOM,
}app_t;

typedef struct
{
	char saoma_buf_ssid[32];
	char saoma_buf_psk[32];
	int flag;
	pthread_t tid;
}SAOMA_INFO_S;

SAOMA_INFO_S saoma_info;


extern int start(app_t app);
extern int stop(app_t app);
extern int startall();
extern int stopall();
extern int restartall();

#endif //_APP_H_

