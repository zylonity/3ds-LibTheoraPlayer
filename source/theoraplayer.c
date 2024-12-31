#include "theoraplayer.h"
#include <SDL/SDL.h>
#include <SDL/SDL_audio.h>

THEORA_Context THEORA_vidCtx;
TH3DS_Frame THEORA_frame;
Thread THEORA_vthread = NULL;
Thread THEORA_athread = NULL;
size_t g_bufferSize = 8 * 4096;
ndspWaveBuf THEORA_waveBuf[WAVEBUFCOUNT];
int16_t *g_audioBuffer;
LightEvent THEORA_soundEvent;
int THEORA_ready = 0;
float THEORA_scaleframe = 1.0f;
int THEORA_isplaying = false;
size_t g_bufferPos = 0;             
int g_audioChannels = 0;

static inline float TP_getFrameScalef(float wi, float hi, float targetw, float targeth)
{
    float w = targetw / wi;
    float h = targeth / hi;
    return fabs(w) > fabs(h) ? h : w;
}

int audioDevice; // SDL audio device handle

void TP_queueAudio(const int16_t *buffer, size_t size)
{
    // Queue the audio data to the SDL audio device
    if (SDL_QueueAudio(audioDevice, buffer, size * sizeof(int16_t)) < 0)
    {
        fprintf(stderr, "Failed to queue audio: %s\n", SDL_GetError());
    }
}

void TP_audioInit(THEORA_audioinfo *ainfo)
{
    SDL_AudioSpec desiredSpec;

    desiredSpec.freq = ainfo->rate;         // Set the sample rate
    desiredSpec.format = AUDIO_S16SYS;      // 16-bit signed audio
    desiredSpec.channels = ainfo->channels; // Mono or Stereo
    desiredSpec.samples = 4096;             // Buffer size
    desiredSpec.callback = audioCallback;   // Audio callback function
    desiredSpec.userdata = NULL;            // No userdata needed

    if (SDL_OpenAudio(&desiredSpec, NULL) < 0)
    {
        fprintf(stderr, "Failed to open audio: %s\n", SDL_GetError());
        return;
    }

    // Assign global variables
    g_audioBuffer = audioBuffer;
    g_bufferSize = bufferSize;
    g_bufferPos = 0;
    g_audioChannels = ainfo->channels;

    SDL_PauseAudio(0); // Start audio playback
}

void TP_audioClose(void)
{
    SDL_CloseAudioDevice(audioDevice);
}

