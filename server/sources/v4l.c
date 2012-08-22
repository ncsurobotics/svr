
/* v4l only supported on Linux */
#ifdef __SW_Linux__

/*
 * Adapted from the example:
 *     http://v4l2spec.bytesex.org/spec/capture-example.html
 */

#include "svr.h"
#include "svrd.h"

#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <highgui.h>

#include <asm/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>

static SVRD_Source* V4LSource_open(const char* name, Dictionary* arguments);
static void V4LSource_close(SVRD_Source* source);

SVRD_SourceType SVR_SOURCE(v4l) = {
        .name = "v4l",
        .open = V4LSource_open,
        .close = V4LSource_close
};

struct buffer
{
  void * start;
  size_t length;
};

typedef struct {
    int fd;
    struct buffer* buffers;
    unsigned int buffer_count;
    pthread_t thread;
    bool close;
} SVRD_V4LSource;

static bool V4LSource_get_frame(SVRD_Source* source, struct v4l2_buffer* buf);
static void V4LSource_enqueue(SVRD_Source* source_data, struct v4l2_buffer* buf);
static void* V4LSource_background(void* _source);
static void V4LSource_close_data(SVRD_V4LSource* source_data, const char* name, bool show_errors);

/* Order to try pixel formats */
unsigned long format_order[] = {
    V4L2_PIX_FMT_MJPEG,
};

