#define LOG_TAG "camdev"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
//#include <utils/Log.h>
#include "camdev.h"
#include<sys/mman.h>
#include <linux/videodev2.h>

#define DO_USE_VAR(v)   do { v = v; } while (0)
#define VIDEO_CAPTURE_BUFFER_COUNT  3
#define NATIVE_WIN_BUFFER_COUNT     3
//#define DEF_WIN_PIX_FMT         HAL_PIXEL_FORMAT_YCrCb_420_SP // HAL_PIXEL_FORMAT_RGBX_8888 or HAL_PIXEL_FORMAT_YCrCb_420_SP
#define DEF_WIN_PIX_FMT         HAL_PIXEL_FORMAT_RGBX_8888
#define CAMDEV_GRALLOC_USAGE    (GRALLOC_USAGE_SW_READ_NEVER | GRALLOC_USAGE_SW_WRITE_NEVER | GRALLOC_USAGE_HW_TEXTURE)



struct video_buffer {
    void    *addr;
    unsigned len;
};

// camdev context
typedef struct {
    FILE *fp;
    struct v4l2_buffer      buf;
    struct video_buffer     vbs[VIDEO_CAPTURE_BUFFER_COUNT];
    int                     fd;
    sp<ANativeWindow>       new_win;
    sp<ANativeWindow>       cur_win;
    int                     win_w;
    int                     win_h;
    #define CAMDEV_TS_EXIT       (1 << 0)  // exit threads
    #define CAMDEV_TS_PAUSE_C    (1 << 1)  // pause capture
    #define CAMDEV_TS_PAUSE_P    (1 << 2)  // pause preview
    #define CAMDEV_TS_PREVIEW    (1 << 3)  // enable preview
    sem_t                   sem_render;
    sem_t                   sem_buf;

    pthread_t               thread_id_render;
    pthread_t               thread_id_capture;
	pthread_t				thread_id_switch_camera;
	int 					switch_facing_interval_sec;
    int                     thread_state;
    int                     update_flag;
    int                     cam_pixfmt;
	int 					gra_pix_fmt;
    int                     cam_w;
    int                     cam_h;
    int                     cam_frate_num; // camdev frame rate num get from v4l2 interface
    int                     cam_frate_den; // camdev frame rate den get from v4l2 interface
    CAMDEV_CAPTURE_CALLBACK callback;

    uint32_t stat_capture_count;
} CAMDEV;

static void* camdev_capture_thread_proc(void *param)
{
    CAMDEV  *cam = (CAMDEV*)param;
    int      err;

    //++ for select
    fd_set        fds;
    struct timeval tv;
    //-- for select

    while (!(cam->thread_state & CAMDEV_TS_EXIT)) {
        FD_ZERO(&fds);
        FD_SET (cam->fd, &fds);
        tv.tv_sec  = 1;
        tv.tv_usec = 0;

        if (select(cam->fd + 1, &fds, NULL, NULL, &tv) <= 0) {
            ALOGD("select error or timeout !\n");
            continue;
        }

        if (cam->thread_state & CAMDEV_TS_PAUSE_C) {
            cam->thread_state |= (CAMDEV_TS_PAUSE_C << 16);
            usleep(10*1000);
            continue;
        }

		sem_wait(&cam->sem_buf);
        // dequeue camera video buffer
        if (-1 == ioctl(cam->fd, VIDIOC_DQBUF, &cam->buf)) {
            ALOGD("failed to de-queue buffer !\n");
            continue;
        }

//      ALOGD("%d. bytesused: %d, sequence: %d, length = %d\n", cam->buf.index, cam->buf.bytesused,
//              cam->buf.sequence, cam->buf.length);
//      ALOGD("timestamp: %ld, %ld\n", cam->buf.timestamp.tv_sec, cam->buf.timestamp.tv_usec);
        if (cam->thread_state & CAMDEV_TS_PREVIEW) {
            sem_post(&cam->sem_render);
        }
    }

//  ALOGD("camdev_capture_thread_proc exited !");
    return NULL;
}

int camdev_capture_frame(void *ctxt)
{
    CAMDEV *cam = (CAMDEV*)ctxt;
    int      err;
    fd_set   fds;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET (cam->fd, &fds);
    tv.tv_sec  = 10;
    tv.tv_usec = 0;

    err = select(cam->fd + 1, &fds, NULL, NULL, &tv);

    if (err < 0) {
        ALOGE("select error %s !\n", strerror(errno));
        return err;
    }

    if (!err) {
        ALOGE("select timeout!\n");
        return 0;
    }

    // dequeue camera video buffer
    err = ioctl(cam->fd, VIDIOC_DQBUF, &cam->buf);
    if (err < 0) {
        ALOGD("failed to de-queue buffer %s !\n", strerror(errno));
        return err;
    }

    cam->stat_capture_count ++;
    return 1;
}

