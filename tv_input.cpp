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
#include <tv/CTv.h>
#include <tvin/CTvin.h>
#include <tvserver/TvService.h>
/*****************************************************************************/

#define LOGD(...) \
{ \
__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__); }


typedef struct tv_input_private
{
	tv_input_device_t device;

	// Callback related data
	const tv_input_callback_ops_t *callback;
	void *callback_data;
	//TvService* pTvService;
	CTv *pTv;
} tv_input_private_t;

static int notify_ATV_device_available(tv_input_private_t *priv)
{
	tv_input_event_t event;
	event.device_info.device_id = SOURCE_TV;
	event.device_info.type = TV_INPUT_TYPE_TUNER;
	event.type = TV_INPUT_EVENT_DEVICE_AVAILABLE;
	event.device_info.audio_type = AUDIO_DEVICE_NONE;
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
	priv->callback->notify(&priv->device, &event, priv->callback_data);
	return 0;
}

static int notify_AV_device_available(tv_input_private_t *priv)
{
	tv_input_event_t event;
	event.device_info.device_id = SOURCE_AV1;
	event.device_info.type = TV_INPUT_TYPE_COMPONENT;
	event.type = TV_INPUT_EVENT_DEVICE_AVAILABLE;
	event.device_info.audio_type = AUDIO_DEVICE_NONE;
	priv->callback->notify(&priv->device, &event, priv->callback_data);
	return 0;
}

static int notify_AV_stream_configurations_change(tv_input_private_t *priv)
{
	tv_input_event_t event;
	event.device_info.device_id = SOURCE_AV1;
	event.device_info.type = TV_INPUT_TYPE_COMPONENT;
	event.type = TV_INPUT_EVENT_STREAM_CONFIGURATIONS_CHANGED;
	event.device_info.audio_type = AUDIO_DEVICE_NONE;
	priv->callback->notify(&priv->device, &event, priv->callback_data);
	return 0;
}

static int notify_HDMI_device_available(tv_input_private_t *priv, int dev_id, uint32_t port_id)
{
	tv_input_event_t event;
	event.device_info.device_id = dev_id;
	event.device_info.type = TV_INPUT_TYPE_HDMI;
	event.type = TV_INPUT_EVENT_DEVICE_AVAILABLE;
	event.device_info.hdmi.port_id = port_id;
	event.device_info.audio_type = AUDIO_DEVICE_NONE;
	priv->callback->notify(&priv->device, &event, priv->callback_data);
	return 0;
}

static int notify_HDMI_stream_configurations_change(tv_input_private_t *priv, int dev_id, uint32_t port_id)
{
	tv_input_event_t event;
	event.device_info.device_id = dev_id;
	event.device_info.type = TV_INPUT_TYPE_HDMI;
	event.type = TV_INPUT_EVENT_STREAM_CONFIGURATIONS_CHANGED;
	event.device_info.hdmi.port_id = port_id;
	event.device_info.audio_type = AUDIO_DEVICE_NONE;
	priv->callback->notify(&priv->device, &event, priv->callback_data);
	return 0;
}

static int get_stream_configs(int dev_id, int *num_configurations, const tv_stream_config_t **configs)
{
	tv_stream_config_t *mconfig = (tv_stream_config_t *)malloc(sizeof(mconfig));
	if (!mconfig)
	{
		return -1;
	}
	switch (dev_id)
	{
	case SOURCE_TV:
		mconfig->stream_id = SOURCE_TV;
		mconfig->type = TV_STREAM_TYPE_INDEPENDENT_VIDEO_SOURCE ;
		mconfig->max_video_width = 1920;
		mconfig->max_video_height = 1080;
		*num_configurations = 1;
		*configs = mconfig;
		break;
	case SOURCE_DTV:
		mconfig->stream_id = SOURCE_DTV;
		mconfig->type = TV_STREAM_TYPE_INDEPENDENT_VIDEO_SOURCE ;
		mconfig->max_video_width = 1920;
		mconfig->max_video_height = 1080;
		*num_configurations = 1;
		*configs = mconfig;
		break;
	case SOURCE_AV1:
		mconfig->stream_id = SOURCE_AV1;
		mconfig->type = TV_STREAM_TYPE_INDEPENDENT_VIDEO_SOURCE ;
		mconfig->max_video_width = 1920;
		mconfig->max_video_height = 1080;
		*num_configurations = 1;
		*configs = mconfig;
		break;
	case SOURCE_HDMI1:
		mconfig->stream_id = SOURCE_HDMI1;
		mconfig->type = TV_STREAM_TYPE_INDEPENDENT_VIDEO_SOURCE ;
		mconfig->max_video_width = 1920;
		mconfig->max_video_height = 1080;
		*num_configurations = 1;
		*configs = mconfig;
		break;
	case SOURCE_HDMI2:
		mconfig->stream_id = SOURCE_HDMI2;
		mconfig->type = TV_STREAM_TYPE_INDEPENDENT_VIDEO_SOURCE ;
		mconfig->max_video_width = 1920;
		mconfig->max_video_height = 1080;
		*num_configurations = 1;
		*configs = mconfig;
		break;
	case SOURCE_HDMI3:
		mconfig->stream_id = SOURCE_HDMI3;
		mconfig->type = TV_STREAM_TYPE_INDEPENDENT_VIDEO_SOURCE ;
		mconfig->max_video_width = 1920;
		mconfig->max_video_height = 1080;
		*num_configurations = 1;
		*configs = mconfig;
		break;
	default:
		break;
	}
	return 0;
}

