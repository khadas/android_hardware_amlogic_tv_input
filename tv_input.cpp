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
#include "tvapi/android/tv/TvPlay.h"
#include "tv_callback.h"
#include "tvapi/android/include/tvcmd.h"
#include <ui/GraphicBufferMapper.h>
#include <ui/GraphicBuffer.h>
/*****************************************************************************/

#define LOGD(...) \
{ \
__android_log_print(ANDROID_LOG_DEBUG, "tv_input", __VA_ARGS__); }

#ifndef container_of
#define container_of(ptr, type, member) ({                      \
    const typeof(((type *) 0)->member) *__mptr = (ptr);     \
    (type *) ((char *) __mptr - (char *)(&((type *)0)->member)); })
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
    TvPlay *mpTv;
    TvCallback *tvcallback;
} tv_input_private_t;


static int notify_ATV_device_available(tv_input_private_t *priv)
{
    tv_input_event_t event;
    event.device_info.device_id = SOURCE_TV;
    event.device_info.type = TV_INPUT_TYPE_TUNER;
    event.type = TV_INPUT_EVENT_DEVICE_AVAILABLE;
    event.device_info.audio_type = AUDIO_DEVICE_NONE;
    event.device_info.audio_address = NULL;
    priv->callback->notify(&priv->device, &event, priv->callback_data);
    return 0;
}

static int notify_ATV_stream_configurations_change(tv_input_private_t *priv)
{
    tv_input_event_t event;
    event.device_info.device_id = SOURCE_TV;
    event.device_info.type = TV_INPUT_TYPE_TUNER;
    event.type = TV_INPUT_EVENT_STREAM_CONFIGURATIONS_CHANGED;
    event.device_info.audio_type = AUDIO_DEVICE_NONE;
    event.device_info.audio_address = NULL;
    priv->callback->notify(&priv->device, &event, priv->callback_data);
    return 0;
}

static int notify_DTV_device_available(tv_input_private_t *priv)
{
    tv_input_event_t event;
    event.device_info.device_id = SOURCE_DTV;
    event.device_info.type = TV_INPUT_TYPE_TUNER;
    event.type = TV_INPUT_EVENT_DEVICE_AVAILABLE;
    event.device_info.audio_type = AUDIO_DEVICE_NONE;
    event.device_info.audio_address = NULL;
    priv->callback->notify(&priv->device, &event, priv->callback_data);
    return 0;
}

static int notify_DTV_stream_configurations_change(tv_input_private_t *priv)
{
    tv_input_event_t event;
    event.device_info.device_id = SOURCE_DTV;
    event.device_info.type = TV_INPUT_TYPE_TUNER;
    event.type = TV_INPUT_EVENT_STREAM_CONFIGURATIONS_CHANGED;
    event.device_info.audio_type = AUDIO_DEVICE_NONE;
    event.device_info.audio_address = NULL;
    priv->callback->notify(&priv->device, &event, priv->callback_data);
    return 0;
}

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

static int notify_AV_device_available(tv_input_private_t *priv, tv_source_input_t source_input, int type)
{
    tv_input_event_t event;
    event.device_info.device_id = source_input;
    event.device_info.type = TV_INPUT_TYPE_COMPONENT;
    event.type = type;
    event.device_info.audio_type = AUDIO_DEVICE_NONE;
    event.device_info.audio_address = NULL;
    priv->callback->notify(&priv->device, &event, priv->callback_data);
    return 0;
}

static int notify_AV_stream_configurations_change(tv_input_private_t *priv, tv_source_input_t source_input)
{
    tv_input_event_t event;
    event.device_info.device_id = source_input;
    event.device_info.type = TV_INPUT_TYPE_COMPONENT;
    event.type = TV_INPUT_EVENT_STREAM_CONFIGURATIONS_CHANGED;
    event.device_info.audio_type = AUDIO_DEVICE_NONE;
    event.device_info.audio_address = NULL;
    priv->callback->notify(&priv->device, &event, priv->callback_data);
    return 0;
}

