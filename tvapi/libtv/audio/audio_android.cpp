#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <media/mediarecorder.h>
#include <system/audio.h>
#include <android/log.h>
#include <cutils/properties.h>
#include <media/AudioSystem.h>
#include <sys/prctl.h>
#include <math.h>

#include "audio_cfg.h"
#include "audio_api.h"
#include "audio_android.h"
#include "audio_android_effect.h"

#define msleep(x) usleep(x*1000)

#define LOG_TAG "AndroidAudio"
#include "CTvLog.h"

#define SOFTWARE_RESAMPLE

#define HDMI_PERIOD_SIZE		1024
#define PERIOD_SIZE				512

#define debug					0
//temp buffer between record thread and playback thread
#define temp_buffer_size    	1024*14
#define mid_buffer_size			1024*7		//reset distance for HDMI
#define reset_distance			1024*2		//reset distance for atv


#ifdef SOFTWARE_RESAMPLE
#define Upresample_distance		1024*10
#define Downresample_distance	1024*4

#define period_size				2			//1024*periodsize
#define resample_ratio			16			// unit:one in a thousand
#define fraction_bit			16			//16.16 type
#define upsample_framecount		(1024 + resample_ratio)
#define upsample_ratio			(unsigned int)((float)downsample_framecount*65536.0f/(float)1024)
#define downsample_framecount	(1024 - resample_ratio)
#define downsample_ratio		(unsigned int)((float)upsample_framecount*65536.0f/(float)1024)
#define resample_counter		(1024/resample_ratio)

short *resample_temp_buffer = NULL;

const unsigned int FractionStep_up = upsample_ratio;
const unsigned int FractionStep_down = downsample_ratio;
unsigned int Upsample_Fraction = 0;
unsigned int Downsample_Fraction = 0;
#endif

#if debug == 1
int record_counter = 0;
int playback_counter = 0;
#endif


#define HISTORY_BASE   (12)
#define HISTORY_NUM    (1<<12)
static signed history[2][HISTORY_NUM];

//showbo
static const int CC_AUDIO_SOURCE_IN_HDMI = 0;//just test,not true value

CAndroidAudio::CAndroidAudio(CAudioAlsa *p): mpAudioAlsa(p)
{
	int mDumpDataFlag = 0;
	int mDumpDataFd = -1;
	mlpRecorder = NULL;
	mlpTracker = NULL;
	temp_buffer = NULL;
	end_temp_buffer = NULL;
	record_write_pointer = NULL;
	playback_read_pointer = NULL;
	gEnableNoiseGate = false;
	gUserSetEnableNoiseGate = false;
	zero_count_left = 2000;
	zero_count_right = 2000;
	NOISE_HIS = 48000 * 5;
	gNoiseGateThresh = 0;
}
CAndroidAudio::~CAndroidAudio()
{
}

void CAndroidAudio::noise_filter_init()
{
	memset(history, 0, sizeof(history));
	zero_count_left = 2000;
	zero_count_right = 2000;
}

signed CAndroidAudio::noise_filter_left(signed sample)
{
	//if(!enable_noise_filter ||hdmi_src_in)
	//	return sample;
	signed sum_left = 0;
	signed sum_right = 0;
	unsigned left_pos = 0;
	unsigned right_pos = 0;
	signed y = 0;
	int i = 0;
	signed sum = 0;
	signed ret_sample;
	signed zero = 0;
	sample <<= 16;
	sample >>= 16;
#if 0
	if(hdmi_src_in) { //for hdmi src,not use filter and  -6db for prevent clip of SRS/other post process.
		ret_sample = Mul28(sample, M3DB);
		return ret_sample;
	} else {
		ret_sample = Mul28(sample, M3DB);
		sample = ret_sample;
	}
#endif
	sum_left -= history[0][left_pos];
	sum_left += history[0][(left_pos + HISTORY_NUM - 1)
						   & ((1 << HISTORY_BASE) - 1)];
	sum = sum_left >> HISTORY_BASE;
	left_pos = (left_pos + 1) & ((1 << HISTORY_BASE) - 1);
	history[0][(left_pos + HISTORY_NUM - 1) & ((1 << HISTORY_BASE) - 1)] =
		sample;

	zero = abs(sample - sum);
	if (zero < gNoiseGateThresh) {
		zero_count_left++;
		if (zero_count_left > NOISE_HIS) {
			zero_count_left = NOISE_HIS;
			y = 0;
		} else {
			y = sample;
		}
	} else {
		y = sample;
		zero_count_left = 0;
	}
	return y;
}

