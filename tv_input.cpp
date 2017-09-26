/*
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "TvInput"
#include <fcntl.h>
#include <errno.h>

#include <cutils/log.h>
#include <cutils/native_handle.h>

#include <hardware/tv_input.h>
#include "TvPlay.h"
#include "tv_callback.h"
#include <tvcmd.h>
#include <ui/GraphicBufferMapper.h>
#include <ui/GraphicBuffer.h>
#include <gralloc_priv.h>
#include <gralloc_helper.h>

#if PLATFORM_SDK_VERSION > 23
#include <gralloc_usage_ext.h>
#endif

#include <hardware/hardware.h>
#include <hardware/aml_screen.h>
#include <linux/videodev2.h>
#include <android/native_window.h>
/*****************************************************************************/

#define LOGD(...) \
{ \
__android_log_print(ANDROID_LOG_DEBUG, "tv_input", __VA_ARGS__); }

#ifndef container_of
#define container_of(ptr, type, member) \
    (type *)((char*)(ptr) - offsetof(type, member))
#endif

struct sideband_handle_t {
    native_handle_t nativeHandle;
    int identflag;
    int usage;
};

typedef struct tv_input_private {
    tv_input_device_t device;
    const tv_input_callback_ops_t *callback;
    void *callback_data;
    aml_screen_device_t *mDev;
    TvPlay *mpTv;
    TvCallback *tvcallback;
} tv_input_private_t;

#define SCREENSOURCE_GRALLOC_USAGE  ( GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_SW_READ_RARELY | GRALLOC_USAGE_SW_WRITE_NEVER)
//static int capBufferSize ;
static int capWidth;
static int capHeight;

void TvIputHal_ChannelConl(tv_input_private_t *priv, int ops_type, int device_id)
{
    if (priv->mpTv) {
        if (ops_type) {
            LOGD ( "%s, OpenSourceSwitchInput id = %d\n", __FUNCTION__, device_id );
            priv->mpTv->StartTv();
            priv->mpTv->SwitchSourceInput((tv_source_input_t) device_id);
        } else if (priv->mpTv->GetCurrentSourceInput() == device_id) {
            LOGD ( "%s, StopSourceSwitchInput id = %d\n", __FUNCTION__, device_id );
            priv->mpTv->StopTv();
        }
    }
}

static int notify_tv_device_status(tv_input_private_t *priv, tv_source_input_t source_input, int type)
{
    tv_input_event_t event;
    event.device_info.device_id = source_input;
    event.device_info.audio_type = AUDIO_DEVICE_NONE;
    event.device_info.audio_address = NULL;
    event.type = type;
    switch (source_input) {
        case SOURCE_TV:
        case SOURCE_DTV:
        case SOURCE_ADTV:
            event.device_info.type = TV_INPUT_TYPE_TUNER;
            break;
        case SOURCE_AV1:
        case SOURCE_AV2:
            event.device_info.type = TV_INPUT_TYPE_COMPOSITE;
            break;
        case SOURCE_HDMI1:
        case SOURCE_HDMI2:
        case SOURCE_HDMI3:
        case SOURCE_HDMI4:
            event.device_info.type = TV_INPUT_TYPE_HDMI;
            event.device_info.hdmi.port_id = priv->mpTv->getHdmiPort(source_input);
            break;
        case SOURCE_SPDIF:
            event.device_info.type = TV_INPUT_TYPE_OTHER_HARDWARE;
            break;
        default:
            break;
    }
    priv->callback->notify(&priv->device, &event, priv->callback_data);
    return 0;
}

static int notify_TV_Input_Capture_Succeeded(tv_input_private_t *priv, int device_id, int stream_id, uint32_t seq)
{
    tv_input_event_t event;
    event.type = TV_INPUT_EVENT_CAPTURE_SUCCEEDED;
    event.capture_result.device_id = device_id;
    event.capture_result.stream_id = stream_id;
    event.capture_result.seq = seq;
    priv->callback->notify(&priv->device, &event, priv->callback_data);
    return 0;
}