static void* camdev_render_thread_proc(void *param)
{
    CAMDEV  *cam = (CAMDEV*)param;
    int      err;

    while (!(cam->thread_state & CAMDEV_TS_EXIT)) {
        if (0 != sem_wait(&cam->sem_render)) {
            ALOGD("failed to wait sem_render !\n");
            break;
        }

        if (cam->thread_state & CAMDEV_TS_PAUSE_P) {
            cam->thread_state |= (CAMDEV_TS_PAUSE_P << 16);
            usleep(10*1000);
            continue;
        }

        if (cam->update_flag) {
            cam->cur_win = cam->new_win;
            if (cam->cur_win != NULL) {
				native_window_api_connect(cam->cur_win.get(), NATIVE_WINDOW_API_CPU);
				native_window_set_buffers_dimensions(cam->cur_win.get(), cam->win_w, cam->win_h);
				native_window_set_buffers_format(cam->cur_win.get(), HAL_PIXEL_FORMAT_RGBX_8888);
				native_window_set_usage(cam->cur_win.get(), GRALLOC_USAGE_SW_WRITE_OFTEN);
				native_window_set_scaling_mode(cam->cur_win.get(), NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW);
				native_window_set_buffer_count(cam->cur_win.get(), 3);
            }
            cam->update_flag = 0;
        }

        char *data = (char*)cam->vbs[cam->buf.index].addr;
        int   len  = cam->vbs[cam->buf.index].len;

        ANativeWindowBuffer *buf;
        if (cam->cur_win != NULL && 0 == native_window_dequeue_buffer_and_wait(cam->cur_win.get(), &buf)) {
		   sp<GraphicBuffer> test_buf(GraphicBuffer::from(buf));
            uint8_t* img = NULL;
            if (0 == test_buf->lock(GRALLOC_USAGE_SW_WRITE_OFTEN, (void**)(&img))) {
				memcpy(img, data, len);
			    test_buf->unlock();
            }

            if ((err = cam->cur_win->queueBuffer(cam->cur_win.get(), test_buf->getNativeBuffer(), -1)) != 0) {
                ALOGW("Surface::queueBuffer returned error %d\n", err);
            }


        }
    }

//  ALOGD("camdev_render_thread_proc exited !");
    return NULL;
}

static void* switch_camera_thread(void *param)
{
	CAMDEV  *cam = (CAMDEV*)param;    
	int      err;	
	int facing = 0;	

	while (!(cam->thread_state & CAMDEV_TS_EXIT)) {		
		sleep(cam->switch_facing_interval_sec);

		ALOGD("%s->%d, switch to facing %d.\n", __func__, __LINE__, facing);	   
		printf("%s->%d, switch to facing %d.\n", __func__, __LINE__, facing);      
		if (-1 == ioctl(cam->fd, VIDIOC_S_INPUT, &facing)) 
		{       
			ALOGE("failed to switch facing to %d, exit!\n", facing);	
			exit(0);       
		}		
		facing = !facing;	
	}

	return NULL;
}

int camdev_preview_frame(void *ctxt)
{
    CAMDEV *cam = (CAMDEV*)ctxt;
    int err;

    if (cam->update_flag) {
        cam->cur_win = cam->new_win;
        if (cam->cur_win != NULL) {
            native_window_api_connect(cam->cur_win.get(), NATIVE_WINDOW_API_CPU);
            native_window_set_buffers_dimensions(cam->cur_win.get(), cam->win_w, cam->win_h);
//            native_window_set_buffers_format(cam->cur_win.get(), HAL_PIXEL_FORMAT_YCBCR_422_I);
            native_window_set_buffers_format(cam->cur_win.get(), cam->gra_pix_fmt);

            native_window_set_usage(cam->cur_win.get(), GRALLOC_USAGE_SW_WRITE_OFTEN);
            native_window_set_scaling_mode(cam->cur_win.get(), NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW);
            native_window_set_buffer_count(cam->cur_win.get(), 3);
        }
        cam->update_flag = 0;
    }

    char *data = (char*)cam->vbs[cam->buf.index].addr;
    int   len  = cam->vbs[cam->buf.index].len;
    ANativeWindowBuffer *buf;
    size_t wsize;
 
    if (cam->cur_win != NULL
        && 0 == native_window_dequeue_buffer_and_wait(cam->cur_win.get(), &buf)) {
        sp<GraphicBuffer> test_buf(GraphicBuffer::from(buf));
        uint8_t* img = NULL;
        if (0 == test_buf->lock(GRALLOC_USAGE_SW_WRITE_OFTEN, (void**)(&img))) {
            memcpy(img, data, len);
            test_buf->unlock();

            if (cam->fp) {
                wsize = fwrite(data, len, 1, cam->fp);
                fflush(cam->fp);
                ALOGW("save YUV data %lu\n", wsize);
            }
        }

        if ((err = cam->cur_win->queueBuffer(cam->cur_win.get(), test_buf->getNativeBuffer(), -1)) != 0) {
            ALOGW("Surface::queueBuffer returned error %d\n", err);
        }
    }

    // requeue camera video buffer
    err = ioctl(cam->fd, VIDIOC_QBUF , &cam->buf);
    if (err < 0) {
        ALOGD("failed to en-queue buffer %s !\n", strerror(errno));
    }
	sem_post(&cam->sem_buf);

    return err;
}

