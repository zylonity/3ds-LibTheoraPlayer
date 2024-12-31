#ifndef _theoraplayer_h_
#define _theoraplayer_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "video.h"
#include "frame.h"

#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 240

#define WAVEBUFCOUNT 3
#define MAX_LIST 28

extern THEORA_Context THEORA_vidCtx;
extern TH3DS_Frame THEORA_frame;
extern Thread THEORA_vthread;
extern Thread THEORA_athread;
extern size_t THEORA_buffSize;
extern ndspWaveBuf THEORA_waveBuf[WAVEBUFCOUNT];
extern int16_t *THEORA_audioBuffer;
extern LightEvent THEORA_soundEvent;
extern int THEORA_ready;
extern float THEORA_scaleframe;
extern int THEORA_isplaying;


static inline float TP_getFrameScalef(float wi, float hi, float targetw, float targeth);
void TP_audioInit(THEORA_audioinfo *ainfo);
void TP_audioClose(void);
void TP_videoDecode_thread(void *nul);
void TP_audioCallback(void *const arg_);
void TP_exitThread(void);
static int TP_isOgg(const char *filepath);
extern void TP_changeFile(const char *filepath);

#ifdef __cplusplus
}
#endif

#endif // _theoraplayer_h_