static int notify_TV_Input_Capture_Fail(tv_input_private_t *priv, int device_id, int stream_id, uint32_t seq)
{
    tv_input_event_t event;
    event.type = TV_INPUT_EVENT_CAPTURE_FAILED;
    event.capture_result.device_id = device_id;
    event.capture_result.stream_id = stream_id;
    event.capture_result.seq = seq;
    priv->callback->notify(&priv->device, &event, priv->callback_data);
    return 0;
}
void TvCallback::onTvEvent (int32_t msgType, const Parcel &p)
{
    tv_input_private_t *priv = (tv_input_private_t *)(mPri);
    switch (msgType) {
    case SOURCE_CONNECT_CALLBACK: {
        tv_source_input_t source = (tv_source_input_t)p.readInt32();
        int connectState = p.readInt32();
        LOGD("TvCallback::onTvEvent  source = %d, status = %d", source, connectState);

        if (source != SOURCE_HDMI1 && source != SOURCE_HDMI2 && source != SOURCE_HDMI3
            && source != SOURCE_HDMI4 && source != SOURCE_AV1 && source != SOURCE_AV2)
            break;

        if (connectState == 1) {
            notify_tv_device_status(priv, source, TV_INPUT_EVENT_DEVICE_AVAILABLE);
            notify_tv_device_status(priv, source, TV_INPUT_EVENT_STREAM_CONFIGURATIONS_CHANGED);
        } else {
            notify_tv_device_status(priv, source, TV_INPUT_EVENT_DEVICE_UNAVAILABLE);
        }
        break;
    }
    default:
        break;
    }
}

#define NORMAL_STREAM_ID 1
#define FRAME_CAPTURE_STREAM_ID 2
static tv_stream_config_t mconfig[2];
static int get_stream_configs(int dev_id __unused, int *num_configurations, const tv_stream_config_t **configs)
{
    mconfig[0].stream_id = NORMAL_STREAM_ID;
    mconfig[0].type = TV_STREAM_TYPE_INDEPENDENT_VIDEO_SOURCE ;
    mconfig[0].max_video_width = 1920;
    mconfig[0].max_video_height = 1080;
    mconfig[1].stream_id = FRAME_CAPTURE_STREAM_ID;
    mconfig[1].type = TV_STREAM_TYPE_BUFFER_PRODUCER ;
    mconfig[1].max_video_width = 1920;
    mconfig[1].max_video_height = 1080;
    *num_configurations = 2;
    *configs = mconfig;
    return 0;
}

static int get_tv_stream(tv_stream_t *stream)
{
    static struct sideband_handle_t *tvstream = NULL;
    if (stream->stream_id == NORMAL_STREAM_ID) {
        if ( !tvstream ) {
            tvstream = (struct sideband_handle_t *)native_handle_create(0, 2);
            if ( !tvstream ) {
                return -EINVAL;
            }
        }
        tvstream->identflag = 0xabcdcdef; //magic word
        tvstream->usage = GRALLOC_USAGE_AML_VIDEO_OVERLAY;
        stream->type = TV_STREAM_TYPE_INDEPENDENT_VIDEO_SOURCE;
        stream->sideband_stream_source_handle = (native_handle_t *)tvstream;
    } else if (stream->stream_id == FRAME_CAPTURE_STREAM_ID) {
        stream->type = TV_STREAM_TYPE_BUFFER_PRODUCER;
    }
    return 0;
}