static int notify_HDMI_device_available(tv_input_private_t *priv, tv_source_input_t source_input, uint32_t port_id, int type)
{
    tv_input_event_t event;
    event.device_info.device_id = source_input;
    event.device_info.type = TV_INPUT_TYPE_HDMI;
    event.type = type;
    event.device_info.hdmi.port_id = port_id;
    event.device_info.audio_type = AUDIO_DEVICE_NONE;
    event.device_info.audio_address = NULL;
    priv->callback->notify(&priv->device, &event, priv->callback_data);
    return 0;
}

static int notify_HDMI_stream_configurations_change(tv_input_private_t *priv, tv_source_input_t source_input, uint32_t port_id)
{
    tv_input_event_t event;
    event.device_info.device_id = source_input;
    event.device_info.type = TV_INPUT_TYPE_HDMI;
    event.type = TV_INPUT_EVENT_STREAM_CONFIGURATIONS_CHANGED;
    event.device_info.hdmi.port_id = port_id;
    event.device_info.audio_type = AUDIO_DEVICE_NONE;
    event.device_info.audio_address = NULL;
    priv->callback->notify(&priv->device, &event, priv->callback_data);
    return 0;
}

void TvCallback::onTvEvent (int32_t msgType, const Parcel &p)
{
    tv_input_private_t *priv = (tv_input_private_t *)(mPri);
    switch (msgType) {
    case SOURCE_CONNECT_CALLBACK: {
        int source = p.readInt32();
        int connectState = p.readInt32();
        LOGD("TvCallback::onTvEvent  source = %d, status = %d", source, connectState)

        switch (source) {
        case SOURCE_HDMI1:
        case SOURCE_HDMI2:
        case SOURCE_HDMI3: {
            if (connectState == 1) {
                notify_HDMI_device_available(priv, (tv_source_input_t)source, 1, TV_INPUT_EVENT_DEVICE_AVAILABLE);
                notify_HDMI_stream_configurations_change(priv, (tv_source_input_t)source, 1);
            } else {
                notify_HDMI_device_available(priv, (tv_source_input_t)source, 1, TV_INPUT_EVENT_DEVICE_UNAVAILABLE);
            }
        }
        break;

        case SOURCE_AV1:
        case SOURCE_AV2: {
            if (connectState == 1) {
                notify_AV_device_available(priv, (tv_source_input_t)source, TV_INPUT_EVENT_DEVICE_AVAILABLE);
                notify_AV_stream_configurations_change(priv, (tv_source_input_t)source);
            } else {
                notify_AV_device_available(priv, (tv_source_input_t)source, TV_INPUT_EVENT_DEVICE_UNAVAILABLE);
            }
        }
        break;

        default:
            break;
        }
    }
    break;

    default:
        break;
    }
}