static int get_tv_stream(int device_id, tv_stream_t *stream)
{
	native_handle *h = native_handle_create(0, 0);
	if (!h)
	{
		return -EINVAL;
	}
	switch (device_id)
	{
	case SOURCE_TV:
		stream->stream_id = SOURCE_TV;
		stream->type = TV_STREAM_TYPE_INDEPENDENT_VIDEO_SOURCE;
		stream->sideband_stream_source_handle = h;
		break;
	case SOURCE_DTV:
		stream->stream_id = SOURCE_DTV;
		stream->type = TV_STREAM_TYPE_INDEPENDENT_VIDEO_SOURCE;
		stream->sideband_stream_source_handle = h;
		break;
	case SOURCE_AV1:
		stream->stream_id = SOURCE_AV1;
		stream->type = TV_STREAM_TYPE_INDEPENDENT_VIDEO_SOURCE;
		stream->sideband_stream_source_handle = h;
		break;
	case SOURCE_HDMI1:
		stream->stream_id = SOURCE_HDMI1;
		stream->type = TV_STREAM_TYPE_INDEPENDENT_VIDEO_SOURCE;
		stream->sideband_stream_source_handle = h;
		break;
	case SOURCE_HDMI2:
		stream->stream_id = SOURCE_HDMI2;
		stream->type = TV_STREAM_TYPE_INDEPENDENT_VIDEO_SOURCE;
		stream->sideband_stream_source_handle = h;
		break;
	case SOURCE_HDMI3:
		stream->stream_id = SOURCE_HDMI3;
		stream->type = TV_STREAM_TYPE_INDEPENDENT_VIDEO_SOURCE;
		stream->sideband_stream_source_handle = h;
		break;
defualt:
		break;
	}
	return 0;
}

static int tv_input_device_open(const struct hw_module_t *module,
								const char *name, struct hw_device_t **device);

static struct hw_module_methods_t tv_input_module_methods =
{
open:
	tv_input_device_open
};

tv_input_module_t HAL_MODULE_INFO_SYM =
{
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
	}
};

/*****************************************************************************/

static int tv_input_initialize(struct tv_input_device *dev,
							   const tv_input_callback_ops_t *callback, void *data)
{
	if (dev == NULL || callback == NULL)
	{
		return -EINVAL;
	}
	tv_input_private_t *priv = (tv_input_private_t *)dev;
	if (priv->callback != NULL)
	{
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
	/*  AV_DEVICE_AVAILABLE */
	notify_AV_device_available(priv);
	notify_AV_stream_configurations_change(priv);
	/*  HDMI1_DEVICE_AVAILABLE */
	notify_HDMI_device_available(priv, SOURCE_HDMI1, 1);
	notify_HDMI_stream_configurations_change(priv, SOURCE_HDMI1, 1);
	/*  HDMI2_DEVICE_AVAILABLE */
	notify_HDMI_device_available(priv, SOURCE_HDMI2, 1);
	notify_HDMI_stream_configurations_change(priv, SOURCE_HDMI2, 1);
	/*  HDMI3_DEVICE_AVAILABLE */
	notify_HDMI_device_available(priv, SOURCE_HDMI3, 1);
	notify_HDMI_stream_configurations_change(priv, SOURCE_HDMI3, 1);

	return 0;
}

static int tv_input_get_stream_configurations(const struct tv_input_device *dev,
		int device_id, int *num_configurations,
		const tv_stream_config_t **configs)
{
	if (get_stream_configs(device_id, num_configurations, configs) == 0)
	{
		return 0;
	}
	return -EINVAL;
}

static int tv_input_open_stream(struct tv_input_device *dev, int device_id,
								tv_stream_t *stream)
{
	tv_input_private_t *priv = (tv_input_private_t *)dev;
	if (priv)
	{
		if (get_tv_stream(device_id, stream) != 0)
		{
			return -EINVAL;
		}
		LOGD ( "%s, SetSourceSwitchInput  id  = %d\n", __FUNCTION__,  device_id );
		priv->pTv->StartTvLock();
		priv->pTv->SetSourceSwitchInput((tv_source_input_t) device_id);
		return 0;
	}
	return -EINVAL;
}

static int tv_input_close_stream(struct tv_input_device *dev, int device_id,
								 int stream_id)
{
	tv_input_private_t *priv = (tv_input_private_t *)dev;
	if (priv)
	{
		LOGD ( "%s\n", __FUNCTION__ );
		priv->pTv->StopTvLock();
		return 0;
	}
	return -EINVAL;
}

static int tv_input_request_capture(
	struct tv_input_device *, int, int, buffer_handle_t, uint32_t)
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
	if (priv->pTv != NULL)
	{
		delete priv->pTv;
	}
	if (priv)
	{
		free(priv);
	}
	return 0;
}

/*****************************************************************************/

static int tv_input_device_open(const struct hw_module_t *module,
								const char *name, struct hw_device_t **device)
{
	int status = -EINVAL;
	if (!strcmp(name, TV_INPUT_DEFAULT_DEVICE))
	{
		tv_input_private_t *dev = (tv_input_private_t *)malloc(sizeof(*dev));

		/* initialize our state here */
		memset(dev, 0, sizeof(*dev));
		/*intialize tv*/
		dev->pTv = new CTv();
		TvService::instantiate(dev->pTv);
		dev->pTv->OpenTv();
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