static SVRD_Source* V4LSource_open(const char* name, Dictionary* arguments) {
    SVRD_V4LSource* source_data = calloc(sizeof(SVRD_V4LSource), 1);
    SVR_FrameProperties* frame_properties;
    SVRD_Source* source;
    unsigned int n;
    const char* dev;
    struct stat st;
    struct v4l2_capability cap;
    struct v4l2_requestbuffers buffer_request;
    struct v4l2_buffer buf;
    struct v4l2_format format;

    /* Open device */

    if(!Dictionary_exists(arguments, "dev")) {
        SVR_log(SVR_ERROR, Util_format("Error opening \"%s\": dev argument must be specified", name));
        return NULL;
    }

    dev = Dictionary_get(arguments, "dev");
    if(stat(dev, &st) == -1) {
        switch (errno) {
            case EACCES:
                SVR_log(SVR_ERROR, Util_format("Error opening \"%s\": Access denied for deviec: \"%s\"", name, dev));
            break;
            case ENAMETOOLONG:
                SVR_log(SVR_ERROR, Util_format("Error opening \"%s\": Device filename too long", name));
            break;
            case ENOENT:
            case ENOTDIR:
                SVR_log(SVR_ERROR, Util_format("Error opening \"%s\": Device not Found: \"%s\"", name, dev));
            break;
            default:
                SVR_log(SVR_ERROR, Util_format("Error opening \"%s\": stat call on \"%s\" failed (errno %d)", name, dev, errno));
        }
        return NULL;
    }

    if(!S_ISCHR(st.st_mode)) {
        SVR_log(SVR_ERROR, Util_format("Error opening \"%s\": \"%s\" is not a video device.  Try /dev/video[0-63] or /dev/video", name, dev));
        return NULL;
    }

    source_data->fd = open(dev, O_RDWR | O_NONBLOCK, 0);
    if(source_data->fd == -1) {
        SVR_log(SVR_ERROR, Util_format("Error opening \"%s\": Open call on \"%s\" failed (errno %d)", name, dev, errno));
        return NULL;
    }

    /* Check device capabilities */

    if(ioctl(source_data->fd, VIDIOC_QUERYCAP, &cap) == -1) {
        if(errno == EINVAL) {
            SVR_log(SVR_ERROR, Util_format("Error opening \"%s\": Not a V4L2 device: %s", name, dev));
        } else {
            SVR_log(SVR_ERROR, Util_format("Error opening \"%s\": VIDIOC_QUERYCAP failed (ioctl errno %d)", name, errno));
        }
        V4LSource_close_data(source_data, name, false);
        return NULL;
    }

    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        SVR_log(SVR_ERROR, Util_format("Error opening \"%s\": Not a V4L2 device: %s", name, dev));
        V4LSource_close_data(source_data, name, false);
        return NULL;
    }

    if(!(cap.capabilities & V4L2_CAP_STREAMING)) {
        SVR_log(SVR_ERROR, Util_format("Error opening \"%s\": Device does not support streaming", name));
        V4LSource_close_data(source_data, name, false);
        return NULL;
    }

    /* Find usable image format */
    for(n=0; n<sizeof(format_order); n++) {

        memset(&format, 0, sizeof(format));
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        format.fmt.pix.width = 640;
        format.fmt.pix.height = 480;
        format.fmt.pix.pixelformat = format_order[n];
        format.fmt.pix.field = V4L2_FIELD_ANY;

        if(ioctl(source_data->fd, VIDIOC_S_FMT, &format) == -1) {
            if(errno == EBUSY) {
                SVR_log(SVR_ERROR, Util_format("Error opening \"%s\": Device is busy: \"%s\"", name, dev));
                V4LSource_close_data(source_data, name, false);
                return NULL;
            }
        } else {
            /* Usable format found */
            break;
        }

    }
    if(n>=sizeof(format_order)) {
        SVR_log(SVR_ERROR, Util_format("Error opening \"%s\": Device does not support any implemented pixel formats", name));
        V4LSource_close_data(source_data, name, false);
        return NULL;
    }

    /* Setup memory map */

    memset(&buffer_request, 0, sizeof(buffer_request));
    buffer_request.count = 4;
    buffer_request.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer_request.memory = V4L2_MEMORY_MMAP;

    if(ioctl(source_data->fd, VIDIOC_REQBUFS, &buffer_request) == -1) {
        if(errno == EINVAL) {
            SVR_log(SVR_ERROR, Util_format("Error opening \"%s\": Device does not support memory mapping", name));
        } else {
            SVR_log(SVR_ERROR, Util_format("Error opening \"%s\": Buffer memory request failed (ioctl errno %d)", name, errno));
        }
        V4LSource_close_data(source_data, name, false);
        return NULL;
    }
    source_data->buffer_count = buffer_request.count;

    if(source_data->buffer_count < 2) {
        SVR_log(SVR_ERROR, Util_format("Error opening \"%s\": Device has insufficient buffer memory", name));
        V4LSource_close_data(source_data, name, false);
        return NULL;
    }

    source_data->buffers = calloc(source_data->buffer_count, sizeof(struct v4l2_buffer));
    if(source_data->buffers == NULL) {
        SVR_log(SVR_ERROR, Util_format("Error opening \"%s\": Not enough memory for buffers available on device", name));
        V4LSource_close_data(source_data, name, false);
        return NULL;
    }

    for(n=0; n<source_data->buffer_count; n++) {

        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n;

        if(ioctl(source_data->fd, VIDIOC_QUERYBUF, &buf) == -1) {
            SVR_log(SVR_ERROR, Util_format("Error opening \"%s\": Could not set up buffers (ioctl errno %d)", name, errno));
            V4LSource_close_data(source_data, name, false);
            return NULL;
        }

        source_data->buffers[n].length = buf.length;
        source_data->buffers[n].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, source_data->fd, buf.m.offset);

        if(source_data->buffers[n].start == MAP_FAILED) {
            SVR_log(SVR_ERROR, Util_format("Error opening \"%s\": Memory map failed", name));
            V4LSource_close_data(source_data, name, false);
            return NULL;
        }

    }

    /* Enqueue Buffers */
    for(n=0; n<source_data->buffer_count; n++) {

        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n;

        if(ioctl(source_data->fd, VIDIOC_QBUF, &buf) == -1) {
            SVR_log(SVR_ERROR, Util_format("Error opening \"%s\": Could not enqueue initial buffers (ioctl errno %d)", name, errno));
            V4LSource_close_data(source_data, name, false);
            return NULL;
        }

    }

    /* Stream On */
    if(ioctl(source_data->fd, VIDIOC_STREAMON, &buf.type) == -1) {
        SVR_log(SVR_ERROR, Util_format("Error opening \"%s\": Could not turn on stream (ioctl errno %d)", name, errno));
        V4LSource_close_data(source_data, name, false);
        return NULL;
    }

    /* Create source */

    frame_properties = SVR_FrameProperties_new();
    frame_properties->width = format.fmt.pix.width;
    frame_properties->height = format.fmt.pix.height;
    frame_properties->channels = 3;
    frame_properties->depth = 8;

    source = SVRD_Source_new(name);
    SVRD_Source_setEncoding(source, "raw");
    SVRD_Source_setFrameProperties(source, frame_properties);
    SVR_FrameProperties_destroy(frame_properties);

    if(source == NULL) {
        SVR_log(SVR_ERROR, Util_format("Error creating source '%s'", name));
        V4LSource_close_data(source_data, name, true);
        return NULL;
    }

    source->private_data = source_data;

    pthread_create(&source_data->thread, NULL, V4LSource_background, source);
    return source;
}

