#ifndef _AEC_H_
#define _AEC_H_


#define AEC_SIZE 1024

#define AEC_CHANNEL	       1

#define AEC_RATE            16000

#define AEC_BIT             16

#define AEC_VOLUME          60

typedef struct aec_info_t {
	bool wakeup;
}aec_info_t;

extern int aec_start(void);
extern int aec_stop(void);
extern int aec_feed(const void *rec, const void *play, int size);
extern int aec_key_wakeup(void);
#endif