signed CAndroidAudio::noise_filter_right(signed sample)
{
	//if(!enable_noise_filter ||hdmi_src_in)
	//	return sample;
	signed y = 0;
	int i = 0;
	signed ret_sample;
	signed sum = 0;
	sample <<= 16;
	sample >>= 16;
#if 0
	if(hdmi_src_in) { //for hdmi src,not use filter and  -6db for prevent clip of SRS/other post process.
		ret_sample = Mul28(sample, M3DB);
		return ret_sample;
	} else {
		ret_sample = Mul28(sample, M3DB);
		sample = ret_sample;
	}
#endif
	sum_right -= history[1][right_pos];
	sum_right += history[1][(right_pos + HISTORY_NUM - 1)
							& ((1 << HISTORY_BASE) - 1)];
	sum = sum_right >> HISTORY_BASE;
	right_pos = (right_pos + 1) & ((1 << HISTORY_BASE) - 1);
	history[1][(right_pos + HISTORY_NUM - 1) & ((1 << HISTORY_BASE) - 1)] =
		sample;

	if (abs(sample - sum) < gNoiseGateThresh) {
		zero_count_right++;
		if (zero_count_right > NOISE_HIS) {
			zero_count_right = NOISE_HIS;
			y = 0;
		} else {
			y = sample;
		}
	} else {
		y = sample;
		zero_count_right = 0;
	}
	return y;
}

void CAndroidAudio::DeleteAudioRecorder(void)
{
#if ANDROID_PLATFORM_SDK_VERSION < 19
	if (mlpRecorder != NULL) {
		delete mlpRecorder;
		mlpRecorder = NULL;
	}
#else
	if (mlpRecorder != NULL ) {
		mlpRecorder = NULL;
	}

	if (mmpAudioRecorder != NULL ) {
		mmpAudioRecorder.clear();
	}
#endif
}

void CAndroidAudio::FreeAudioRecorder(void)
{
	if (mlpRecorder != NULL) {
		mlpRecorder->stop();
	}

	DeleteAudioRecorder();
}

void CAndroidAudio::DeleteAudioTracker(void)
{
#if ANDROID_PLATFORM_SDK_VERSION < 19
	if (mlpTracker != NULL) {
		delete mlpTracker;
		mlpTracker = NULL;
	}
#else
	if (mlpTracker != NULL ) {
		mlpTracker = NULL;
	}

	if (mmpAudioTracker != NULL ) {
		mmpAudioTracker.clear();
	}
#endif
}

void CAndroidAudio::FreeAudioTracker(void)
{
	if (mlpTracker != NULL) {
		mlpTracker->stop();
	}

	DeleteAudioTracker();
}

int CAndroidAudio::InitTempBuffer()
{
	int tmp_size = 0;

	if (NULL == temp_buffer) {
		tmp_size = temp_buffer_size;
		temp_buffer = new short[tmp_size];
		if (NULL == temp_buffer) {
			return -1;
		}
		memset(temp_buffer, 0, tmp_size * sizeof(short));
		end_temp_buffer = temp_buffer + tmp_size;
		record_write_pointer = temp_buffer;
		playback_read_pointer = temp_buffer;
	}
#ifdef SOFTWARE_RESAMPLE
	if (NULL == resample_temp_buffer) {
		tmp_size = upsample_framecount * 2 * period_size + 10;
		resample_temp_buffer = new short[tmp_size];
		if (NULL == resample_temp_buffer) {
			return -1;
		}
		memset(temp_buffer, 0, tmp_size * sizeof(short));
	}
#endif
	return 0;
}

static void MuteTempBuffer()
{
	if ((NULL != temp_buffer) && (NULL != mlpTracker)) {
		playback_read_pointer = record_write_pointer;
		memset(temp_buffer, 0, temp_buffer_size * sizeof(short));
		for (int i = 0; i < 10; i++) {
			mlpTracker->write(temp_buffer, temp_buffer_size);
		}
	}
}

void CAndroidAudio::FreeTempBuffer()
{
	if (temp_buffer != NULL) {
		delete[] temp_buffer;
		temp_buffer = NULL;
		end_temp_buffer = NULL;
		record_write_pointer = NULL;
		playback_read_pointer = NULL;
	}
#ifdef SOFTWARE_RESAMPLE
	if (resample_temp_buffer != NULL) {
		delete[] resample_temp_buffer;
		resample_temp_buffer = NULL;
	}
#endif
}