static int v4l2_try_fmt_size(int fd, int fmt, int *width, int *height)
{
    struct v4l2_fmtdesc fmtdesc;
    struct v4l2_format  v4l2fmt;
    int                 find    =  0;
    int                 ret     =  0;

    fmtdesc.index = 0;
    fmtdesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

#if 1
    ALOGD("\n");
    ALOGD("VIDIOC_ENUM_FMT   \n");
    ALOGD("------------------\n");
#endif

    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) != -1) {
#if 1
        ALOGD("%d. flags: %d, description: %-16s, pixelfmt: 0x%0x\n",
            fmtdesc.index, fmtdesc.flags,
            fmtdesc.description,
            fmtdesc.pixelformat);
#endif
        if (fmt == (int)fmtdesc.pixelformat) {
            find = 1;
        }
        fmtdesc.index++;
    }
    ALOGD("\n");

    if (!find) {
        ALOGD("video device can't support pixel format: 0x%0x\n", fmt);
        return -1;
    }

    // try format
    v4l2fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    v4l2fmt.fmt.pix_mp.width       = *width;
    v4l2fmt.fmt.pix_mp.height      = *height;
    v4l2fmt.fmt.pix_mp.pixelformat = fmt;
    v4l2fmt.fmt.pix_mp.num_planes       = 1;
    ret = ioctl(fd, VIDIOC_TRY_FMT, &v4l2fmt);
    if (ret < 0)
    {
        ALOGE("VIDIOC_TRY_FMT Failed: %s\n", strerror(errno));
        return ret;
    }

    // driver surpport this size
    *width  = v4l2fmt.fmt.pix_mp.width;
    *height = v4l2fmt.fmt.pix_mp.height;

    ALOGD("using pixel format: 0x%0x\n", fmt);
    ALOGD("using frame size: %d, %d\n\n", *width, *height);
    return 0;
}

