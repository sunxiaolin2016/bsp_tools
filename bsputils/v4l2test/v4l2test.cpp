#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <cutils/properties.h>
#include <linux/videodev2.h>
#include <signal.h>
#include <stdlib.h>

#include "camdev.h"
#include "graphics_ext.h"

/* NOTES: THIS PROGRAM ONLY USED TO DEBUG DVR OR RVM VIDEO QUALITY ISSUE! */

static void usage(char* base)
{
	printf("%s -d /dev/video0 -X 0 -Y 0 -W 1920 -H 720 -x 0 -y 0 -w 720 -h 480 -p RGB656 -P graph_pix -i 3 -I input -o /data/save_stream_file.\n"\
		"-X, -Y, -W, -H set window position.\n"
		"-x, -y, -w, -h set stream param.\n"\
		"-p set pix format used by VIDIOC_S_FMT, can be RGB656, RGB888, YUYV, XYUV, NV12, YUV444, XBGR32, ABGR32\n"
		"-i set auto change input interval in second.\n"\
		"-I set fix input source, can only be 0 or 1.\n"\
		"-o if need save stream, -o set save file name.\n"\
		"-P graph pix format(default 0x14)\n"\
		"NOTES: THIS PROGRAM ONLY USED TO DEBUG DVR OR RVM VIDEO QUALITY ISSUE!\n",
		base);
	exit(-1);
}

int get_pix_format(char* fmt)
{
	printf("fmt:%s\n",fmt);

	/* 0. flags: 0, description: 16-bit RGB 5-6-5, pixelfmt: 0x50424752		//V4L2_PIX_FMT_RGB565 */
	if (!strcasecmp(fmt, "RGB656"))
		return V4L2_PIX_FMT_RGB565;

	/* 1. flags: 0, description: 24-bit RGB 8-8-8, pixelfmt: 0x33424752		//V4L2_PIX_FMT_RGB24	*/
	if (!strcasecmp(fmt, "RGB888"))
		return V4L2_PIX_FMT_RGB24;

	/* 3. flags: 0, description: YUYV 4:2:2      , pixelfmt: 0x56595559		//V4L2_PIX_FMT_YUYV */
	if (!strcasecmp(fmt, "YUYV"))
		return V4L2_PIX_FMT_YUYV;

	/* 4. flags: 0, description: 32-bit A/XYUV 8-8-8-8, pixelfmt: 0x34565559	//V4L2_PIX_FMT_YUV32 */
	if (!strcasecmp(fmt, "XYUV"))
		return V4L2_PIX_FMT_YUV32;

	if (!strcasecmp(fmt, "UYVY"))
		return V4L2_PIX_FMT_UYVY;

	/* 5. flags: 0, description: Y/CbCr 4:2:0    , pixelfmt: 0x3231564e			//V4L2_PIX_FMT_NV12 */
	if (!strcasecmp(fmt, "NV12"))
		return V4L2_PIX_FMT_NV12;

	/* 6. flags: 0, description: Planar YUV 4:4:4 (N-C), pixelfmt: 0x34324d59	//V4L2_PIX_FMT_YUV444M */
	if (!strcasecmp(fmt, "YUV444"))
		return V4L2_PIX_FMT_YUV444M;

	/* 7. flags: 0, description: 32-bit BGRX 8-8-8-8, pixelfmt: 0x34325258		//V4L2_PIX_FMT_XBGR32 */
	if (!strcasecmp(fmt, "XBGR32"))
		return V4L2_PIX_FMT_XBGR32;

	/* 8. flags: 0, description: 32-bit BGRA 8-8-8-8, pixelfmt: 0x34325241		//V4L2_PIX_FMT_ABGR32 */
	if (!strcasecmp(fmt, "ABGR32"))
		return V4L2_PIX_FMT_ABGR32;

	/* default */
	return V4L2_PIX_FMT_RGB565;
}
const char* get_pix_fmt_string(int fmt)
{

	switch(fmt) {
	case V4L2_PIX_FMT_RGB565: return "RGB656";
	case V4L2_PIX_FMT_RGB24 : return "RGB888";
	case V4L2_PIX_FMT_YUYV	: return "YUYV";

	case V4L2_PIX_FMT_UYVY: return "UYVY";

	case V4L2_PIX_FMT_YUV32	: return "XYUV";
	case V4L2_PIX_FMT_NV12 	: return "NV12";
	case V4L2_PIX_FMT_YUV444M : return "YUV444";
	case V4L2_PIX_FMT_XBGR32 : return "XBGR32";
	case V4L2_PIX_FMT_ABGR32 : return "ABGR32";
	default: return "Unkown Pix Format";
	}
}