static void* V4LSource_background(void* _source) {
    SVRD_Source* source = (SVRD_Source*) _source;
    SVRD_V4LSource* source_data = (SVRD_V4LSource*) source->private_data;
    struct v4l2_buffer buf;
    bool ret;
    IplImage* frame;
    CvMat mat;

    while(source_data->close == false) {
        ret = V4LSource_get_frame(source, &buf);
        if(ret) {

            /* Convert image to bgr
             * For now this converts from mjpeg to bgr, but in the future more
             * formats may be implemented.
             */
            mat = cvMat(source->frame_properties->width, source->frame_properties->height, CV_8UC1, source_data->buffers[buf.index].start);
            frame = cvDecodeImage(&mat, 1);

            SVRD_Source_provideData(source, (void*) frame->imageData, frame->imageSize);
            V4LSource_enqueue(source, &buf);
            cvReleaseImage(&frame);

        } else {
            Util_usleep(1.0);
        }
    }

    return NULL;
}

static bool V4LSource_get_frame(SVRD_Source* source, struct v4l2_buffer* buf) {
    SVRD_V4LSource* source_data = (SVRD_V4LSource*) source->private_data;
    fd_set fds;
    struct timeval tv;
    int ret;

    /* Select Call */

    FD_ZERO(&fds);
    FD_SET(source_data->fd, &fds);

    tv.tv_sec = 2;
    tv.tv_usec = 0;

    ret = select(source_data->fd+1, &fds, NULL, NULL, &tv);
    if(ret == -1) {
        if(errno == EINTR) {
            /* Signal was caught */
            return false;
        }
        SVR_log(SVR_ERROR, Util_format("Select error on camera \"%s\"", source->name));
        return false;
    }
    if(ret == 0) {
        SVR_log(SVR_ERROR, Util_format("Select timeout on camera \"%s\"", source->name));
        return false;
    }

    /* Dequeue Buffer */

    memset(buf, 0, sizeof(struct v4l2_buffer));
    buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf->memory = V4L2_MEMORY_MMAP;

    if(ioctl(source_data->fd, VIDIOC_DQBUF, buf) == -1) {

        if(errno == EIO) {
            /* Ignore EIO */
        } else if(errno == EAGAIN) {
            /* Try again later */
            return false;
        } else {
            SVR_log(SVR_ERROR, Util_format("Error capturing \"%s\": Dequeing buffer failed (ioctl errno %d)", source->name, errno));
            return false;
        }

    }

    if(buf->index >= source_data->buffer_count) {
        SVR_log(SVR_ERROR, Util_format("Error capturing \"%s\": Dequeing buffer failed, invalid buffer index", source->name));
        V4LSource_enqueue(source, buf);
        return false;
    }

    return true;

}

static void V4LSource_enqueue(SVRD_Source* source, struct v4l2_buffer* buf) {
    SVRD_V4LSource* source_data = (SVRD_V4LSource*) source->private_data;
    if(ioctl(source_data->fd, VIDIOC_QBUF, buf) == -1) {
        SVR_log(SVR_ERROR, Util_format("Error capturing \"%s\": Enqueing buffer failed (ioctl errno %d)", source->name, errno));
    }
}

static void V4LSource_close(SVRD_Source* source) {
    SVRD_V4LSource* source_data = (SVRD_V4LSource*) source->private_data;

    source_data->close = true;
    pthread_join(source_data->thread, NULL);

    V4LSource_close_data(source_data, source->name, true);
}

/* Used to close SVRD_V4LSource before a SVRD_Source is created */
static void V4LSource_close_data(SVRD_V4LSource* source_data, const char* name, bool show_errors) {
    enum v4l2_buf_type type;
    unsigned int i;
    bool munmap_error = false;

    /* Stream Off */
    if(source_data->fd) {
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if(ioctl(source_data->fd, VIDIOC_STREAMOFF, &type) == -1 && show_errors) {
                SVR_log(SVR_ERROR, Util_format("Error closing \"%s\": Could not turn off stream (ioctl errno %d)", name, errno));
        }

    }

    /* Shut down memory map */
    if(source_data->buffers != NULL) {
        for(i=0; i<source_data->buffer_count; i++) {
            if(munmap(source_data->buffers[i].start, source_data->buffers[i].length) == -1 && !munmap_error && show_errors) {
                munmap_error = true;
                SVR_log(SVR_ERROR, Util_format("Error closing \"%s\": Could not unmap memory (munmap errno %d)", name, errno));
            }
        }
        free(source_data->buffers);
        source_data->buffers = NULL;
    }

    close(source_data->fd);
    free(source_data);

}

#else
/* Dummy code so the compiler doesn't complain about an empty file */
static void _v4l_dummy(void) {
    return;
}
#endif