void* camdev_init(const char *dev, int sub, int w, int h, int frate, const char *output,
	int pix_fmt, int input_interval, int fix_input, int gra_pix_fmt)
{
    v4l2_std_id id;
    CAMDEV *cam = new CAMDEV();
    if (!cam) {
        return NULL;
    } else {
        memset(cam, 0, sizeof(CAMDEV));
    }

    // open camera device
    cam->fd = open(dev, O_RDWR | O_NONBLOCK);
    if (cam->fd < 0) {
        ALOGW("failed to open video device: %s\n", dev);
        delete cam;
        return NULL;
    }

	{
		int ret;
		struct v4l2_input input;

		ret = ioctl(cam->fd, VIDIOC_ENUMINPUT, &input);
				
		if (ret < 0 || input.status == V4L2_IN_ST_NO_SIGNAL) {
			ALOGE("%s->%d, no video signal detectd. exit!\n", __func__, __LINE__);
			close(cam->fd);
			delete cam;
			return NULL;
		}
		ALOGI("%s->%d, video signal detected!\n",__func__,__LINE__);
	}

	if (fix_input >= 0) {
		int ret;
		ret = ioctl(cam->fd, VIDIOC_S_INPUT, &fix_input);
		printf("set fix input to %d, ret:%d\n",fix_input, ret);
		if (ret) {
			ALOGE("select input %d fail!\n", fix_input);
			close(cam->fd);
			delete cam;
			exit(-1);
		}	
	}

    struct v4l2_capability cap;
    if (ioctl(cam->fd, VIDIOC_QUERYCAP, &cap) < 0) {
        ALOGE("VIDIOC_QUERYCAP Failed: %s\n", strerror(errno));
	close(cam->fd);
	delete cam;
        return NULL;
    }
 	cam->gra_pix_fmt = gra_pix_fmt;
    ALOGD("\n");
    ALOGD("video device caps \n");
    ALOGD("------------------\n");
    ALOGD("driver:       %s\n" , cap.driver      );
    ALOGD("card:         %s\n" , cap.card        );
    ALOGD("bus_info:     %s\n" , cap.bus_info    );
    ALOGD("version:      %0x\n", cap.version     );
    ALOGD("capabilities: %0x\n", cap.capabilities);
    ALOGD("\n");

    //if (ioctl(cam->fd, VIDIOC_G_STD, &id) < 0) {
   //     ALOGE("get std failed\n");
    //    return NULL;
    //}
 
    if (0 == v4l2_try_fmt_size(cam->fd, pix_fmt, &w, &h)) {
		ALOGD("===ck=== %s\n",get_pix_fmt_string(pix_fmt));
		printf("===ck=== %s\n",get_pix_fmt_string(pix_fmt));
        cam->cam_pixfmt = pix_fmt;
        cam->cam_w      = w;
        cam->cam_h      = h;
    }

    struct v4l2_format v4l2fmt;
    v4l2fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(cam->fd, VIDIOC_G_FMT, &v4l2fmt) < 0) {
        ALOGE("VIDIOC_G_FMT Failed: %s\n", strerror(errno));
	close(cam->fd);
	delete cam;
        return NULL;
    }
    v4l2fmt.fmt.pix_mp.pixelformat = cam->cam_pixfmt;
    v4l2fmt.fmt.pix_mp.width       = cam->cam_w & 0xFFFFFFF8;
    v4l2fmt.fmt.pix_mp.height      = cam->cam_h & 0xFFFFFFF8;
	v4l2fmt.fmt.pix_mp.num_planes = 1;
    if (ioctl(cam->fd, VIDIOC_S_FMT, &v4l2fmt) != -1) {
        ALOGD("VIDIOC_S_FMT      \n");
        ALOGD("------------------\n");
        ALOGD("width:        %d\n",    v4l2fmt.fmt.pix_mp.width       );
        ALOGD("height:       %d\n",    v4l2fmt.fmt.pix_mp.height      );
        ALOGD("pixfmt:       0x%0x\n", v4l2fmt.fmt.pix_mp.pixelformat );
        ALOGD("field:        %d\n",    v4l2fmt.fmt.pix_mp.field       );
        ALOGD("colorspace:   %d\n",    v4l2fmt.fmt.pix_mp.colorspace  );
    }
    else {
        ALOGW("failed to set camera preview size and pixel format !\n");
        close(cam->fd);
        delete cam;
        return NULL;
    }

    struct v4l2_streamparm streamparam;
    streamparam.parm.capture.timeperframe.numerator   = 1;
    streamparam.parm.capture.timeperframe.denominator = frate;
    streamparam.parm.capture.capturemode              = 0;
    streamparam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(cam->fd, VIDIOC_S_PARM, &streamparam) != -1) {
        ioctl(cam->fd, VIDIOC_G_PARM, &streamparam);
        ALOGD("current camera frame rate: %d/%d !\n",
            streamparam.parm.capture.timeperframe.denominator,
            streamparam.parm.capture.timeperframe.numerator );
        cam->cam_frate_num = streamparam.parm.capture.timeperframe.denominator;
        cam->cam_frate_den = streamparam.parm.capture.timeperframe.numerator;
    }
    else {
        ALOGW("failed to set camera frame rate !\n");
    }

    struct v4l2_requestbuffers req;
    req.count  = VIDEO_CAPTURE_BUFFER_COUNT;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(cam->fd, VIDIOC_REQBUFS, &req) < 0) {
        ALOGE("VIDIOC_REQBUFS Failed: %s\n", strerror(errno));
	close(cam->fd);
	delete cam;
        return NULL;
    }
 
	struct v4l2_plane planes;
    for (int i=0; i<VIDEO_CAPTURE_BUFFER_COUNT; i++) {
		memset(&planes, 0, sizeof(planes));
        cam->buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        cam->buf.memory = V4L2_MEMORY_MMAP;
        cam->buf.index  = i;
		cam->buf.m.planes = &planes;
		cam->buf.length = 1;	/* plane num */
        if (ioctl(cam->fd, VIDIOC_QUERYBUF, &cam->buf) < 0) {
            ALOGE("VIDIOC_QUERYBUF Failed: %s\n", strerror(errno));
            return NULL;
        }
        cam->vbs[i].addr= mmap(NULL, cam->buf.m.planes->length, PROT_READ | PROT_WRITE, MAP_SHARED,
                               cam->fd, cam->buf.m.planes->m.mem_offset);
        cam->vbs[i].len = cam->buf.m.planes->length;
		printf("data=%p, len :%d\n", cam->vbs[i].addr, cam->vbs[i].len);

        if (ioctl(cam->fd, VIDIOC_QBUF, &cam->buf) < 0) {
            ALOGE("VIDIOC_QBUF Failed: %s\n", strerror(errno));
	    close(cam->fd);
	    delete cam;
            return NULL;
        }
    }


    // set capture & preview pause flags
    cam->thread_state |= (CAMDEV_TS_PAUSE_C | CAMDEV_TS_PAUSE_P);

    // create sem_render
    sem_init(&cam->sem_render, 0, 0);
    sem_init(&cam->sem_buf, 0, 0);
	sem_post(&cam->sem_buf);
    // create capture thread
    //pthread_create(&cam->thread_id_capture, NULL, camdev_capture_thread_proc, cam);

    // create render thread
    //pthread_create(&cam->thread_id_render , NULL, camdev_render_thread_proc , cam);

	cam->thread_id_switch_camera = (pthread_t)-1;
	if (input_interval > 0) {
		cam->switch_facing_interval_sec = input_interval;
		pthread_create(&cam->thread_id_switch_camera , NULL, switch_camera_thread , cam);
	}

    cam->win_w = w;
    cam->win_h = h;

    if (strlen(output) > 0) {
        cam->fp = fopen(output, "wb");
        if (!cam->fp)
            ALOGW("Open file %s error %s\n", output, strerror(errno));
    }
    return cam;
}

