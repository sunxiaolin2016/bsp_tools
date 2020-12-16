#ifndef __CAMDEV_H__
#define __CAMDEV_H__

#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>
#include <ui/DisplayInfo.h>
#include <ui/GraphicBufferMapper.h>

using namespace android;

typedef int (*CAMDEV_CAPTURE_CALLBACK)(void *recorder, void *data[8], int linesize[8], int64_t pts);

enum {
    CAMDEV_PARAM_VIDEO_WIDTH,
    CAMDEV_PARAM_VIDEO_HEIGHT,
    CAMDEV_PARAM_VIDEO_PIXFMT,
    CAMDEV_PARAM_VIDEO_FRATE_NUM,
    CAMDEV_PARAM_VIDEO_FRATE_DEN,
};


//void* camdev_init (const char *dev, int sub, int w, int h, int frate, const char *output);
void* camdev_init(const char *dev, int sub, int w, int h, int frate, const char *output,
	int pix_fmt, int input_interval, int fix_input, int gra_pix_fmt);

void  camdev_close(void *ctxt);

void  camdev_set_preview_window(void *ctxt, const sp<ANativeWindow> win);
void  camdev_set_preview_target(void *ctxt, const sp<IGraphicBufferProducer>& gbp);

void  camdev_capture_start(void *ctxt);
void  camdev_capture_stop (void *ctxt);
void  camdev_preview_start(void *ctxt);
void  camdev_preview_stop (void *ctxt);
int camdev_capture_frame(void *ctxt);
int camdev_preview_frame(void *ctxt);

int get_pix_format(char* fmt);
const char* get_pix_fmt_string(int fmt);

#endif