void CAndroidAudio::ResetRecordWritePointer()
{
	record_write_pointer = playback_read_pointer;
}

void CAndroidAudio::ResetPlaybackReadPointer()
{
	record_write_pointer = playback_read_pointer;
}

void CAndroidAudio::ResetPointer()
{
	if (mpAudioAlsa->GetAudioInSource() == CC_AUDIO_SOURCE_IN_HDMI) {
		playback_read_pointer = record_write_pointer - mid_buffer_size;
		if (playback_read_pointer < temp_buffer) {
			playback_read_pointer += temp_buffer_size;
		}
	} else {
		playback_read_pointer = record_write_pointer - reset_distance;
		if (playback_read_pointer < temp_buffer) {
			playback_read_pointer += temp_buffer_size;
		}
	}
}

// 0: LINEIN 1: HDMI
void CAndroidAudio::reset_system_framesize(int input_sample_rate, int output_sample_rate)
{
	int frame_size = 0;

	audio_io_handle_t handle = -1;

	if (mpAudioAlsa->GetAudioInSource() == CC_AUDIO_SOURCE_IN_HDMI) {
		frame_size = HDMI_PERIOD_SIZE * 4;
		LOGE("Source In:HDMI, Android system audio out framecount: %d \n",
			 frame_size);
	} else {
		//msleep(500);
		frame_size = PERIOD_SIZE * 4;
		LOGE("Source In:LIENIN, Android system audio out framecount: %d \n",
			 frame_size);
	}

	handle = AudioSystem::getInput(AUDIO_SOURCE_MIC, input_sample_rate,
								   AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_IN_STEREO, 0);

	if (handle > 0) {
		char str[64];
		memset(str, 0, sizeof(str));
		sprintf(str, "frame_count=%d", frame_size);
		LOGE("input handle number: %d \n", handle);
		AudioSystem::setParameters(handle, String8(str));
	} else {
		LOGE("Can't get input handle! \n");
	}

	handle = -1;

	handle = AudioSystem::getOutput(AUDIO_STREAM_MUSIC, output_sample_rate,
									AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_OUT_STEREO,
									AUDIO_OUTPUT_FLAG_NONE);

	if (handle > 0) {
		char str[64];
		memset(str, 0, sizeof(str));
		sprintf(str, "frame_count=%d", frame_size);
		LOGE("output handle number: %d \n", handle);
		AudioSystem::setParameters(handle, String8(str));
	} else {
		LOGE("Can't get output handle! \n");
	}
}

inline short CAndroidAudio::clip(int x)   //Clip from 16.16 fixed-point to 0.15 fixed-point.
{
	if (x < -32768)
		return -32768;
	else if (x > 32767)
		return 32767;
	else
		return x;
}

int CAndroidAudio::upsample(short *input, short *output, unsigned int FractionStep,
							unsigned int input_frame_size, unsigned int frac)
{
	unsigned int inputIndex = 1;
	unsigned int outputIndex = 0;

	while (inputIndex < input_frame_size) {
		*output++ = clip(
						input[2 * inputIndex - 2]
						+ (((input[2 * inputIndex] - input[2 * inputIndex - 2])
							* (int32_t) frac) >> 16));
		*output++ = clip(
						input[2 * inputIndex - 1]
						+ (((input[2 * inputIndex + 1]
							 - input[2 * inputIndex - 1]) * (int32_t) frac)
						   >> 16));

		frac += FractionStep;
		inputIndex += (frac >> 16);
		frac = (frac & 0xffff);
		outputIndex++;
	}

	return outputIndex;
}

int CAndroidAudio::downsample(short *input, short *output, unsigned int FractionStep,
							  unsigned int input_frame_size, unsigned int frac)
{
	unsigned int inputIndex = 1;
	unsigned int outputIndex = 0;

	while (inputIndex < input_frame_size) {
		*output++ = clip(
						input[2 * inputIndex - 2]
						+ (((input[2 * inputIndex] - input[2 * inputIndex - 2])
							* (int32_t) frac) >> 16));
		*output++ = clip(
						input[2 * inputIndex - 1]
						+ (((input[2 * inputIndex + 1]
							 - input[2 * inputIndex - 1]) * (int32_t) frac)
						   >> 16));

		frac += FractionStep;
		inputIndex += (frac >> 16);
		frac = (frac & 0xffff);
		outputIndex++;
	}

	return outputIndex;
}

