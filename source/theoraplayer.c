#include "theoraplayer.h"

THEORA_Context THEORA_vidCtx;
TH3DS_Frame THEORA_frame;
Thread THEORA_vthread = NULL;
Thread THEORA_athread = NULL;
size_t THEORA_buffSize = 8 * 4096;
ndspWaveBuf THEORA_waveBuf[WAVEBUFCOUNT];
int16_t *THEORA_audioBuffer;
LightEvent THEORA_soundEvent;
int THEORA_ready = 0;
float THEORA_scaleframe = 1.0f;
int THEORA_isplaying = false;


static inline float TP_getFrameScalef(float wi, float hi, float targetw, float targeth)
{
    float w = targetw / wi;
    float h = targeth / hi;
    return fabs(w) > fabs(h) ? h : w;
}


void TP_audioInit(THEORA_audioinfo *ainfo)
{
    ndspChnReset(0);
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnSetInterp(0, ainfo->channels == 2 ? NDSP_INTERP_POLYPHASE : NDSP_INTERP_LINEAR);
    ndspChnSetRate(0, ainfo->rate);
    ndspChnSetFormat(0, ainfo->channels == 2 ? NDSP_FORMAT_STEREO_PCM16 : NDSP_FORMAT_MONO_PCM16);
    THEORA_audioBuffer = linearAlloc((THEORA_buffSize * sizeof(int16_t)) * WAVEBUFCOUNT);

    memset(THEORA_waveBuf, 0, sizeof(THEORA_waveBuf));
    for (unsigned i = 0; i < WAVEBUFCOUNT; ++i)
    {
        THEORA_waveBuf[i].data_vaddr = &THEORA_audioBuffer[i * THEORA_buffSize];
        THEORA_waveBuf[i].nsamples = THEORA_buffSize;
        THEORA_waveBuf[i].status = NDSP_WBUF_DONE;
    }
}

void TP_audioClose(void)
{
    ndspChnReset(0);
    if (THEORA_audioBuffer)
        linearFree(THEORA_audioBuffer);
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
                ndspWaveBuf *buf = &THEORA_waveBuf[cur_wvbuf];

                if (buf->status == NDSP_WBUF_DONE)
                {
                    //__lock_acquire(oggMutex);
                    size_t read = THEORA_readaudio(&THEORA_vidCtx, (char *)buf->data_pcm16, THEORA_buffSize);
                    //__lock_release(oggMutex);
                    if (read <= 0)
                        break;
                    else if (read <= THEORA_buffSize)
                        buf->nsamples = read / ainfo->channels;

                    ndspChnWaveBufAdd(0, buf);
                }
                DSP_FlushDataCache(buf->data_pcm16, THEORA_buffSize * sizeof(int16_t));
            }
        }
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
    (void)arg_;

    if (!THEORA_isplaying)
        return;

    LightEvent_Signal(&THEORA_soundEvent);
}


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
}