#define NORMAL_STREAM_ID 1
#define FRAME_CAPTURE_STREAM_ID 2
static tv_stream_config_t mconfig[2];
static int get_stream_configs(int dev_id, int *num_configurations, const tv_stream_config_t **configs)
{
    switch (dev_id) {
    case SOURCE_TV:
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
        break;
    case SOURCE_DTV:
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
        break;
    case SOURCE_AV1:
    case SOURCE_AV2:
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
        break;
    case SOURCE_HDMI1:
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
        break;
    case SOURCE_HDMI2:
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
        break;
    case SOURCE_HDMI3:
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
        break;
    default:
        break;
    }
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
    } else if (stream->stream_id == NORMAL_STREAM_ID) {
        stream->type = TV_STREAM_TYPE_BUFFER_PRODUCER;
    }
    return 0;
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
    /*  ATV_DEVICE_AVAILABLE */
    notify_ATV_device_available(priv);
    notify_ATV_stream_configurations_change(priv);
    /*  DTV_DEVICE_AVAILABLE */
    notify_DTV_device_available(priv);
    notify_DTV_stream_configurations_change(priv);

    if (priv->mpTv->GetHdmiAvHotplugDetectOnoff()) {
        /*  AV1_DEVICE_AVAILABLE */
        int status = priv->mpTv->GetSourceConnectStatus(SOURCE_AV1);
        if (status == 1) { //IN
            notify_AV_device_available(priv, SOURCE_AV1, TV_INPUT_EVENT_DEVICE_AVAILABLE);
            notify_AV_stream_configurations_change(priv, SOURCE_AV1);
        }

        /*  AV2_DEVICE_AVAILABLE */
        status = priv->mpTv->GetSourceConnectStatus(SOURCE_AV2);
        if (status == 1) { //IN
            notify_AV_device_available(priv, SOURCE_AV2, TV_INPUT_EVENT_DEVICE_AVAILABLE);
            notify_AV_stream_configurations_change(priv, SOURCE_AV2);
        }

        /*  HDMI1_DEVICE_AVAILABLE */
        status = priv->mpTv->GetSourceConnectStatus(SOURCE_HDMI1);
        if (status == 1) { //IN
            notify_HDMI_device_available(priv, SOURCE_HDMI1, 1, TV_INPUT_EVENT_DEVICE_AVAILABLE);
            notify_HDMI_stream_configurations_change(priv, SOURCE_HDMI1, 0);
        }

        /*  HDMI2_DEVICE_AVAILABLE */
        status = priv->mpTv->GetSourceConnectStatus(SOURCE_HDMI2);
        if (status == 1) { //IN
            notify_HDMI_device_available(priv, SOURCE_HDMI2, 1, TV_INPUT_EVENT_DEVICE_AVAILABLE);
            notify_HDMI_stream_configurations_change(priv, SOURCE_HDMI2, 1);
        }

        /*  HDMI3_DEVICE_AVAILABLE */
        status = priv->mpTv->GetSourceConnectStatus(SOURCE_HDMI3);
        if (status == 1) { //IN
            notify_HDMI_device_available(priv, SOURCE_HDMI3, 2, TV_INPUT_EVENT_DEVICE_AVAILABLE);
            notify_HDMI_stream_configurations_change(priv, SOURCE_HDMI3, 2);
        }

        priv->mpTv->setTvObserver(priv->tvcallback);
    } else {
        /*  AV1_DEVICE_AVAILABLE */
        notify_AV_device_available(priv, SOURCE_AV1, TV_INPUT_EVENT_DEVICE_AVAILABLE);
        notify_AV_stream_configurations_change(priv, SOURCE_AV1);
        /*  AV2_DEVICE_AVAILABLE */
        notify_AV_device_available(priv, SOURCE_AV2, TV_INPUT_EVENT_DEVICE_AVAILABLE);
        notify_AV_stream_configurations_change(priv, SOURCE_AV2);
        /*  HDMI1_DEVICE_AVAILABLE */
        notify_HDMI_device_available(priv, SOURCE_HDMI1, 1, TV_INPUT_EVENT_DEVICE_AVAILABLE);
        notify_HDMI_stream_configurations_change(priv, SOURCE_HDMI1, 0);
        /*  HDMI2_DEVICE_AVAILABLE */
        notify_HDMI_device_available(priv, SOURCE_HDMI2, 1, TV_INPUT_EVENT_DEVICE_AVAILABLE);
        notify_HDMI_stream_configurations_change(priv, SOURCE_HDMI2, 1);
        /*  HDMI3_DEVICE_AVAILABLE */
        notify_HDMI_device_available(priv, SOURCE_HDMI3, 2, TV_INPUT_EVENT_DEVICE_AVAILABLE);
        notify_HDMI_stream_configurations_change(priv, SOURCE_HDMI3, 2);
    }
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
        return 0;
    }
    return -EINVAL;
}

static int tv_input_request_capture(
    struct tv_input_device *dev __unused, int device_id __unused,
    int stream_id __unused, buffer_handle_t buffer __unused, uint32_t seq __unused)
{
    return -EINVAL;
}

static int tv_input_cancel_capture(struct tv_input_device *, int, int, uint32_t)
{
    return -EINVAL;
}

/*****************************************************************************/

static int tv_input_device_close(struct hw_device_t *dev)
{
    tv_input_private_t *priv = (tv_input_private_t *)dev;
    if (priv) {
        if (priv->mpTv) {
            delete priv->mpTv;
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

        *device = &dev->device.common;
        status = 0;
    }
    return status;
}