void camdev_close(void *ctxt)
{
    CAMDEV *cam = (CAMDEV*)ctxt;
    if (!cam) return;

    // wait thread safely exited
    cam->thread_state |= CAMDEV_TS_EXIT; sem_post(&cam->sem_render);
    //pthread_join(cam->thread_id_capture, NULL);
    //pthread_join(cam->thread_id_render , NULL);

	if (cam->thread_id_switch_camera != (pthread_t)-1)
		pthread_join(cam->thread_id_switch_camera , NULL);

    // unmap buffers
    for (int i=0; i<VIDEO_CAPTURE_BUFFER_COUNT; i++) {
        munmap(cam->vbs[i].addr, cam->vbs[i].len);
    }

    // close & free
    close(cam->fd);
    delete cam;

}


void camdev_set_preview_window(void *ctxt, const sp<ANativeWindow> win)
{
    CAMDEV *cam = (CAMDEV*)ctxt;
    if (!cam) return;
    cam->new_win     = win;
    cam->update_flag = 1;
}

void camdev_capture_start(void *ctxt)
{
    CAMDEV *cam = (CAMDEV*)ctxt;
    // check fd valid
    if (!cam || cam->fd <= 0) return;

    // turn on stream
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(cam->fd, VIDIOC_STREAMON, &type) < 0) {
        ALOGE("VIDIOC_STREAMON Failed: %s\n", strerror(errno));
        return;
    }

    // resume thread
    cam->thread_state &= ~CAMDEV_TS_PAUSE_C;
}

void camdev_capture_stop(void *ctxt)
{
    CAMDEV *cam = (CAMDEV*)ctxt;
    // check fd valid
    if (!cam || cam->fd <= 0) return;

    // pause thread
    cam->thread_state |= CAMDEV_TS_PAUSE_C;

    // turn off stream
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(cam->fd, VIDIOC_STREAMOFF, &type) < 0)
        ALOGE("VIDIOC_STREAMON Failed: %s\n", strerror(errno));
}

void camdev_preview_start(void *ctxt)
{
    CAMDEV *cam = (CAMDEV*)ctxt;
    if (!cam) return;
    // set start prevew flag
    cam->thread_state &=~CAMDEV_TS_PAUSE_P;
    cam->thread_state |= CAMDEV_TS_PREVIEW;
}

void camdev_preview_stop(void *ctxt)
{
    CAMDEV *cam = (CAMDEV*)ctxt;
    if (!cam) return;
    // set stop prevew flag
    cam->thread_state |= CAMDEV_TS_PAUSE_P;
    cam->thread_state &=~CAMDEV_TS_PREVIEW;
}