void TP_videoDecode_thread(void *nul)
{
    THEORA_videoinfo *vinfo = THEORA_vidinfo(&THEORA_vidCtx);
    THEORA_audioinfo *ainfo = THEORA_audinfo(&THEORA_vidCtx);

    if (THEORA_HasAudio(&THEORA_vidCtx))
        TP_audioInit(ainfo);

    if (THEORA_HasVideo(&THEORA_vidCtx))
    {
        printf("Ogg stream is Theora %dx%d %.02f fps\n", vinfo->width, vinfo->height, vinfo->fps);
        frameInit(&THEORA_frame, vinfo);
        THEORA_scaleframe = TP_getFrameScalef(vinfo->width, vinfo->height, SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    THEORA_isplaying = true;

    while (THEORA_isplaying)
    {
        if (THEORA_eos(&THEORA_vidCtx))
            break;

        if (THEORA_HasVideo(&THEORA_vidCtx))
        {
            th_ycbcr_buffer ybr;
            if (THEORA_getvideo(&THEORA_vidCtx, ybr))
            {
                frameWrite(&THEORA_frame, vinfo, ybr);
            }
        }

        if (THEORA_HasAudio(&THEORA_vidCtx))
        {
            for (int cur_wvbuf = 0; cur_wvbuf < WAVEBUFCOUNT; cur_wvbuf++)
            {
                int16_t audioBuffer[g_bufferSize]; // Temporary buffer for decoded audio
                size_t read = THEORA_readaudio(&THEORA_vidCtx, (char *)audioBuffer, g_bufferSize);

                if (read > 0)
                {
                    TP_queueAudio(audioBuffer, read / sizeof(int16_t)); // Queue the decoded audio
                }
            }
        }
        SDL_Delay(10);
    }

    printf("frames: %d dropped: %d\n", THEORA_vidCtx.frames, THEORA_vidCtx.dropped);
    THEORA_isplaying = false;

    if (THEORA_HasVideo(&THEORA_vidCtx))
        frameDelete(&THEORA_frame);

    if (THEORA_HasAudio(&THEORA_vidCtx))
        TP_audioClose();

    THEORA_Close(&THEORA_vidCtx);
    threadExit(0);
}

void TP_audioCallback(void *const arg_)
{
    // (void)arg_;

    // if (!THEORA_isplaying)
    //     return;

    // LightEvent_Signal(&THEORA_soundEvent);
    // Determine how much data is remaining in the buffer
    size_t remaining = g_bufferSize - g_bufferPos;
    size_t toCopy = (len > remaining) ? remaining : len;

    // Copy the audio data to the stream
    SDL_memcpy(stream, (Uint8 *)(g_audioBuffer + g_bufferPos), toCopy);

    // Advance the buffer position
    g_bufferPos += toCopy / sizeof(int16_t);

    // If there's no more audio, fill the rest with silence
    if (toCopy < len)
    {
        SDL_memset(stream + toCopy, 0, len - toCopy);
    }
}

/*void audioDecode_thread(void* nul) {
    THEORA_audioinfo* ainfo = THEORA_audinfo(&THEORA_vidCtx);

    if (THEORA_HasAudio(&THEORA_vidCtx))
        TP_audioInit(ainfo);

    while (THEORA_isplaying) {
        if (THEORA_HasAudio(&THEORA_vidCtx)) {
            for (int cur_wvbuf = 0; cur_wvbuf < WAVEBUFCOUNT; cur_wvbuf++) {
                ndspWaveBuf *buf = &THEORA_waveBuf[cur_wvbuf];

                if(buf->status == NDSP_WBUF_DONE) {
                    __lock_acquire(oggMutex);
                    size_t read = THEORA_readaudio(&THEORA_vidCtx, buf->data_pcm16, THEORA_buffSize);
                    __lock_release(oggMutex);
                    if(read <= 0)
                        break;
                    else if(read <= THEORA_buffSize)
                        buf->nsamples = read / ainfo->channels;

                    ndspChnWaveBufAdd(0, buf);
                }
                DSP_FlushDataCache(buf->data_pcm16, THEORA_buffSize * sizeof(int16_t));
            }
            LightEvent_Wait(&THEORA_soundEvent);
        }
    }

    //TP_audioClose();

    threadExit(0);
}*/

void TP_exitThread(void)
{
    THEORA_isplaying = false;

    if (!THEORA_vthread && !THEORA_athread)
        return;

    threadJoin(THEORA_vthread, U64_MAX);
    threadFree(THEORA_vthread);

    LightEvent_Signal(&THEORA_soundEvent);
    threadJoin(THEORA_athread, U64_MAX);
    threadFree(THEORA_athread);

    THEORA_vthread = NULL;
    THEORA_athread = NULL;
}

static int TP_isOgg(const char *filepath)
{
    FILE *fp = fopen(filepath, "r");
    char magic[16];

    if (!fp)
    {
        printf("Could not open %s. Please make sure file exists.\n", filepath);
        return 0;
    }

    fseek(fp, 0, SEEK_SET);
    fread(magic, 1, 16, fp);
    fclose(fp);

    if (!strncmp(magic, "OggS", 4))
        return 1;

    return 0;
}

void TP_changeFile(const char *filepath)
{
    int ret = 0;

    if (THEORA_vthread != NULL || THEORA_athread != NULL)
        TP_exitThread();

    if (!TP_isOgg(filepath))
    {
        printf("The file is not an ogg file.\n");
        return;
    }

    if ((ret = THEORA_Create(&THEORA_vidCtx, filepath)))
    {
        printf("THEORA_Create exited with error, %d.\n", ret);
        return;
    }

    if (!THEORA_HasVideo(&THEORA_vidCtx) && !THEORA_HasAudio(&THEORA_vidCtx))
    {
        printf("No audio or video stream could be found.\n");
        return;
    }

    printf("Theora Create sucessful.\n");

    s32 prio;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    THEORA_vthread = threadCreate(TP_videoDecode_thread, NULL, 32 * 1024, prio - 1, -1, false);
    // THEORA_athread = threadCreate(audioDecode_thread, NULL, 32 * 1024, prio-1, -1, false);
}