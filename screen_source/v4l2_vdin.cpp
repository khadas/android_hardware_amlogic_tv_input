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


//reinclude because of a bug with the log macros
//#define LOG_NDEBUG 0
#define LOG_TAG "V4L2VINSOURCE"
#include <utils/Log.h>
#include <utils/String8.h>

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

#include <cutils/properties.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "v4l2_vdin.h"
#include <ui/GraphicBufferMapper.h>
#include <ui/GraphicBuffer.h>
#include <linux/videodev2.h>

namespace android {

#define V4L2_ROTATE_ID 0x980922

#ifndef container_of
#define container_of(ptr, type, member) ({                      \
		const typeof(((type *) 0)->member) *__mptr = (ptr);     \
		(type *) ((char *) __mptr - (char *)(&((type *)0)->member)); })
#endif

#define BOUNDRY 32

#define ALIGN(x) (x + (BOUNDRY) - 1)& ~((BOUNDRY) - 1)

	static size_t getBufSize(int format, int width, int height)
	{
		size_t buf_size = 0;

		switch (format) {
		case V4L2_PIX_FMT_YVU420:
		case V4L2_PIX_FMT_NV21:
			buf_size = width * height * 3 / 2;
			break;
		case V4L2_PIX_FMT_YUYV:
		case V4L2_PIX_FMT_RGB565:
			buf_size = width * height * 2;
			break;
		case V4L2_PIX_FMT_RGB24:
			buf_size = width * height * 3;
			break;
		case V4L2_PIX_FMT_RGB32:
			buf_size = width * height * 4;
			break;
		default:
			ALOGE("Invalid format");
			buf_size = 0;
		}
		return buf_size;
	}

	static int  getNativeWindowFormat(int format)
	{
		int nativeFormat = HAL_PIXEL_FORMAT_YCbCr_422_I;

		switch (format) {
		case V4L2_PIX_FMT_YVU420:
			nativeFormat = HAL_PIXEL_FORMAT_YV12;
			break;
		case V4L2_PIX_FMT_NV21:
			nativeFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
			break;
		case V4L2_PIX_FMT_YUYV:
			nativeFormat = HAL_PIXEL_FORMAT_YCbCr_422_I;
			break;
		case V4L2_PIX_FMT_RGB565:
			nativeFormat = HAL_PIXEL_FORMAT_RGB_565;
			break;
		case V4L2_PIX_FMT_RGB24:
			nativeFormat = HAL_PIXEL_FORMAT_RGB_888;
			break;
		case V4L2_PIX_FMT_RGB32:
			nativeFormat = HAL_PIXEL_FORMAT_RGBA_8888;
			break;
		default:
			ALOGE("Invalid format,Use default format");
		}
		return nativeFormat;
	}


	static ANativeWindowBuffer *handle_to_buffer(buffer_handle_t *handle)
	{
		return container_of(handle, ANativeWindowBuffer, handle);
	}

	vdin_screen_source::vdin_screen_source()
		: mCameraHandle(-1),
		  mVideoInfo(NULL)
	{
		ALOGV("%s %d", __FUNCTION__, __LINE__);
	}

	int vdin_screen_source::init()
	{
		ALOGV("%s %d", __FUNCTION__, __LINE__);
		mCameraHandle = open("/dev/video11", O_RDWR | O_NONBLOCK);
		if (mCameraHandle < 0) {
			ALOGE("[%s %d] mCameraHandle:%x [%s]", __FUNCTION__, __LINE__, mCameraHandle, strerror(errno));
			return -1;
		}
		mVideoInfo = (struct VideoInfo *) calloc (1, sizeof (struct VideoInfo));
		if (mVideoInfo == NULL) {
			ALOGE("[%s %d] no memory for mVideoInfo", __FUNCTION__, __LINE__);
			close(mCameraHandle);
			return NO_MEMORY;
		}
		mBufferCount = 4;
		mPixelFormat = V4L2_PIX_FMT_NV21;
		mNativeWindowPixelFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
		mFrameWidth = 1280;
		mFrameHeight = 720;
		mBufferSize = mFrameWidth * mFrameHeight * 3 / 2;
		mSetStateCB = NULL;
		mState = STOP;
		mANativeWindow = NULL;
		mFrameType = 0;
		mWorkThread = NULL;
		mDataCB = NULL;
		mOpen = false;
		return NO_ERROR;
	}

	vdin_screen_source::~vdin_screen_source()
	{
		if (mVideoInfo) {
			free (mVideoInfo);
		}
		if (mCameraHandle >= 0) {
			close(mCameraHandle);
		}
	}

	int vdin_screen_source::start_v4l2_device()
	{
		int ret = -1;

		ALOGV("[%s %d] mCameraHandle:%x", __FUNCTION__, __LINE__, mCameraHandle);

		ioctl(mCameraHandle, VIDIOC_QUERYCAP, &mVideoInfo->cap);

		mVideoInfo->rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		mVideoInfo->rb.memory = V4L2_MEMORY_MMAP;
		mVideoInfo->rb.count = mBufferCount;

		ret = ioctl(mCameraHandle, VIDIOC_REQBUFS, &mVideoInfo->rb);

		if (ret < 0) {
			ALOGE("[%s %d] VIDIOC_REQBUFS:%d mCameraHandle:%x", __FUNCTION__, __LINE__, ret, mCameraHandle);
			return ret;
		}

		for (int i = 0; i < mBufferCount; i++) {
			memset (&mVideoInfo->buf, 0, sizeof (struct v4l2_buffer));

			mVideoInfo->buf.index = i;
			mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

			ret = ioctl (mCameraHandle, VIDIOC_QUERYBUF, &mVideoInfo->buf);
			if (ret < 0) {
				ALOGE("[%s %d]VIDIOC_QUERYBUF %d failed", __FUNCTION__, __LINE__, i);
				return ret;
			}
			mVideoInfo->canvas[i] = mVideoInfo->buf.reserved;
			mVideoInfo->mem[i] = (long *)mmap (0, mVideoInfo->buf.length, PROT_READ | PROT_WRITE,
											   MAP_SHARED, mCameraHandle, mVideoInfo->buf.m.offset);

			if (mVideoInfo->mem[i] == MAP_FAILED) {
				ALOGE("[%s %d] MAP_FAILED", __FUNCTION__, __LINE__);
				return -1;
			}
			mVideoInfo->refcount[i] = 0;
			mBufs.add(mVideoInfo->mem[i], i);
		}
		ALOGV("[%s %d] VIDIOC_QUERYBUF successful", __FUNCTION__, __LINE__);

		for (int i = 0; i < mBufferCount; i++) {
			mVideoInfo->buf.index = i;
			mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;
			ret = ioctl(mCameraHandle, VIDIOC_QBUF, &mVideoInfo->buf);
			if (ret < 0) {
				ALOGE("VIDIOC_QBUF Failed");
				return -1;
			}
		}
		enum v4l2_buf_type bufType;
		bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		ret = ioctl (mCameraHandle, VIDIOC_STREAMON, &bufType);

		ALOGV("[%s %d] VIDIOC_STREAMON:%x", __FUNCTION__, __LINE__, ret);
		return ret;
	}

	int vdin_screen_source::stop_v4l2_device()
	{
		int ret;
		enum v4l2_buf_type bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		ret = ioctl (mCameraHandle, VIDIOC_STREAMOFF, &bufType);
		if (ret < 0) {
			ALOGE("StopStreaming: Unable to stop capture: %s", strerror(errno));
		}
		for (int i = 0; i < mBufferCount; i++) {
			if (munmap(mVideoInfo->mem[i], mVideoInfo->buf.length) < 0) {
				ALOGE("Unmap failed");
			}
		}
		return ret;
	}

	int vdin_screen_source::start()
	{
		ALOGV("%s %d", __FUNCTION__, __LINE__);
		int ret;
		if (mOpen == true) {
			ALOGI("already open");
			return NO_ERROR;
		}

		ret = start_v4l2_device();
		if (ret != NO_ERROR) {
			ALOGE("Start v4l2 device failed:%d", ret);
			return ret;
		}
		if (mFrameType & NATIVE_WINDOW_DATA) {
			ret = init_native_window();
			if (ret != NO_ERROR) {
				ALOGE("Init Native Window Failed:%d", ret);
				return ret;
			}
		}
		if (mFrameType & NATIVE_WINDOW_DATA || mFrameType & CALL_BACK_DATA) {
			ALOGD("Create Work Thread");
			mWorkThread = new WorkThread(this);
		}
		if (mSetStateCB != NULL) {
			mSetStateCB(START);
		}
		mState = START;
		mOpen = true;
		ALOGV("%s %d ret:%d", __FUNCTION__, __LINE__, ret);
		return NO_ERROR;
	}

	int vdin_screen_source::pause()
	{
		ALOGV("%s %d", __FUNCTION__, __LINE__);
		mState = PAUSE;
		if (mSetStateCB != NULL) {
			mSetStateCB(PAUSE);
		}
		return NO_ERROR;
	}
	int vdin_screen_source::stop()
	{
		ALOGV("%s %d", __FUNCTION__, __LINE__);
		int ret;
		mState = STOPING;

		if (mWorkThread != NULL) {
			mWorkThread->requestExitAndWait();
			mWorkThread.clear();
		}

		mBufferCount = 0;
		mState = STOP;
		if (mSetStateCB != NULL) {
			mSetStateCB(STOP);
		}
		mOpen = false;
		return ret;
	}

	int vdin_screen_source::set_state_callback(olStateCB callback)
	{
		if (!callback) {
			ALOGE("NULL state callback pointer");
			return BAD_VALUE;
		}
		mSetStateCB = callback;
		return NO_ERROR;
	}

	int vdin_screen_source::set_preview_window(ANativeWindow *window)
	{
		ALOGV("%s %d", __FUNCTION__, __LINE__);
		if (mOpen == true) {
			return NO_ERROR;
		}
		//can work without a valid window object ?
		if (window == NULL) {
			ALOGD("NULL window object passed to ScreenSource");
			if (mWorkThread != NULL) {
				mWorkThread->requestExitAndWait();
				mWorkThread.clear();
			}
			mFrameType &= ~NATIVE_WINDOW_DATA;
			return NO_ERROR;
		}
		mFrameType |= NATIVE_WINDOW_DATA;
		mANativeWindow = window;
		return NO_ERROR;
	}

	int vdin_screen_source::set_data_callback(app_data_callback callback, void *user)
	{
		ALOGV("%s %d", __FUNCTION__, __LINE__);
		if (callback == NULL) {
			ALOGE("NULL data callback pointer");
			return BAD_VALUE;
		}
		mDataCB = callback;
		mUser = user;
		mFrameType |= CALL_BACK_DATA;
		return NO_ERROR;
	}

	int vdin_screen_source::get_format()
	{
		return mPixelFormat;
	}

	int vdin_screen_source::set_format(int width, int height, int color_format)
	{
		ALOGV("[%s %d]", __FUNCTION__, __LINE__);
		if (mOpen == true) {
			return NO_ERROR;
		}
		int ret;
		mVideoInfo->width = ALIGN(width);
		mVideoInfo->height = height;
		mVideoInfo->framesizeIn = (mVideoInfo->width * mVideoInfo->height << 3);      //note color format
		mVideoInfo->formatIn = color_format;

		mVideoInfo->format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		mVideoInfo->format.fmt.pix.width = ALIGN(width);
		mVideoInfo->format.fmt.pix.height = height;
		mVideoInfo->format.fmt.pix.pixelformat = color_format;
		mPixelFormat = color_format;
		mNativeWindowPixelFormat = getNativeWindowFormat(color_format);
		mFrameWidth = ALIGN(width);
		mFrameHeight = height;
		mBufferSize = getBufSize(color_format, mFrameWidth, mFrameHeight);
		ALOGD("mFrameWidth:%d,mFrameHeight:%d", mFrameWidth, mFrameHeight);
		ALOGD("mPixelFormat:%x,mNativeWindowPixelFormat:%x,mBufferSize:%d", mPixelFormat, mNativeWindowPixelFormat, mBufferSize);
		ret = ioctl(mCameraHandle, VIDIOC_S_FMT, &mVideoInfo->format);
		if (ret < 0) {
			ALOGE("[%s %d]VIDIOC_S_FMT %d", __FUNCTION__, __LINE__, ret);
			return ret;
		}
		return ret;
	}

	int vdin_screen_source::set_rotation(int degree)
	{
		ALOGV("[%s %d]", __FUNCTION__, __LINE__);

		int ret = 0;
		struct v4l2_control ctl;

		if (mCameraHandle < 0) {
			return -1;
		}

		if ((degree != 0) && (degree != 90) && (degree != 180) && (degree != 270)) {
			ALOGE("Set rotate value invalid: %d.", degree);
			return -1;
		}

		memset( &ctl, 0, sizeof(ctl));
		ctl.value = degree;
		ctl.id = V4L2_ROTATE_ID;
		ret = ioctl(mCameraHandle, VIDIOC_S_CTRL, &ctl);

		if (ret < 0) {
			ALOGE("Set rotate value fail: %s. ret=%d", strerror(errno), ret);
		}
		return ret ;
	}

	int vdin_screen_source::set_crop(int x, int y, int width, int height)
	{
		ALOGV("[%s %d]", __FUNCTION__, __LINE__);
		if (NULL == mANativeWindow.get())
			return BAD_VALUE;

		int err = NO_ERROR;
		android_native_rect_t crop = { x, y, x + width - 1, y + height - 1 };
		err = native_window_set_crop(mANativeWindow.get(), &crop);
		if (err != 0) {
			ALOGW("Failed to set crop!");
			return err;
		}
		return NO_ERROR;
	}

	int vdin_screen_source::set_frame_rate(int frameRate)
	{
		ALOGV("[%s %d]", __FUNCTION__, __LINE__);
		int ret = 0;
		struct v4l2_control ctl;

		if (mCameraHandle < 0) {
			return -1;
		}

		struct v4l2_streamparm sparm;
		memset(&sparm, 0, sizeof( sparm ));
		sparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//stream_flag;
		sparm.parm.output.timeperframe.denominator = frameRate;
		sparm.parm.output.timeperframe.numerator = 1;

		ret = ioctl(mCameraHandle, VIDIOC_S_PARM, &sparm);
		if (ret < 0) {
			ALOGE("Set frame rate fail: %s. ret=%d", strerror(errno), ret);
		}
		return ret ;
	}

	int vdin_screen_source::set_amlvideo2_crop(int x, int y, int width, int height)
	{
		ALOGV("[%s %d]", __FUNCTION__, __LINE__);
		int ret = 0;

		struct v4l2_crop crop;
		memset(&crop, 0, sizeof(struct v4l2_crop));

		crop.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
		crop.c.left = x;
		crop.c.top = y;
		crop.c.width = width;
		crop.c.height = height;
		ret = ioctl(mCameraHandle, VIDIOC_S_CROP, &crop);
		if (ret) {
			ALOGE("Set frame rate fail: %s. ret=%d", strerror(errno), ret);
		}

		return ret ;
	}

	int vdin_screen_source::set_source_type(int sourceType)
	{
		ALOGV("[%s %d]", __FUNCTION__, __LINE__);
		int ret = 0;

		ret = ioctl(mCameraHandle, VIDIOC_S_INPUT, &sourceType);
		if (ret < 0) {
			ALOGE("Set source type fail: %s. ret:%d", strerror(errno), ret);
		}
		return ret;
	}

	int vdin_screen_source::get_source_type()
	{
		ALOGV("[%s %d]", __FUNCTION__, __LINE__);
		int ret = -1;
		int sourceType;

		ret = ioctl(mCameraHandle, VIDIOC_G_INPUT, &sourceType);
		if (ret < 0) {
			ALOGE("Set source type fail: %s. ret:%d", strerror(errno), ret);
			return ret;
		}
		return sourceType;
	}

	int vdin_screen_source::aquire_buffer(aml_screen_buffer_info_t *buff_info)
	{
		ALOGV("%s %d", __FUNCTION__, __LINE__);
		int ret = -1;
		mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

		ret = ioctl(mCameraHandle, VIDIOC_DQBUF, &mVideoInfo->buf);
		if (ret < 0) {
			if (EAGAIN == errno) {
				ret = -EAGAIN;
			} else {
				ALOGE("[%s %d]aquire_buffer %d", __FUNCTION__, __LINE__, ret);
			}
			buff_info->buffer_mem    = 0;
			buff_info->buffer_canvas = 0;
			return ret;
		}
		buff_info->buffer_mem    = mVideoInfo->mem[mVideoInfo->buf.index];
		buff_info->buffer_canvas = mVideoInfo->canvas[mVideoInfo->buf.index];
		buff_info->tv_sec        = mVideoInfo->buf.timestamp.tv_sec;
		buff_info->tv_usec       = mVideoInfo->buf.timestamp.tv_usec;
		return ret;
	}

	/* int vdin_screen_source::inc_buffer_refcount(int *ptr){
		ALOGV("%s %d", __FUNCTION__, __LINE__);
		int ret = -1;
		int index;
		index = mBufs.valueFor((unsigned int)ptr);
		mVideoInfo->refcount[index] += 1;
		return true;
	} */

	int vdin_screen_source::release_buffer(long *ptr)
	{
		ALOGV("%s %d", __FUNCTION__, __LINE__);
		int ret = -1;
		int currentIndex;
		v4l2_buffer hbuf_query;

		Mutex::Autolock autoLock(mLock);

		currentIndex = mBufs.valueFor(ptr);
		if (mVideoInfo->refcount[currentIndex] > 0) {
			mVideoInfo->refcount[currentIndex] -= 1;
		} else {
			ALOGE("return buffer when refcount already zero");
			return 0;
		}
		if (mVideoInfo->refcount[currentIndex] == 0) {
			memset(&hbuf_query, 0, sizeof(v4l2_buffer));
			hbuf_query.index = currentIndex;
			hbuf_query.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			hbuf_query.memory = V4L2_MEMORY_MMAP;
			ALOGV("return buffer :%d", currentIndex);
			ret = ioctl(mCameraHandle, VIDIOC_QBUF, &hbuf_query);
			if (ret != 0) {
				ALOGE("Return Buffer :%d failed", currentIndex);
			}
		}
		return 0;
	}

	int vdin_screen_source::init_native_window()
	{
		ALOGV("%s %d", __FUNCTION__, __LINE__);
		int err = NO_ERROR;

		if (NULL == mANativeWindow.get()) {
			return BAD_VALUE;
		}

		// Set gralloc usage bits for window.
		err = native_window_set_usage(mANativeWindow.get(), SCREENSOURCE_GRALLOC_USAGE);
		if (err != 0) {
			ALOGE("native_window_set_usage failed: %s\n", strerror(-err));
			if (ENODEV == err ) {
				ALOGE("Preview surface abandoned!");
				mANativeWindow = NULL;
			}
			return err;
		}

		ALOGD("Number of buffers set to ANativeWindow %d", mBufferCount);
		///Set the number of buffers needed for camera preview
		err = native_window_set_buffer_count(mANativeWindow.get(), mBufferCount);
		if (err != 0) {
			ALOGE("native_window_set_buffer_count failed: %s (%d)", strerror(-err), -err);
			if (ENODEV == err) {
				ALOGE("Preview surface abandoned!");
				mANativeWindow = NULL;
			}
			return err;
		}

		ALOGD("native_window_set_buffers_geometry format:0x%x", mNativeWindowPixelFormat);
		// Set window geometry
		err = native_window_set_buffers_geometry(
				  mANativeWindow.get(),
				  mFrameWidth,
				  mFrameHeight,
				  mNativeWindowPixelFormat);

		if (err != 0) {
			ALOGE("native_window_set_buffers_geometry failed: %s", strerror(-err));
			if ( ENODEV == err ) {
				ALOGE("Surface abandoned!");
				mANativeWindow = NULL;
			}
			return err;
		}
		err = native_window_set_scaling_mode(mANativeWindow.get(), NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW);
		if (err != 0) {
			ALOGW("Failed to set scaling mode: %d", err);
			return err;
		}
		return NO_ERROR;
	}

	int vdin_screen_source::workThread()
	{
		bool buff_keep = false;
		int index;
		aml_screen_buffer_info_t buff_info;
		int ret;
		long *src = NULL;
		unsigned char *dest = NULL;
		uint8_t *handle = NULL;
		ANativeWindowBuffer *buf;
		if (mState == START) {
			usleep(5000);
			ret = aquire_buffer(&buff_info);
			if (ret != 0 || (buff_info.buffer_mem == 0)) {
				ALOGV("Get V4l2 buffer failed");
				return ret;
			}
			src = (long *)buff_info.buffer_mem;
			index = mBufs.valueFor(src);
			if (mFrameType & NATIVE_WINDOW_DATA) {
				mVideoInfo->refcount[index] += 1;
				if (mANativeWindow.get() == NULL) {
					ALOGE("Null window");
					return BAD_VALUE;
				}
				ret = mANativeWindow->dequeueBuffer_DEPRECATED(mANativeWindow.get(), &buf);
				if (ret != 0) {
					ALOGE("dequeue buffer failed :%s (%d)", strerror(-ret), -ret);
					return BAD_VALUE;
				}
				mANativeWindow->lockBuffer_DEPRECATED(mANativeWindow.get(), buf);
				sp<GraphicBuffer> graphicBuffer(new GraphicBuffer(buf, false));
				graphicBuffer->lock(SCREENSOURCE_GRALLOC_USAGE, (void **)&dest);
				if (dest == NULL) {
					ALOGE("Invalid Gralloc Handle");
					return BAD_VALUE;
				}
				memcpy(dest, src, mBufferSize);
				graphicBuffer->unlock();
				mANativeWindow->queueBuffer_DEPRECATED(mANativeWindow.get(), buf);
				graphicBuffer.clear();
				ALOGV("queue one buffer to native window");
				release_buffer(src);
			}
			if (mFrameType & CALL_BACK_DATA && mDataCB != NULL && mState == START) {
				mVideoInfo->refcount[index] += 1;
				mDataCB(mUser, &buff_info);
			}
		}
		return NO_ERROR;
	}

}