static void available_all_tv_device(tv_input_private_t *priv)
{
    int tv_devices[15];
    int count = 0;
    priv->mpTv->getAllTvDevices(tv_devices, &count);

    if (count == 0) {
        ALOGE("tv.source.input.ids.default is not set.");
        return;
    }

    bool isHotplugDetectOn = priv->mpTv->GetHdmiAvHotplugDetectOnoff();

    if (isHotplugDetectOn)
        priv->mpTv->setTvObserver(priv->tvcallback);

    for (int i=0; i < count; i++) {
        tv_source_input_t source_input  = (tv_source_input_t)tv_devices[i];

        bool status = true;
        if (isHotplugDetectOn && SOURCE_AV1 <= source_input && source_input <= SOURCE_HDMI4) {
            status = priv->mpTv->GetSourceConnectStatus(source_input);
        }

        if (status) {
            notify_tv_device_status(priv, source_input, TV_INPUT_EVENT_DEVICE_AVAILABLE);
            notify_tv_device_status(priv, source_input, TV_INPUT_EVENT_STREAM_CONFIGURATIONS_CHANGED);
        }
    }
}

static int tv_input_device_open(const struct hw_module_t *module,
                                const char *name, struct hw_device_t **device);

static struct hw_module_methods_t tv_input_module_methods = {
open:
    tv_input_device_open
};

tv_input_module_t HAL_MODULE_INFO_SYM = {
common:
    {
tag:
        HARDWARE_MODULE_TAG,
        version_major: 0,
        version_minor: 1,
id:
        TV_INPUT_HARDWARE_MODULE_ID,
name: "TVInput module"
        ,
author: "Amlogic"
        ,
methods:
        &tv_input_module_methods,
dso:    NULL,
reserved: {0},
    }
};

/*****************************************************************************/
static int tv_input_initialize(struct tv_input_device *dev,
                               const tv_input_callback_ops_t *callback, void *data)
{
    if (dev == NULL || callback == NULL) {
        return -EINVAL;
    }
    tv_input_private_t *priv = (tv_input_private_t *)dev;
    if (priv->callback != NULL) {
        return -EEXIST;
    }
    priv->callback = callback;
    priv->callback_data = data;

    available_all_tv_device(priv);
    return 0;
}

static int tv_input_get_stream_configurations(const struct tv_input_device *dev __unused,
        int device_id, int *num_configurations,
        const tv_stream_config_t **configs)
{
    if (get_stream_configs(device_id, num_configurations, configs) == 0) {
        return 0;
    }
    return -EINVAL;
}

static int tv_input_open_stream(struct tv_input_device *dev, int device_id,
                                tv_stream_t *stream)
{
    tv_input_private_t *priv = (tv_input_private_t *)dev;
    if (priv) {
        if (get_tv_stream(stream) != 0) {
            return -EINVAL;
        }
        if (stream->stream_id == NORMAL_STREAM_ID) {
            TvIputHal_ChannelConl(priv, 1, device_id);
            return 0;
        } else if (stream->stream_id == FRAME_CAPTURE_STREAM_ID) {
            aml_screen_module_t* mModule;
            if (hw_get_module(AML_SCREEN_HARDWARE_MODULE_ID,
                (const hw_module_t **)&mModule) < 0) {
                ALOGE("can not get screen source module");
            } else {
                mModule->common.methods->open((const hw_module_t *)mModule,
                AML_SCREEN_SOURCE, (struct hw_device_t**)&(priv->mDev));
                //do test here, we can use ops of mDev to operate vdin source
            }

            if (priv->mDev) {
                if (capWidth == 0 || capHeight == 0) {
                    capWidth = stream->buffer_producer.width;
                    capHeight = stream->buffer_producer.height;
                }
                priv->mDev->ops.set_format(priv->mDev, capWidth, capHeight, V4L2_PIX_FMT_NV21);
                priv->mDev->ops.set_port_type(priv->mDev, (int)0x4000); //TVIN_PORT_HDMI0 = 0x4000
                priv->mDev->ops.start_v4l2_device(priv->mDev);
            }
            return 0;
        }
    }
    return -EINVAL;
}

static int tv_input_close_stream(struct tv_input_device *dev, int device_id,
                                 int stream_id)
{
    tv_input_private_t *priv = (tv_input_private_t *)dev;
    if (stream_id == NORMAL_STREAM_ID) {
        TvIputHal_ChannelConl(priv, 0, device_id);
        return 0;
    } else if (stream_id == FRAME_CAPTURE_STREAM_ID) {
        if (priv->mDev) {
            priv->mDev->ops.stop_v4l2_device(priv->mDev);
        }
        return 0;
    }
    return -EINVAL;
}