int CAndroidAudio::GetWriteSpace(short volatile *WritePoint,
								 short volatile *ReadPoint)
{
	unsigned int space;

	if (WritePoint >= ReadPoint)
		space = temp_buffer_size - (WritePoint - ReadPoint);
	else
		space = ReadPoint - WritePoint;

	return space;
}
int CAndroidAudio::GetReadSpace(short volatile *WritePoint, short volatile *ReadPoint)
{
	unsigned int space;

	if (WritePoint >= ReadPoint)
		space = WritePoint - ReadPoint;
	else
		space = temp_buffer_size - (ReadPoint - WritePoint);

	return space;
}

void CAndroidAudio::recorderCallback(int event, void *user, void *info)
{
	if (AudioRecord::EVENT_MORE_DATA == event) {
		AudioRecord::Buffer *pbuf = (AudioRecord::Buffer *) info;

		if (NULL == mlpRecorder)
			return;

		unsigned int offset_bytes = 2
									* (end_temp_buffer - record_write_pointer);
		unsigned int copy_bytes;
		unsigned int output_bytes;

		int available_write_space;
		available_write_space = GetWriteSpace(record_write_pointer,
											  playback_read_pointer);

		//LOGE("~~~~~available_write_space = %d \n", available_write_space);

		if (available_write_space < pbuf->size / 2) {
			LOGE(
				"[%s]: ***** FULL! *****\n\trd=0x%x, wr=0x%x, wr_lvl = 0x%x, wr_req = 0x%x\n",
				__FUNCTION__, playback_read_pointer, record_write_pointer,
				available_write_space, pbuf->size / 2);

			if (mpAudioAlsa->GetAudioInSource() == CC_AUDIO_SOURCE_IN_HDMI) {
				playback_read_pointer += mid_buffer_size;
				if (playback_read_pointer >= end_temp_buffer) {
					playback_read_pointer -= temp_buffer_size;
				}
			} else {
				playback_read_pointer += (temp_buffer_size - reset_distance);
				if (playback_read_pointer >= end_temp_buffer) {
					playback_read_pointer -= temp_buffer_size;
				}
			}
			return;
		}
#if debug == 1
		if(record_counter > 500) {
			LOGE("~~~~~RecordCallback, pbuf->size:%d, pbuf->frameCount:%d", pbuf->size, pbuf->frameCount);
			record_counter = 0;
		} else {
			record_counter++;
		}
#endif

#ifdef SOFTWARE_RESAMPLE
		if (mpAudioAlsa->GetAudioInSource() == CC_AUDIO_SOURCE_IN_HDMI) {
			if (available_write_space <= Downresample_distance) {
				output_bytes = 4
							   * downsample((short *) pbuf->raw, resample_temp_buffer,
											FractionStep_down, pbuf->frameCount,
											Downsample_Fraction);

				LOGE(
					"downsample: output_framecount = %d, input_frameCount = %d\n",
					output_bytes / 4, pbuf->frameCount);

				if (offset_bytes >= output_bytes) {
					memcpy((short *) record_write_pointer, resample_temp_buffer,
						   output_bytes);
					record_write_pointer += output_bytes / 2;

					if (record_write_pointer == end_temp_buffer)
						record_write_pointer = temp_buffer;
				} else {
					memcpy((short *) record_write_pointer, resample_temp_buffer,
						   offset_bytes);
					copy_bytes = offset_bytes;
					offset_bytes = output_bytes - copy_bytes;
					record_write_pointer = temp_buffer;
					memcpy((short *) record_write_pointer,
						   resample_temp_buffer + (copy_bytes / 2),
						   offset_bytes);
					record_write_pointer += offset_bytes / 2;
				}
				return;
			} else if (available_write_space >= Upresample_distance) {
				output_bytes = 4
							   * upsample((short *) pbuf->raw, resample_temp_buffer,
										  FractionStep_up, pbuf->frameCount,
										  Upsample_Fraction);

				LOGE(
					"upsample: output_framecount = %d, input_frameCount = %d\n",
					output_bytes / 4, pbuf->frameCount);

				if (offset_bytes >= output_bytes) {
					memcpy((short *) record_write_pointer, resample_temp_buffer,
						   output_bytes);
					record_write_pointer += output_bytes / 2;

					if (record_write_pointer == end_temp_buffer)
						record_write_pointer = temp_buffer;
				} else {
					memcpy((short *) record_write_pointer, resample_temp_buffer,
						   offset_bytes);
					copy_bytes = offset_bytes;
					offset_bytes = output_bytes - copy_bytes;
					record_write_pointer = temp_buffer;
					memcpy((short *) record_write_pointer,
						   resample_temp_buffer + (copy_bytes / 2),
						   offset_bytes);
					record_write_pointer += offset_bytes / 2;
				}
				return;
			}

			Upsample_Fraction = 0;
			Downsample_Fraction = 0;
		}
#endif

		if (offset_bytes >= pbuf->size) {
			memcpy((short *) record_write_pointer, pbuf->raw, pbuf->size);
			record_write_pointer += pbuf->size / 2;

			if (record_write_pointer == end_temp_buffer)
				record_write_pointer = temp_buffer;
		} else {
			memcpy((short *) record_write_pointer, pbuf->raw, offset_bytes);
			copy_bytes = offset_bytes;
			offset_bytes = pbuf->size - copy_bytes;
			record_write_pointer = temp_buffer;
			memcpy((short *) record_write_pointer,
				   (short *) pbuf->raw + (copy_bytes / 2), offset_bytes);
			record_write_pointer += offset_bytes / 2;
		}

		//LOGD("--------RecordCallback, pbuf->size:%d, pbuf->frameCount:%d\n", pbuf->size, pbuf->frameCount);
		//LOGD("--------Record----offset_bytes:%d\n", 2*(record_write_pointer-temp_buffer));

	} else if (AudioRecord::EVENT_OVERRUN == event) {
		LOGE("[%s]: AudioRecord::EVENT_OVERRUN\n", __FUNCTION__);
	} else if (AudioRecord::EVENT_MARKER == event) {
		LOGD("[%s]: AudioRecord::EVENT_MARKER\n", __FUNCTION__);
	} else if (AudioRecord::EVENT_NEW_POS == event) {
		LOGD("[%s]: AudioRecord::EVENT_NEW_POS\n", __FUNCTION__);
	}
}