static void parse_args(int argc, char* argv[],
	char* dev, int dev_len,
	int *X, int *Y, int *W, int *H,
	int *x, int *y, int *w, int *h,
	int *pix_fmt,
	int *change_input_interval, int* fix_input,
	int *gra_pix_fmt,
	char* save_file, int save_file_len)
{
	int flags, opt;
	extern char *optarg;
	
	while ((opt = getopt(argc, argv, "d:X:Y:W:H:x:y:w:h:p:i:I:o:P:")) != -1) {
		switch (opt) {
		case 'd': 
				if ((int)strlen(optarg) + 1 > dev_len) {
					printf("invalid %s", optarg);
					usage(argv[0]);
				} else {
					strcpy(dev, optarg);
				}
				break;
		case 'X': *X = atoi(optarg);break;
		case 'Y': *Y = atoi(optarg);break;
		case 'W': *W = atoi(optarg);break;
		case 'H': *H = atoi(optarg);break;

		case 'x': *x = atoi(optarg);break;
		case 'y': *y = atoi(optarg);break;
		case 'w': *w = atoi(optarg);break;
		case 'h': *h = atoi(optarg);break;

		case 'i': *change_input_interval = atoi(optarg);break;
		case 'I':
				*fix_input = atoi(optarg);
				if (*fix_input != 0 && *fix_input != 1)
					usage(argv[0]);
				break;
		case 'p': *pix_fmt = get_pix_format(optarg); break;
		case 'P': printf("%s->%d, optart:%s.\n",__func__,__LINE__,optarg);
				//*gra_pix_fmt = atoi(optarg); break;
				*gra_pix_fmt = strtol(optarg, NULL, 16); break;
		case 'o': 
				if ((int)strlen(optarg) + 1 > save_file_len)
					usage(argv[0]);
				else 
					strcpy(save_file, optarg);
				break;
		default:
				usage(argv[0]);
		}
	}
}
static void   *cam = NULL;
void camdev_exit()
{
	printf("%s->%d\n",__func__,__LINE__);

    // stoppreview
    camdev_preview_stop(cam);
    camdev_capture_stop(cam);

    // close camdev
    camdev_close(cam);

}
void sig_func(int sig)
{

	printf("%s->%d\n",__func__,__LINE__);
	fflush(stdout);
	camdev_exit();
}

int main(int argc, char *argv[])
{
    char   o[128] = {0};
    char    dev[32] = "/dev/video0";
    int     w       = 1280;
    int     h       = 960;
	int     x       = 0;
    int     y      	= 0;
	int 	X 		= 0;
	int 	Y		= 0;
	int 	W		= 1280;
	int 	H		= 960;
	int 	pix_fmt = V4L2_PIX_FMT_YUYV;
	//int 	pix_fmt = V4L2_PIX_FMT_UYVY;
	int 	gra_pix_fmt = HAL_PIXEL_FORMAT_CbYCrY_422_I;
	int 	input_interval = -1;
	int 	fix_input = -1;

	signal(15, sig_func);
	fflush(stdout);

	//atexit(camdev_exit);

	parse_args(argc, argv, dev, sizeof(dev), 
		&X, &Y, &W, &H,
		&x, &y, &w, &h,
		&pix_fmt,  
		&input_interval, &fix_input,
		&gra_pix_fmt,
		o, sizeof(o));

	if (fix_input >= 0) {
		printf("fix input set, ignore auto change input!\n");
		input_interval = -1;
	}

	printf("command:%s dev:%s X:%d, Y:%d, W:%d, H:%d, x:%d y:%d w:%d h:%d video_pix_fmt:%s gra_pix_fmt:%d(%x) input_interval:%d fix_input:%d o:%s\n",
		argv[0], dev, 
		X, Y, W, H,
		x, y, w, h,
		get_pix_fmt_string(pix_fmt),
		gra_pix_fmt,gra_pix_fmt,
		input_interval, fix_input, o);

    // create a client to surfaceflinger
    sp<SurfaceComposerClient> client = new SurfaceComposerClient();
    sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(
            ISurfaceComposer::eDisplayIdMain));

    DisplayInfo dinfo;
    status_t status = SurfaceComposerClient::getDisplayInfo(dtoken, &dinfo);
    if (status) {
        return -1;
    }

//    sp<SurfaceControl> surfaceControl = client->createSurface(String8("test_v4l2"),
//						dinfo.w, dinfo.h, PIXEL_FORMAT_RGB_565);

    sp<SurfaceControl> surfaceControl = client->createSurface(String8("test_v4l2"),
						dinfo.w, dinfo.h, gra_pix_fmt);//HAL_PIXEL_FORMAT_YCBCR_422_I);//HAL_PIXEL_FORMAT_CbYCrY_422_I

    SurfaceComposerClient::Transaction t;
    t.setLayer(surfaceControl, 0x40000001);
	t.setPosition(surfaceControl, X, Y);
	t.setSize(surfaceControl, W, H);
	t.show(surfaceControl);
    t.apply();
    //SurfaceComposerClient::closeGlobalTransaction();

    sp<Surface>   surface = surfaceControl->getSurface();
    sp<ANativeWindow> win = surface;

    // init camdev
    cam = camdev_init(dev, 0, w, h, 25, o, pix_fmt, input_interval, fix_input, gra_pix_fmt);
    if (!cam) {
        printf("failed to create camdev !\n");
        exit(1);
    }

    // startpreview
    camdev_set_preview_window(cam, win);
    camdev_capture_start(cam);
    camdev_preview_start(cam);

    // wait exit
    while (1) {
        int ret;
 
        ret = camdev_capture_frame(cam);

        if (ret < 0)
            break;

        if (!ret)
            continue;

        ret = camdev_preview_frame(cam);

        if (ret < 0)
            break;
    }

    // stoppreview
    camdev_preview_stop(cam);
    camdev_capture_stop(cam);

    // close camdev
    camdev_close(cam);

    return 0;
}