static int tv_input_request_capture(
    struct tv_input_device *dev __unused, int device_id __unused,
    int stream_id __unused, buffer_handle_t buffer __unused, uint32_t seq __unused)
{
    tv_input_private_t *priv = (tv_input_private_t *)dev;
    int index;
    aml_screen_buffer_info_t buff_info;
    int mFrameWidth , mFrameHeight ;
    int ret;
    long *src = NULL;
    unsigned char *dest = NULL;
    ANativeWindowBuffer *buf;
    if (priv->mDev) {
        ret = priv->mDev->ops.aquire_buffer(priv->mDev, &buff_info);
        if (ret != 0 || (buff_info.buffer_mem == 0)) {
            LOGD("Get V4l2 buffer failed");
            notify_TV_Input_Capture_Fail(priv,device_id,stream_id,--seq);
            return -EWOULDBLOCK;
        }
        src = (long *)buff_info.buffer_mem;
        buf = container_of(buffer, ANativeWindowBuffer, handle);
        sp<GraphicBuffer> graphicBuffer(new GraphicBuffer(buf, false));
        graphicBuffer->lock(SCREENSOURCE_GRALLOC_USAGE, (void **)&dest);
        if (dest == NULL) {
            LOGD("Invalid Gralloc Handle");
            return -EWOULDBLOCK;
        }
        memcpy(dest, src, capWidth*capHeight);
        graphicBuffer->unlock();
        graphicBuffer.clear();
        priv->mDev->ops.release_buffer(priv->mDev, src);

        notify_TV_Input_Capture_Succeeded(priv,device_id,stream_id,seq);
        return 0;
    }
    return -EWOULDBLOCK;
}

static int tv_input_cancel_capture(struct tv_input_device *, int, int, uint32_t)
{
    return -EINVAL;
}

static int tv_input_set_capturesurface_size(struct tv_input_device *dev __unused, int width, int height)
{
    if (width == 0 || height == 0) {
        return -EINVAL;
    } else {
        capWidth = width;
        capHeight = height;
        return 1;
    }
}
/*****************************************************************************/

static int tv_input_device_close(struct hw_device_t *dev)
{
    tv_input_private_t *priv = (tv_input_private_t *)dev;
    if (priv) {
        if (priv->mpTv) {
            delete priv->mpTv;
        }
        if (priv->mDev) {
            delete priv->mDev;
        }
        free(priv);
    }
    return 0;
}

/*****************************************************************************/

static int tv_input_device_open(const struct hw_module_t *module,
                                const char *name, struct hw_device_t **device)
{
    int status = -EINVAL;
    if (!strcmp(name, TV_INPUT_DEFAULT_DEVICE)) {
        tv_input_private_t *dev = (tv_input_private_t *)malloc(sizeof(*dev));
        /* initialize our state here */
        memset(dev, 0, sizeof(*dev));
        dev->mpTv = new TvPlay();
        dev->tvcallback = new TvCallback(dev);
        /* initialize the procs */
        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version = TV_INPUT_DEVICE_API_VERSION_0_1;
        dev->device.common.module = const_cast<hw_module_t *>(module);
        dev->device.common.close = tv_input_device_close;

        dev->device.initialize = tv_input_initialize;
        dev->device.get_stream_configurations =
            tv_input_get_stream_configurations;
        dev->device.open_stream = tv_input_open_stream;
        dev->device.close_stream = tv_input_close_stream;
        dev->device.request_capture = tv_input_request_capture;
        dev->device.cancel_capture = tv_input_cancel_capture;
        dev->device.set_capturesurface_size = tv_input_set_capturesurface_size;

        *device = &dev->device.common;
        status = 0;
    }
    return status;
}