void CAndroidAudio::trackerCallback(int event, void *user, void *info)
{
	if (AudioTrack::EVENT_MORE_DATA == event) {
		AudioTrack::Buffer *pbuf = (AudioTrack::Buffer *) info;

		if (NULL == mlpTracker)
			return;

		int available_read_space;
		available_read_space = GetReadSpace(record_write_pointer,
											playback_read_pointer);

		//LOGE("~~~~~available_read_space = %d  input_bytes = %d \n", available_read_space,input_bytes);

		if (available_read_space < pbuf->size / 2) {
			LOGE(
				"[%s]: ***** EMPTY *****\n\trd=0x%x, wr=0x%x, rd_lvl=0x%x, rd_req=0x%x*****\n",
				__FUNCTION__, playback_read_pointer, record_write_pointer,
				available_read_space, pbuf->size / 2);

			if (mpAudioAlsa->GetAudioInSource() == CC_AUDIO_SOURCE_IN_HDMI) {
				playback_read_pointer -= mid_buffer_size;
				if (playback_read_pointer < temp_buffer)
					playback_read_pointer += temp_buffer_size;
			} else {
				playback_read_pointer -= reset_distance;
				if (playback_read_pointer < temp_buffer)
					playback_read_pointer += temp_buffer_size;
			}

		} else {
			unsigned int offset_bytes = 2
										* (end_temp_buffer - playback_read_pointer);
			unsigned int copy_bytes;
			int sampNum;

			if (offset_bytes >= pbuf->size) {
				if (gEnableNoiseGate) {
					sampNum = pbuf->size / 2;
					for (int i = 0; i < sampNum; i++) {
						pbuf->i16[i] = (short) noise_filter_left(
										   (signed) (playback_read_pointer[i]));
						i++;
						pbuf->i16[i] = (short) noise_filter_left(
										   (signed) (playback_read_pointer[i]));
					}
				} else {
					memcpy(pbuf->raw, (short *) playback_read_pointer,
						   pbuf->size);
				}

				memset((void *) playback_read_pointer, 0, pbuf->size);
				playback_read_pointer += pbuf->size / 2;

				if (playback_read_pointer == end_temp_buffer)
					playback_read_pointer = temp_buffer;
			} else {
				if (gEnableNoiseGate) {
					sampNum = offset_bytes / 2;
					for (int i = 0; i < sampNum; i++) {
						pbuf->i16[i] = (short) noise_filter_left(
										   (signed) (playback_read_pointer[i]));
						i++;
						pbuf->i16[i] = (short) noise_filter_left(
										   (signed) (playback_read_pointer[i]));
					}
				} else {
					memcpy(pbuf->raw, (short *) playback_read_pointer,
						   offset_bytes);
				}

				memset((void *) playback_read_pointer, 0, offset_bytes);
				copy_bytes = offset_bytes;
				offset_bytes = pbuf->size - copy_bytes;
				playback_read_pointer = temp_buffer;

				if (gEnableNoiseGate) {
					short *pSrcPtr = &(pbuf->i16[copy_bytes / 2]);
					sampNum = offset_bytes / 2;
					for (int i = 0; i < sampNum; i++) {
						pSrcPtr[i] = (short) noise_filter_left(
										 (signed) (playback_read_pointer[i]));
						i++;
						pSrcPtr[i] = (short) noise_filter_left(
										 (signed) (playback_read_pointer[i]));
					}
				} else {
					memcpy((short *) pbuf->raw + (copy_bytes / 2),
						   (short *) playback_read_pointer, offset_bytes);
				}
				memset((void *) playback_read_pointer, 0, offset_bytes);
				playback_read_pointer += offset_bytes / 2;
			}
			DoDumpData(pbuf->raw, pbuf->size);

#if debug == 1
			if(playback_counter > 500) {
				LOGE("----PlaybackCallback, pbuf->size:%d, pbuf->frameCount:%d", pbuf->size, pbuf->frameCount);
				playback_counter = 0;
			} else {
				playback_counter++;
			}
#endif

		}
		//LOGE("---Playback---offset_bytes:%d\n", 2*(playback_read_pointer-temp_buffer));
	} else if (AudioTrack::EVENT_UNDERRUN == event) {
		LOGE("[%s]: AudioTrack::EVENT_UNDERRUN\n", __FUNCTION__);
	} else if (AudioTrack::EVENT_LOOP_END == event) {
		LOGD("[%s]: AudioTrack::EVENT_LOOP_END\n", __FUNCTION__);
	} else if (AudioTrack::EVENT_MARKER == event) {
		LOGD("[%s]: AudioTrack::EVENT_MARKER\n", __FUNCTION__);
	} else if (AudioTrack::EVENT_NEW_POS == event) {
		LOGD("[%s]: AudioTrack::EVENT_NEW_POS\n", __FUNCTION__);
	} else if (AudioTrack::EVENT_BUFFER_END == event) {
		LOGD("[%s]: AudioTrack::EVENT_BUFFER_END\n", __FUNCTION__);
	}
}

