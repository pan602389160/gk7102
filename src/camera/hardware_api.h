
#ifndef HARDWARE_API_H
#define HARDWARE_API_H


//----------------------------------------------------------
// Video capture api.

typedef int (*video_capture_callback)(void *context, void *data, int len, int is_key_frame);

int hardware_video_capture_open(void **context, int camera_no, int width, int height,
	int bit_rate, int frame_rate, int key_frame_rate, video_capture_callback callback_func);
int hardware_video_capture_force_key_frame(void *context);
void hardware_video_capture_close(void *context);


//----------------------------------------------------------
// Audio record api.

typedef int (*audio_record_callback)(void *context, void *data, int len);

int hardware_audio_record_open(void **context, int microphone_no,
	int sample_rate, int frame_len, audio_record_callback callback_func);
void hardware_audio_record_close(void *context);


//----------------------------------------------------------
// Audio play api.

int hardware_audio_play_open(void **context, int speaker_no, int sample_rate, int frame_len);
int hardware_audio_play_write_data(void *context, void *data, int len);
void hardware_audio_play_close(void *context);


#endif //HARDWARE_API_H