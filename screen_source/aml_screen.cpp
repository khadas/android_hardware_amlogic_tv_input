/*
 * Copyright (C) 2013 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "screen_source"
#include <hardware/hardware.h>
#include <hardware/aml_screen.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <linux/videodev2.h>
#include <sys/time.h>


#include <errno.h>
#include <cutils/log.h>
#include <cutils/atomic.h>

#include "v4l2_vdin.h"

#ifndef LOGD
#define LOGD ALOGD
#endif
#ifndef LOGV
#define LOGV ALOGV
#endif
#ifndef LOGE
#define LOGE ALOGE
#endif
#ifndef LOGI
#define LOGI ALOGI
#endif

static unsigned int gAmlScreenOpen = 0;
static android::Mutex gAmlScreenLock;

/*****************************************************************************/

static int aml_screen_device_open(const struct hw_module_t *module, const char *name,
								  struct hw_device_t **device);

static struct hw_module_methods_t aml_screen_module_methods = {
open:
	aml_screen_device_open
};

aml_screen_module_t HAL_MODULE_INFO_SYM = {
common:
	{
tag:
		HARDWARE_MODULE_TAG,
		version_major: 1,
		version_minor: 0,
id:
		AML_SCREEN_HARDWARE_MODULE_ID,
name: "aml screen source module"
		,
author: "Amlogic"
		,
methods:
		&aml_screen_module_methods,
dso :
		NULL,
reserved :
		{0},
	}
};

/*****************************************************************************/

static int aml_screen_device_close(struct hw_device_t *dev)
{
	android::vdin_screen_source *source = NULL;
	aml_screen_device_t *ctx = (aml_screen_device_t *)dev;

	android::Mutex::Autolock lock(gAmlScreenLock);
	if (ctx) {
		if (ctx->priv) {
			source = (android::vdin_screen_source *)ctx->priv;
			delete source;
			source = NULL;
		}
		free(ctx);
	}
	gAmlScreenOpen--;
	return 0;
}

int screen_source_start(struct aml_screen_device *dev)
{
	android::vdin_screen_source *source = (android::vdin_screen_source *)dev->priv;
	return source->start();
}

int screen_source_stop(struct aml_screen_device *dev)
{
	android::vdin_screen_source *source = (android::vdin_screen_source *)dev->priv;
	return source->stop();
}

int screen_source_pause(struct aml_screen_device *dev)
{
	android::vdin_screen_source *source = (android::vdin_screen_source *)dev->priv;
	return source->pause();
}

int screen_source_get_format(struct aml_screen_device *dev)
{
	android::vdin_screen_source *source = (android::vdin_screen_source *)dev->priv;
	return source->get_format();
}

int screen_source_set_format(struct aml_screen_device *dev, int width, int height, int pix_format)
{
	android::vdin_screen_source *source = (android::vdin_screen_source *)dev->priv;

	if ((width > 0) && (height > 0) && ((pix_format == V4L2_PIX_FMT_NV21) || (pix_format == V4L2_PIX_FMT_YUV420))) {
		return source->set_format(width, height, pix_format);
	} else {
		return source->set_format();
	}
}

int screen_source_set_rotation(struct aml_screen_device *dev, int degree)
{
	android::vdin_screen_source *source = (android::vdin_screen_source *)dev->priv;
	return source->set_rotation(degree);
}

int screen_source_set_crop(struct aml_screen_device *dev, int x, int y, int width, int height)
{
	android::vdin_screen_source *source = (android::vdin_screen_source *)dev->priv;

	if ((x >= 0) && (y >= 0) && (width > 0) && (height > 0))
		return source->set_crop(x, y, width, height);

	return android::BAD_VALUE;
}

int screen_source_set_amlvideo2_crop(struct aml_screen_device *dev, int x, int y, int width, int height)
{
	android::vdin_screen_source *source = (android::vdin_screen_source *)dev->priv;

	if ((x >= 0) && (y >= 0) && (width > 0) && (height > 0))
		return source->set_amlvideo2_crop(x, y, width, height);

	return android::BAD_VALUE;
}

int screen_source_aquire_buffer(struct aml_screen_device *dev, aml_screen_buffer_info_t *buff_info)
{
	android::vdin_screen_source *source = (android::vdin_screen_source *)dev->priv;

	return source->aquire_buffer(buff_info);
}