int CAndroidAudio::initAudioTracker(int sr)
{
	status_t ret;
	int tmp_session_id = 0;

	if (NULL != mlpTracker) {
		LOGE("[%s:%d] Trying to new AudioTrack while it's still not NULL.\n",
			 __FUNCTION__, __LINE__);
		goto err_exit;
	}

#if ANDROID_PLATFORM_SDK_VERSION < 19
	mlpTracker = new AudioTrack();
	if (NULL == mlpTracker) {
		LOGE("[%s:%d] Failed to new AudioTrack.\n", __FUNCTION__, __LINE__);
		goto err_exit;
	}
#else
	mmpAudioTracker = new AudioTrack();
	mlpTracker = mmpAudioTracker.get();
#endif

	tmp_session_id = amAndroidGetAudioSessionId();
	ret = mlpTracker->set(AUDIO_STREAM_DEFAULT, //inputSource
						  sr, //sampleRate
						  AUDIO_FORMAT_PCM_16_BIT, //format
						  AUDIO_CHANNEL_IN_STEREO, //channelMask
						  0, //frameCount
						  AUDIO_OUTPUT_FLAG_NONE, //flags
						  trackerCallback, //trackerCallback,
						  NULL, //user when callback
						  0, //notificationFrames
						  NULL, //shared buffer
						  false, //threadCanCallJava
						  tmp_session_id //sessionId
						 );

	if (NO_ERROR != ret) {
		LOGE("[%s:%d] Failed to set AudioTrack parameters. status=%d\n",
			 __FUNCTION__, __LINE__, ret);
		goto err_exit;
	}

	ret = mlpTracker->initCheck();
	if (NO_ERROR != ret) {
		LOGE("[%s:%d] Failed to init AudioTrack. status=%d\n", __FUNCTION__,
			 __LINE__, ret);
		goto err_exit;
	}

	mlpTracker->start();
	ResetPointer();

#if debug == 1
	uint32_t frame_count;
	frame_count = mlpTracker->frameCount();
	LOGE("~~~~~~[%s:%d] frame_count = %u\n", __FUNCTION__, __LINE__, frame_count);
#endif

	return 0;

err_exit: //
	DeleteAudioTracker();

	return -1;
}

int CAndroidAudio::initAudioRecorder(int sr)
{
	status_t ret;

	if (NULL != mlpRecorder) {
		LOGE("[%s:%d] Trying to new AudioRecord while it's still not NULL.\n",
			 __FUNCTION__, __LINE__);
		goto err_exit;
	}

#if ANDROID_PLATFORM_SDK_VERSION < 19
	mlpRecorder = new AudioRecord();
	if (NULL == mlpRecorder) {
		LOGE("[%s:%d] Failed to new AudioRecord.\n", __FUNCTION__, __LINE__);
		goto err_exit;
	}
#else
	mmpAudioRecorder = new AudioRecord();
	mlpRecorder = mmpAudioRecorder.get();
#endif

	ret = mlpRecorder->set(AUDIO_SOURCE_DEFAULT, //inputSource
						   sr, //sampleRate
						   AUDIO_FORMAT_PCM_16_BIT, //format
						   AUDIO_CHANNEL_IN_STEREO, //channelMask
						   0, //frameCount
						   recorderCallback, //callback_t
						   NULL, //void* user
						   0, //notificationFrames,
						   false, //threadCanCallJava
						   0 //sessionId
						  );

	if (NO_ERROR != ret) {
		LOGE("[%s:%d] Failed to set AudioRecord parameters. status=%d\n",
			 __FUNCTION__, __LINE__, ret);
		goto err_exit;
	}

	ret = mlpRecorder->initCheck();
	if (NO_ERROR != ret) {
		LOGE("[%s:%d] Failed to init AudioRecord. status=%d\n", __FUNCTION__,
			 __LINE__, ret);
		goto err_exit;
	}

	ret = mlpRecorder->start();
	if (NO_ERROR != ret) {
		LOGE("[%s:%d] Failed to start AudioRecord. status=%d\n", __FUNCTION__,
			 __LINE__, ret);
		goto err_exit;
	}
#if debug == 1
	uint32_t frame_count;
	frame_count = mlpRecorder->frameCount();
	LOGE("~~~~~~[%s:%d] frame_count = %u\n", __FUNCTION__, __LINE__, frame_count);
#endif

	return 0;
err_exit: //
	DeleteAudioRecorder();
	return -1;
}

int CAndroidAudio::amAndroidInit(int tm_sleep, int init_flag, int recordSr, int trackSr,
								 bool enable_noise_gate)
{
	int tmp_ret;
	LOGD("Enter amAndroidInit function.\n");

	amAndroidUninit(0);

	// Init the noise gate filter
	gUserSetEnableNoiseGate = enable_noise_gate;
	if (enable_noise_gate && (gNoiseGateThresh > 0)
			&& GetAudioNoiseGateEnableCFG()) {
		gEnableNoiseGate = true;
		noise_filter_init();
	} else {
		gEnableNoiseGate = false;
	}
	LOGE("[%s:%d] noise gate enabled:%d!\n", __FUNCTION__, __LINE__,
		 gEnableNoiseGate);

	if (InitTempBuffer() != 0) {
		LOGE("[%s:%d] Failed to create temp_buffer!\n", __FUNCTION__, __LINE__);
		return 0;
	}

	if (init_flag & CC_FLAG_CREATE_RECORD) {
		//LOGD("Start to create Recorder. RecordSr:%d\n", recordSr);
		if (0 != initAudioRecorder(recordSr)) {
			LOGE(" [%s:%d] Created AudioRecorder disable !\n", __FUNCTION__,
				 __LINE__);
			return -1;
		}
		//LOGD("[%s:%d] End to create recorder.\n", __FUNCTION__, __LINE__);
	}

	if (init_flag & CC_FLAG_CREATE_TRACK) {
		//LOGD("Start to create Tracker. TrackSr:%d\n", trackSr);
		if (0 != initAudioTracker(trackSr)) {
			//FreeAudioRecorder();
			//FreeAudioTracker();
			LOGE(" [%s:%d] Created AudioTrack disable !\n", __FUNCTION__,
				 __LINE__);
			return -1;
		}
		//LOGD("[%s:%d] End to create recorder.\n", __FUNCTION__, __LINE__);
	}

	if (tm_sleep > 0)
		sleep(tm_sleep);

	LOGD("Exit amAndroidInit function sucess.\n");

	return 0;
}