int screen_source_release_buffer(struct aml_screen_device *dev, long *ptr)
{
	android::vdin_screen_source *source = (android::vdin_screen_source *)dev->priv;
	return source->release_buffer(ptr);
}

int screen_source_set_state_callback(struct aml_screen_device *dev, olStateCB callback)
{
	android::vdin_screen_source *source = (android::vdin_screen_source *)dev->priv;
	return source->set_state_callback(callback);
}

int screen_source_set_preview_window(struct aml_screen_device *dev, ANativeWindow *window)
{
	android::vdin_screen_source *source = (android::vdin_screen_source *)dev->priv;
	return source->set_preview_window(window);
}

int screen_source_set_data_callback(struct aml_screen_device *dev, app_data_callback callback, void *user)
{
	android::vdin_screen_source *source = (android::vdin_screen_source *)dev->priv;
	return source->set_data_callback(callback, user);
}

int screen_source_set_frame_rate(struct aml_screen_device *dev, int frameRate)
{
	android::vdin_screen_source *source = (android::vdin_screen_source *)dev->priv;
	return source->set_frame_rate(frameRate);
}

int screen_source_set_source_type(struct aml_screen_device *dev, SOURCETYPE sourceType)
{
	android::vdin_screen_source *source = (android::vdin_screen_source *)dev->priv;
	return source->set_source_type(sourceType);
}

int screen_source_get_source_type(struct aml_screen_device *dev)
{
	android::vdin_screen_source *source = (android::vdin_screen_source *)dev->priv;
	return source->get_source_type();
}

/* int screen_source_inc_buffer_refcount(struct aml_screen_device* dev, int* ptr)
{
	android::vdin_screen_source* source = (android::vdin_screen_source*)dev->priv;
    return source->inc_buffer_refcount(ptr);
} */

/*****************************************************************************/

static int aml_screen_device_open(const struct hw_module_t *module, const char *name,
								  struct hw_device_t **device)
{
	int status = -EINVAL;
	android::vdin_screen_source *source = NULL;
	android::Mutex::Autolock lock(gAmlScreenLock);

	LOGV("aml_screen_device_open");

	if (!strcmp(name, AML_SCREEN_SOURCE)) {
		if (gAmlScreenOpen > 1) {
			ALOGD("aml screen device already open");
			*device = NULL;
			return -EINVAL;
		}

		aml_screen_device_t *dev = (aml_screen_device_t *)malloc(sizeof(aml_screen_device_t));

		if (!dev) {
			LOGE("no memory for the screen source device");
			return -ENOMEM;
		}
		/* initialize handle here */
		memset(dev, 0, sizeof(*dev));

		source = new android::vdin_screen_source;
		if (!source) {
			LOGE("no memory for class of vdin_screen_source");
			free (dev);
			return -ENOMEM;
		}

		if (source->init() != 0) {
			LOGE("open vdin_screen_source failed!");
			free (dev);
			return -1;
		}

		dev->priv = (void *)source;

		/* initialize the procs */
		dev->common.tag  = HARDWARE_DEVICE_TAG;
		dev->common.version = 0;
		dev->common.module = const_cast<hw_module_t *>(module);
		dev->common.close = aml_screen_device_close;

		dev->ops.start = screen_source_start;
		dev->ops.stop = screen_source_stop;
		dev->ops.pause = screen_source_pause;
		dev->ops.get_format = screen_source_get_format;
		dev->ops.set_format = screen_source_set_format;
		dev->ops.set_rotation = screen_source_set_rotation;
		dev->ops.set_crop = screen_source_set_crop;
		dev->ops.set_amlvideo2_crop = screen_source_set_amlvideo2_crop;
		dev->ops.aquire_buffer = screen_source_aquire_buffer;
		dev->ops.release_buffer = screen_source_release_buffer;
		dev->ops.setStateCallBack = screen_source_set_state_callback;
		dev->ops.setPreviewWindow = screen_source_set_preview_window;
		dev->ops.setDataCallBack = screen_source_set_data_callback;
		dev->ops.set_frame_rate = screen_source_set_frame_rate;
		dev->ops.set_source_type = screen_source_set_source_type;
		dev->ops.get_source_type = screen_source_get_source_type;
		// dev->ops.inc_buffer_refcount = screen_source_inc_buffer_refcount;
		*device = &dev->common;
		status = 0;
		gAmlScreenOpen++;
	}
	return status;
}