int CAndroidAudio::amAndroidUninit(int tm_sleep)
{
	LOGD("Enter amAndroidUninit function.\n");

	FreeAudioRecorder();
	// MuteTempBuffer();
	FreeAudioTracker();
	FreeTempBuffer();

	if (tm_sleep > 0)
		sleep(tm_sleep);

	LOGD("Exit amAndroidUninit function sucess.\n");

	return 0;
}

int CAndroidAudio::amAndroidSetRecorderSr(int sr)
{
	int ret;
	FreeAudioRecorder();
	ResetRecordWritePointer();
	return initAudioRecorder(sr);
}

int CAndroidAudio::amAndroidSetTrackerSr(int sr)
{
	FreeAudioTracker();
	ResetPlaybackReadPointer();
	return initAudioTracker(sr);
}

//
// Set the noise gate threshold.
// If it's set to 0, the noise gate filter is disabled.
// Theoretically, this variable should be protected by mutex. But this API
// is designed to be called once on system boot, or for debug purpose, and it's
// set only through this API. To improve the performance of the filters, we
// don't utilize any mutex here.
void CAndroidAudio::amAndroidSetNoiseGateThreshold(int thresh)
{
	int upperBound = GetAudioNoiseGateUpperBoundCFG();
	int forcedThresh = GetAudioNoiseGateThresholdCFG();
	if (forcedThresh >= 0) {
		LOGE("Force noise gate threshold from %d to %d\n", thresh,
			 forcedThresh);
		thresh = forcedThresh;
	}

	if ((thresh >= 0) && (thresh <= upperBound)) {
		gNoiseGateThresh = thresh;
	} else {
		gNoiseGateThresh = 0;
	}

	if (gUserSetEnableNoiseGate && (gNoiseGateThresh > 0)
			&& GetAudioNoiseGateEnableCFG()) {
		gEnableNoiseGate = true;
	} else {
		gEnableNoiseGate = false;
	}
	LOGE(
		"[%s:%d] thresh:%d, upperBound:%d, gNoiseGateThresh:%d, gEnableNoiseGate:%d\n",
		__FUNCTION__, __LINE__, thresh, upperBound, gNoiseGateThresh,
		gEnableNoiseGate);
}

int CAndroidAudio::amAndroidSetDumpDataFlag(int tmp_flag)
{
	mDumpDataFlag = tmp_flag;
	return mDumpDataFlag;
}

int CAndroidAudio::amAndroidGetDumpDataFlag()
{
	return mDumpDataFlag;
}

void CAndroidAudio::DoDumpData(void *data_buf, int size)
{
	int tmp_flag = 0;
	char prop_value[PROPERTY_VALUE_MAX];

	if (amAndroidGetDumpDataFlag() == 0) {
		return;
	}

	memset(prop_value, '\0', PROPERTY_VALUE_MAX);

	property_get("audio.dumpdata.en", prop_value, "null");
	if (strcasecmp(prop_value, "null") == 0
			|| strcasecmp(prop_value, "0") == 0) {
		if (mDumpDataFd >= 0) {
			close(mDumpDataFd);
			mDumpDataFd = -1;
		}

		return;
	}

	memset(prop_value, '\0', PROPERTY_VALUE_MAX);

	property_get("audio.dumpdata.path", prop_value, "null");
	if (strcasecmp(prop_value, "null") == 0) {
		return;
	}

	if (mDumpDataFd < 0) {
		if (access(prop_value, 0) == 0) {
			mDumpDataFd = open(prop_value, O_RDWR | O_SYNC);
			if (mDumpDataFd < 0) {
				LOGE("%s, Open device file \"%s\" error: %s.\n", __FUNCTION__,
					 prop_value, strerror(errno));
				return;
			}
		} else {
			mDumpDataFd = open(prop_value, O_WRONLY | O_CREAT | O_EXCL,
							   S_IRUSR | S_IWUSR);
			if (mDumpDataFd < 0) {
				LOGE("%s, Create device file \"%s\" error: %s.\n", __FUNCTION__,
					 prop_value, strerror(errno));
				return;
			}
		}
	}

	write(mDumpDataFd, data_buf, size);
}

