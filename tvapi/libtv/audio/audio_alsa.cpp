#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <tinyalsa/asoundlib.h>
#include "CFbcCommunication.h"
#include <sys/poll.h>
#include <android/log.h>
#include "tvsetting/audio_cfg.h"
#include "audio_alsa.h"
#include <cutils/properties.h>
#define LOG_TAG "CAudioAlsa"
#include "CTvLog.h"
#include "audio_amaudio.h"

static const int MainDigitLutBuf[CC_VOL_TRANS_LUT_BUF_SIZE] = {
	//
	0, 40, 50, 60, 70, 87, 110, 120, 130, 140, // 0~9
	150, 152, 155, 158, 161, 164, 167, 170, 173, 174, // 10~19
	179, 180, 181, 182, 183, 184, 185, 186, 187, 188, // 20~29
	191, 191, 192, 193, 194, 195, 196, 196, 197, 197, // 30~39
	198, 198, 198, 199, 199, 200, 200, 201, 201, 201, // 40~49
	202, 202, 202, 203, 203, 203, 203, 204, 204, 204, // 50~59
	205, 205, 205, 205, 206, 206, 206, 206, 206, 207, // 60~69
	207, 207, 207, 207, 207, 207, 207, 208, 208, 208, // 70~79
	208, 208, 208, 208, 208, 208, 208, 208, 208, 209, // 80~89
	209, 209, 209, 209, 209, 209, 209, 209, 209, 209, // 90~99
	209, // 100
};

static const int SupperBassDigitLutBuf[CC_VOL_TRANS_LUT_BUF_SIZE] = {
	//
	0, 122, 123, 124, 125, 126, 127, 128, 129, 130, // 0~9
	131, 132, 133, 134, 135, 136, 137, 138, 139, 140, // 10~19
	141, 142, 143, 144, 145, 147, 147, 148, 148, 149, // 20~29
	149, 150, 150, 151, 151, 152, 152, 153, 153, 154, // 30~39
	154, 155, 155, 156, 156, 157, 157, 158, 158, 159, // 40~49
	160, 160, 160, 160, 161, 161, 161, 161, 162, 162, // 50~59
	162, 163, 163, 163, 163, 164, 164, 164, 165, 165, // 60~69
	165, 165, 166, 166, 166, 167, 168, 169, 170, 171, // 70~79
	173, 174, 175, 176, 177, 179, 180, 181, 182, 183, // 80~89
	185, 186, 187, 188, 189, 191, 192, 193, 194, 195, // 90~99
	197 // 100
};

CAudioAlsa::CAudioAlsa()
{
	int card_id;
	card_id = get_aml_card();
	LOGD("[%s:%d] card_id:%d\n", __FUNCTION__, __LINE__, card_id);
	mpMixer = mixer_open(card_id);

	card_id = get_USB_Audio_card();
	LOGD("[%s:%d] card_id:%d\n", __FUNCTION__, __LINE__, card_id);
	mpUsbMixer = mixer_open(card_id);
	mMainVolumeBalanceVal = 0;
	mMainVolumeGainVal = 0;
	mSupperBassVolumeGainVal = 0;
	memcpy(mMainDigitLutBuf, MainDigitLutBuf, CC_VOL_TRANS_LUT_BUF_SIZE);
	memcpy(mSupperBassDigitLutBuf, SupperBassDigitLutBuf, CC_VOL_TRANS_LUT_BUF_SIZE);
}

CAudioAlsa::~CAudioAlsa()
{
	if (NULL != mpMixer) {
		mixer_close(mpMixer);
	}

	if (NULL != mpUsbMixer) {
		mixer_close(mpUsbMixer);
	}
}

int CAudioAlsa::get_aml_card()
{
	int card = -1, err = 0;
	int fd = -1;
	char read_buf[512], *pd = NULL;
	static const char *const SOUND_CARDS_PATH = "/proc/asound/cards";
	fd = open(SOUND_CARDS_PATH, O_RDONLY);
	if (fd < 0) {
		ALOGE("ERROR: failed to open config file %s error: %d\n", SOUND_CARDS_PATH, errno);
		close(fd);
		return -EINVAL;
	}
	memset(read_buf, 0x0, 512);
	err = read(fd, read_buf, 512);
	if (fd < 0) {
		ALOGE("ERROR: failed to read config file %s error: %d\n", SOUND_CARDS_PATH, errno);
		close(fd);
		return -EINVAL;
	}
	pd = strstr(read_buf, "AML");
	card = *(pd - 3) - '0';
	close(fd);
	return card;
}

int CAudioAlsa::AudioControlSetValue(int val_count, int data_buf[], char *match_names)
{
	struct mixer_ctl *pctl;
	unsigned int expected_val_count;
	unsigned int value_index;

	if (NULL == mpMixer) {
		LOGE("[%s:%d] Failed to open mixer\n", __FUNCTION__, __LINE__);
		goto err_exit;
	}

	pctl = mixer_get_ctl_by_name(mpMixer, match_names);
	if (NULL == pctl) {
		LOGE("[%s:%d] Failed to get mixer control for:%s\n", __FUNCTION__, __LINE__, match_names);
		goto err_exit;
	}
	expected_val_count = mixer_ctl_get_num_values(pctl);
	if (expected_val_count != (unsigned int)val_count) {
		LOGE("[%s:%d] val_count != expected_val_count\n", __FUNCTION__, __LINE__);
		goto err_exit;
	}
	for (value_index = 0; value_index < expected_val_count; value_index++) {
		if (mixer_ctl_set_value(pctl, value_index, data_buf[value_index]) != 0) {
			LOGE("[%s:%d] Failed to set value:%d\n", __FUNCTION__, __LINE__, value_index);
			goto err_exit;
		}
	}
	return 0;
err_exit:
	return -1;
}

int CAudioAlsa::AudioControlGetValue(int val_count, int ret_buf[], char *match_names)
{
	struct mixer_ctl *pctl;
	unsigned int expected_val_count;
	unsigned int value_index;

	if (NULL == mpMixer) {
		LOGE("[%s:%d] Failed to open mixer\n", __FUNCTION__, __LINE__);
		goto err_exit;
	}
	pctl = mixer_get_ctl_by_name(mpMixer, match_names);
	if (NULL == pctl) {
		LOGE("[%s:%d] Failed to get mixer control for:%s\n", __FUNCTION__, __LINE__, match_names);
		goto err_exit;
	}
	expected_val_count = mixer_ctl_get_num_values(pctl);
	if (expected_val_count != (unsigned int)val_count) {
		LOGE("[%s:%d] val_count != expected_val_count\n", __FUNCTION__, __LINE__);
		goto err_exit;
	}
	for (value_index = 0; value_index < expected_val_count; value_index++) {
		ret_buf[value_index] = mixer_ctl_get_value(pctl, value_index);
	}
	return 0;
err_exit:
	return -1;
}

int CAudioAlsa::get_USB_Audio_card()
{
	int card = -1, err = 0;
	int fd = -1;
	char read_buf[512], *pd = NULL;
	static const char *const SOUND_CARDS_PATH = "/proc/asound/cards";
	fd = open(SOUND_CARDS_PATH, O_RDONLY);
	if (fd < 0) {
		ALOGE("ERROR: failed to open config file %s error: %d\n", SOUND_CARDS_PATH, errno);
		close(fd);
		return -EINVAL;
	}
	memset(read_buf, 0x0, 512);
	err = read(fd, read_buf, 512);
	if (fd < 0) {
		ALOGE("ERROR: failed to read config file %s error: %d\n", SOUND_CARDS_PATH, errno);
		close(fd);
		return -EINVAL;
	}
	pd = strstr(read_buf, "Receiver");
	if (pd == NULL) {
		ALOGE("ERROR: failed to read config file %s error: %d\n", SOUND_CARDS_PATH, errno);
		close(fd);
		return -EINVAL;
	}

	card = *(pd - 3) - '0';
	close(fd);
	return card;
}

int CAudioAlsa::HandleUSBAudioControlValue(int val_count, int data_buf[], int match_count, char **match_names)
{

	struct mixer_ctl *pctl;
	unsigned int expected_val_count;
	unsigned int match_index;
	unsigned int value_index;
	char card_id_str[9] = {0};

	if (NULL == mpUsbMixer) {
		LOGE("[%s:%d] Failed to open mixer\n", __FUNCTION__, __LINE__);
		goto err_exit;
	}

	for (match_index = 0; match_index < match_count; match_index++) {
		pctl = mixer_get_ctl_by_name(mpUsbMixer, match_names[match_index]);
		if (NULL == pctl) {
			LOGE("[%s:%d] Failed to get mixer control for:%s\n", __FUNCTION__, __LINE__, match_names[match_index]);
			goto err_exit;
		}
		expected_val_count = mixer_ctl_get_num_values(pctl);
		if (expected_val_count != (unsigned int)val_count) {
			LOGE("[%s:%d] val_count != expected_val_count\n", __FUNCTION__, __LINE__);
			goto err_exit;
		}
		for (value_index = 0; value_index < expected_val_count; value_index++) {
			if (mixer_ctl_set_value(pctl, value_index, data_buf[value_index]) != 0) {
				LOGE("[%s:%d] Failed to set value:%d\n", __FUNCTION__, __LINE__, value_index);
				goto err_exit;
			}
		}
	}

	return 0;
err_exit:
	return -1;
}

int CAudioAlsa::CheckVolume(int digit_vol, int digit_min, int digit_max, int hd_min,
							int hd_max)
{
	int tmp_val = digit_vol;

	if (digit_vol < hd_min) {
		return hd_min;
	} else if (digit_vol > hd_max) {
		return hd_max;
	}

	return digit_vol;
}

int CAudioAlsa::GetTwoChannelVolume(int vol_buf[], int l_min_vol, int l_max_vol,
									int r_min_vol, int r_max_vol, char *match_names, int hd_min,
									int hd_max)
{
	vol_buf[0] = 0;
	vol_buf[1] = 0;
	return AudioControlGetValue(2, vol_buf, match_names);
}

int CAudioAlsa::GetLineInMaxVol()
{
	if (GetAudioArchitectureTypeCFG() == CC_DAC_M6C_EXTERNAL_DAC_ON_BOARD) {
		return 84;
	}

	return 127;
}

int CAudioAlsa::GetLineOutMaxVol()
{
	if (GetAudioArchitectureTypeCFG() == CC_DAC_M6C_EXTERNAL_DAC_ON_BOARD) {
		return 84;
	}

	return 255;
}

#define CC_GET_ALSA_CTL_AUDIO_IN_SOURCE_NAME                          (0)
#define CC_GET_ALSA_CTL_INTERNAL_DAC_LEFT_LINEIN_SEL_NAME             (1)
#define CC_GET_ALSA_CTL_INTERNAL_DAC_RIGHT_LINEIN_SEL_NAME            (2)
#define CC_GET_ALSA_CTL_INTERNAL_DAC_LINEIN_CAPTURE_VOLUME_NAME       (3)
#define CC_GET_ALSA_CTL_INTERNAL_DAC_LINEOUT_PLAYBACK_VOLUME_NAME     (4)
#define CC_GET_ALSA_CTL_I2S_MUTE_NAME                                 (5)
#define CC_GET_ALSA_CTL_SPDIF_MUTE_NAME                               (6)
#define CC_GET_ALSA_CTL_HW_RESAMPLE_NAME                              (7)
#define CC_GET_ALSA_CTL_EXTERNAL_DAC_MASTER_VOLUME_NAME               (8)
#define CC_GET_ALSA_CTL_EXTERNAL_DAC_CH1_VOLUME_NAME                  (9)
#define CC_GET_ALSA_CTL_EXTERNAL_DAC_CH2_VOLUME_NAME                  (10)
#define CC_GET_ALSA_CTL_EXTERNAL_DAC_CH3_VOLUME_NAME                  (11)
#define CC_GET_ALSA_CTL_EXTERNAL_DAC_CH1_SWITCH_NAME                  (12)
#define CC_GET_ALSA_CTL_EXTERNAL_DAC_CH2_SWITCH_NAME                  (13)
#define CC_GET_ALSA_CTL_EXTERNAL_DAC_CH3_SWITCH_NAME                  (14)
#define CC_GET_ALSA_CTL_EXTERNAL_DAC_EQ_MODE_NAME                     (15)
#define CC_GET_ALSA_CTL_PCM_PLAYBACK_VOLUME                     (16)
#define CC_GET_ALSA_CTL_PCM_PLAYBACK_SWITCH                     (17)
#define CC_GET_ALSA_CTL_SETOUTPUT_SWAP                              (18)
#define CC_GET_ALSA_CTL_AUDIO_IN_SWITCH                     (19)

static  char  gG9AlsaNames[32][48] = {
	{"Audio In Source"},//0
	{"Linein right switch"},//1
	{"Linein left switch"},
	{"ADC Digital Capture Volume"},
	{"DAC Digital Playback Volume"},
	{"Audio i2s mute"},//5
	{"Audio spdif mute"},
	{"Hardware resample enable"},//8
	{"AMP Master Volume"},
	{"AMP Ch1 Volume"},//10
	{"AMP Ch2 Volume"},
	{"AMP Ch3 Volume"},
	{"AMP Ch1 Switch"},
	{"AMP Ch2 Switch"},
	{"AMP Ch3 Switch"},//15
	{"AMP EQ Mode"},
	{"PCM Playback Volume"},
	{"PCM Playback Switch"},
	{"Output Swap"},
	{"AudioIn Switch"},//19
};


char *CAudioAlsa::GetAlsaControlName(int get_type)
{
	if (GetAudioArchitectureTypeCFG() == CC_DAC_M6C_EXTERNAL_DAC_ON_BOARD) {
		/*if(get_type == CC_GET_ALSA_CTL_AUDIO_IN_SOURCE_NAME) {
		    return gAudioInSourceNameBuf;
		} else if(get_type == CC_GET_ALSA_CTL_INTERNAL_DAC_LEFT_LINEIN_SEL_NAME) {
		    strcpy(gInternalDacLeftLineInSelNameBuf, "Left LINEIN Select");
		    return gInternalDacLeftLineInSelNameBuf;
		} else if(get_type == CC_GET_ALSA_CTL_INTERNAL_DAC_RIGHT_LINEIN_SEL_NAME) {
		    strcpy(gInternalDacRightLineInSelNameBuf, "Right LINEIN Select");
		    return gInternalDacRightLineInSelNameBuf;
		} else if (get_type == CC_GET_ALSA_CTL_INTERNAL_DAC_LINEIN_CAPTURE_VOLUME_NAME) {
		    strcpy(gInternalDacLineInCaptureVolumeNameBuf, "LINEIN Capture Volume");
		    return gInternalDacLineInCaptureVolumeNameBuf;
		} else if (get_type == CC_GET_ALSA_CTL_INTERNAL_DAC_LINEOUT_PLAYBACK_VOLUME_NAME) {
		    strcpy(gInternalDacLineOutCaptureVolumeNameBuf, "LINEOUT Playback Volume");
		    return gInternalDacLineOutCaptureVolumeNameBuf;
		} else if(get_type == CC_GET_ALSA_CTL_I2S_MUTE_NAME) {
		    strcpy(gI2SMuteNameBuf, "Audio i2s mute");
		    return gI2SMuteNameBuf;
		} else if(get_type == CC_GET_ALSA_CTL_SPDIF_MUTE_NAME) {
		    strcpy(gSpdifMuteNameBuf, "Audio spdif mute");
		    return gSpdifMuteNameBuf;
		}  else if(get_type == CC_GET_ALSA_CTL_HW_RESAMPLE_NAME) {
		    strcpy(gHWResampleNameBuf, "Hardware resample enable");
		    return gHWResampleNameBuf;
		} else if(get_type == CC_GET_ALSA_CTL_EXTERNAL_DAC_MASTER_VOLUME_NAME) {
		    strcpy(gExternalDacMasterVolumeNameBuf, "AMP Master Volume");
		    return gExternalDacMasterVolumeNameBuf;
		} else if(get_type == CC_GET_ALSA_CTL_EXTERNAL_DAC_CH1_VOLUME_NAME) {
		    strcpy(gExternalDacCh1VolumeNameBuf, "AMP Ch1 Volume");
		    return gExternalDacCh1VolumeNameBuf;
		} else if(get_type == CC_GET_ALSA_CTL_EXTERNAL_DAC_CH2_VOLUME_NAME) {
		    strcpy(gExternalDacCh2VolumeNameBuf, "AMP Ch2 Volume");
		    return gExternalDacCh2VolumeNameBuf;
		} else if(get_type == CC_GET_ALSA_CTL_EXTERNAL_DAC_CH3_VOLUME_NAME) {
		    strcpy(gExternalDacCh3VolumeNameBuf, "AMP Ch3 Volume");
		    return gExternalDacCh3VolumeNameBuf;
		} else if(get_type == CC_GET_ALSA_CTL_EXTERNAL_DAC_CH1_SWITCH_NAME) {
		    strcpy(gExternalDacCh1SwitchNameBuf, "AMP Ch1 Switch");
		    return gExternalDacCh1SwitchNameBuf;
		} else if(get_type == CC_GET_ALSA_CTL_EXTERNAL_DAC_CH2_SWITCH_NAME) {
		    strcpy(gExternalDacCh2SwitchNameBuf, "AMP Ch2 Switch");
		    return gExternalDacCh2SwitchNameBuf;
		} else if(get_type == CC_GET_ALSA_CTL_EXTERNAL_DAC_CH3_SWITCH_NAME) {
		    strcpy(gExternalDacCh3SwitchNameBuf, "AMP Ch3 Switch");
		    return gExternalDacCh3SwitchNameBuf;
		} else if(get_type == CC_GET_ALSA_CTL_EXTERNAL_DAC_EQ_MODE_NAME) {
		    strcpy(gExternalDacEQModeNameBuf, "AMP EQ Mode");
		    return gExternalDacEQModeNameBuf;
		}*/
	} else {
		return gG9AlsaNames[get_type];
	}

	return NULL;
}

int gAudioInSource = -1;

int CAudioAlsa::SetAudioInSource(int source_type)
{
	int set_val = 0;
	char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_AUDIO_IN_SOURCE_NAME);

	if (GetAudioArchitectureTypeCFG() == CC_DAC_M6C_EXTERNAL_DAC_ON_BOARD) {
		if (source_type != 0 && source_type != 1) {
			return -1;
		}

		if (source_type == CC_AUDIO_IN_SOURCE_LINEIN) {
			set_val = 0;
		} else if (source_type == CC_AUDIO_IN_SOURCE_HDMI) {
			set_val = 1;
		}
	} else {
		if (source_type != 0 && source_type != 1 && source_type != 2) {
			return -1;
		}

		set_val = source_type;
	}

	gAudioInSource = set_val;

	return AudioControlSetValue(1, &set_val, match_names);
}

int CAudioAlsa::GetAudioInSource(void)
{
	char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_AUDIO_IN_SOURCE_NAME);
	int source_type = 0;

	source_type = gAudioInSource;
	if (source_type == -1) {
		AudioControlGetValue(1, &source_type, match_names);
	}

	return source_type;
}

int CAudioAlsa::SetInternalDacLineInSelectChannel(int line_in_number)
{
	int tmp_ret = 0;
	char *match_names = NULL;

	if (line_in_number < 0 || line_in_number > 7) {
		return -1;
	}

	match_names = GetAlsaControlName(CC_GET_ALSA_CTL_INTERNAL_DAC_LEFT_LINEIN_SEL_NAME);
	tmp_ret |= AudioControlSetValue(1, &line_in_number, match_names);

	match_names = GetAlsaControlName(CC_GET_ALSA_CTL_INTERNAL_DAC_RIGHT_LINEIN_SEL_NAME);
	tmp_ret |= AudioControlSetValue(1, &line_in_number, match_names);

	return tmp_ret;
}

int CAudioAlsa::SetInternalDacLineInCaptureVolume(int l_vol, int r_vol)
{
	int max_val = GetLineInMaxVol();
	char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_INTERNAL_DAC_LINEIN_CAPTURE_VOLUME_NAME);
	int VolumeBuf[2];

	VolumeBuf[0] = CheckVolume(l_vol, 0, max_val, 0, max_val);
	VolumeBuf[1] = CheckVolume(r_vol, 0, max_val, 0, max_val);

	return AudioControlSetValue(2, VolumeBuf, match_names);
}

int CAudioAlsa::GetInternalDacLineInCaptureVolume(int vol_buf[])
{
	int max_val = GetLineInMaxVol();
	char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_INTERNAL_DAC_LINEIN_CAPTURE_VOLUME_NAME);

	return GetTwoChannelVolume(vol_buf, 0, max_val, 0, max_val, match_names, 0, max_val);
}

int CAudioAlsa::SetInternalDacLineOutPlayBackVolume(int l_vol, int r_vol)
{
	int max_val = GetLineOutMaxVol();
	char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_INTERNAL_DAC_LINEOUT_PLAYBACK_VOLUME_NAME);
	int VolumeBuf[2];

	VolumeBuf[0] = CheckVolume(l_vol, 0, max_val, 0, max_val);
	VolumeBuf[1] = CheckVolume(r_vol, 0, max_val, 0, max_val);

	return AudioControlSetValue(2, VolumeBuf, match_names);
}

int CAudioAlsa::GetInternalDacLineOutPlayBackVolume(int vol_buf[])
{
	int max_val = GetLineOutMaxVol();
	char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_INTERNAL_DAC_LINEOUT_PLAYBACK_VOLUME_NAME);

	return GetTwoChannelVolume(vol_buf, 0, max_val, 0, max_val, match_names, 0, max_val);
}

int CAudioAlsa::SetAudioPcmPlaybackVolume(int vol)
{
	if(get_USB_Audio_card() == -EINVAL) return 0;
	char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_PCM_PLAYBACK_VOLUME);
	return HandleUSBAudioControlValue(1, &vol, 1, &match_names);
}

int CAudioAlsa::SetAudioPcmPlaybackSwitch(int vol)
{
	if(get_USB_Audio_card() == -EINVAL) return 0;
	char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_PCM_PLAYBACK_SWITCH);
	return HandleUSBAudioControlValue(1, &vol, 1, &match_names);
}

int CAudioAlsa::SetExternalDacChannelSwitch(int chan_ind, int switch_val)
{
	char *match_names = NULL;

	if (chan_ind == 1) {
		match_names = GetAlsaControlName(CC_GET_ALSA_CTL_EXTERNAL_DAC_CH1_SWITCH_NAME);
	} else if (chan_ind == 2) {
		match_names = GetAlsaControlName(CC_GET_ALSA_CTL_EXTERNAL_DAC_CH2_SWITCH_NAME);
	} else if (chan_ind == 3) {
		match_names = GetAlsaControlName(CC_GET_ALSA_CTL_EXTERNAL_DAC_CH3_SWITCH_NAME);
	} else {
		return -1;
	}

	return AudioControlSetValue(1, &switch_val, match_names);
}

int CAudioAlsa::SetExternalDacChannelVolume(int chan_ind, int main_vol)
{
	int tmp_ret = 0;
	char *match_names = NULL;

	if (chan_ind == 0) {
		match_names = GetAlsaControlName(CC_GET_ALSA_CTL_EXTERNAL_DAC_MASTER_VOLUME_NAME);
	} else if (chan_ind == 1) {
		match_names = GetAlsaControlName(CC_GET_ALSA_CTL_EXTERNAL_DAC_CH1_VOLUME_NAME);
	} else if (chan_ind == 2) {
		match_names = GetAlsaControlName(CC_GET_ALSA_CTL_EXTERNAL_DAC_CH2_VOLUME_NAME);
	} else if (chan_ind == 3) {
		match_names = GetAlsaControlName(CC_GET_ALSA_CTL_EXTERNAL_DAC_CH3_VOLUME_NAME);
	} else {
		return -1;
	}

	return AudioControlSetValue(1, &main_vol, match_names);
}
int CAudioAlsa::SetAudioSwitchIO(int value)
{
	char *match_names = gG9AlsaNames[CC_GET_ALSA_CTL_AUDIO_IN_SWITCH];

	return AudioControlSetValue(1, &value, match_names);
}
int CAudioAlsa::SetOutput_Swap(int value)
{
	char *match_names = gG9AlsaNames[CC_GET_ALSA_CTL_SETOUTPUT_SWAP];

	return AudioControlSetValue(1, &value, match_names);
}

int CAudioAlsa::SetExternalDacEQMode(int mode_val)
{
	char *match_names = NULL;

	match_names = GetAlsaControlName(CC_GET_ALSA_CTL_EXTERNAL_DAC_EQ_MODE_NAME);
	return AudioControlSetValue(1, &mode_val, match_names);
}

int CAudioAlsa::SetI2SMute(int mute_status)
{
	char *match_names = NULL;

	if (mute_status != CC_I2S_MUTE_ON && mute_status != CC_I2S_MUTE_OFF) {
		LOGE("%s, I2S mute value (%d) error!\n", __FUNCTION__, mute_status);
		return -1;
	}

	match_names = GetAlsaControlName(CC_GET_ALSA_CTL_I2S_MUTE_NAME);
	return AudioControlSetValue(1, &mute_status, match_names);
}

int CAudioAlsa::SetSPDIFMute(int mute_status)
{
	char *match_names = NULL;

	match_names = GetAlsaControlName(CC_GET_ALSA_CTL_SPDIF_MUTE_NAME);
	return AudioControlSetValue(1, &mute_status, match_names);
}

#define CC_SAMPLE_BUF_SIZE    (32)

enum CC_HW_RESAMPLE_TYPE {
	CC_HW_RESAMPLE_DISABLE,
	CC_HW_RESAMPLE_48K,
	CC_HW_RESAMPLE_44K,
	CC_HW_RESAMPLE_32K,
};

int CAudioAlsa::SetHardwareResample(int sr)
{
	int i = 0, set_val = 0, tmp_val = 0;
	int diff_val = 0x7FFFFFFF, diff_ind = -1;
	char *match_names = NULL;
	int sample_buf[CC_SAMPLE_BUF_SIZE] = {48000, 44000, 32000, -1};

	if (sr < 0) {
		set_val = 0;
	} else {
		for (i = 0; i < CC_SAMPLE_BUF_SIZE; i++) {
			if (sample_buf[i] < 0) {
				break;
			}

			if (sample_buf[i] >= sr) {
				tmp_val = sample_buf[i] - sr;
			} else {
				tmp_val = sr - sample_buf[i];
			}

			if (tmp_val <= diff_val) {
				diff_val = tmp_val;
				diff_ind = i;
			}
		}

		if (diff_ind < 0) {
			set_val = 0;
		} else {
			set_val = diff_ind + 1;
		}
	}

	LOGD("%s, set_val = %d.\n", __FUNCTION__, set_val);
	match_names = GetAlsaControlName(CC_GET_ALSA_CTL_HW_RESAMPLE_NAME);
	return AudioControlSetValue(1, &set_val, match_names);
}

int CAudioAlsa::SetMixerBypassSwitch(int switch_val)
{
	char match_names[48] = "Output Mixer Bypass Switch" ;
	int ctl_buf[2];

	if (switch_val != 0 && switch_val != 1) {
		return -1;
	}

	ctl_buf[0] = switch_val;
	ctl_buf[1] = switch_val;

	return AudioControlSetValue(2, ctl_buf, match_names);
}

int CAudioAlsa::GetMixerBypassSwitch(void)
{
	char match_names[48] = "Output Mixer Bypass Switch" ;
	int ctl_buf[2];

	AudioControlSetValue(2, ctl_buf, match_names);

	return ctl_buf[0];
}

int CAudioAlsa::SetMixerDacSwitch(int switch_val)
{
	char match_names[48] = "Output Mixer DAC Switch" ;
	int ctl_buf[2];

	if (switch_val != 0 && switch_val != 1) {
		return -1;
	}

	ctl_buf[0] = switch_val;
	ctl_buf[1] = switch_val;

	return AudioControlSetValue(2, ctl_buf, match_names);
}

int CAudioAlsa::GetMixerDacSwitch(void)
{
	char *match_names = "Output Mixer DAC Switch" ;
	int ctl_buf[2];

	AudioControlGetValue(2, ctl_buf, match_names);

	return ctl_buf[0];
}

int CAudioAlsa::TransVolumeBarVolToDigitalVol(int digit_lut_buf[], int digit_vol,
		int digit_min, int digit_max, int hd_min, int hd_max)
{
	if (digit_vol >= CC_VOL_TRANS_LUT_BUF_SIZE) {
		return digit_lut_buf[CC_VOL_TRANS_LUT_BUF_SIZE - 1];
	} else if (digit_vol < 0) {
		return digit_lut_buf[CC_VOL_TRANS_LUT_BUF_SIZE - 1];
	}

	return digit_lut_buf[digit_vol];
}

int CAudioAlsa::TransDigitalVolToVolumeBarVol(int digit_lut_buf[], int hd_vol, int hd_min,
		int hd_max, int digit_min, int digit_max)
{
	int i;

	for (i = 0; i < CC_VOL_TRANS_LUT_BUF_SIZE; i++) {
		if (digit_lut_buf[i] == hd_vol)
			break;
	}

	if (i < CC_VOL_TRANS_LUT_BUF_SIZE)
		return i;

	for (i = 0; i < CC_VOL_TRANS_LUT_BUF_SIZE - 1; i++) {
		if (digit_lut_buf[i] > hd_vol && digit_lut_buf[i + 1] < hd_vol)
			break;
	}

	if (i < CC_VOL_TRANS_LUT_BUF_SIZE - 1)
		return i;

	return digit_max;
}

void CAudioAlsa::SetMainVolDigitLutBuf(int digit_lut_buf[])
{
	memcpy((void *) mMainDigitLutBuf, digit_lut_buf,
		   CC_VOL_TRANS_LUT_BUF_SIZE * sizeof(int));
}

void CAudioAlsa::SetSupperBassVolDigitLutBuf(int digit_lut_buf[])
{
	memcpy((void *) mSupperBassDigitLutBuf, digit_lut_buf,
		   CC_VOL_TRANS_LUT_BUF_SIZE * sizeof(int));
}

int CAudioAlsa::SetDacMute(int mute_state, int mute_type)
{
	int tmp_ret = 0;

	if (mute_type & CC_DAC_MUTE_TYPE_INTERNAL) {
		tmp_ret |= SetInternalDacMute(mute_state);
	}

	if (mute_type & CC_DAC_MUTE_TYPE_EXTERNAL) {
		tmp_ret |= SetExternalDacMute(mute_state);
		setAudioPcmPlaybackMute(mute_state);
	}

	return tmp_ret;
}

int CAudioAlsa::SetDacMainVolume(int main_vol)
{
	int tmp_ret = 0;
	int aud_arch_type = GetAudioArchitectureTypeCFG();

	if (aud_arch_type == CC_DAC_T866_INTERNAL_DAC) {
		tmp_ret = SetInternalDacMainVolume(main_vol);
	} else {
		tmp_ret = SetExternalDacMainVolume(main_vol);
	}

	return tmp_ret;
}

int CAudioAlsa::SetDacSupperBassVolume(int tmp_vol)
{
	int aud_arch_type = GetAudioArchitectureTypeCFG();

	if (aud_arch_type == CC_DAC_T866_INTERNAL_DAC) {
		return 0;
	}

	return SetExternalDacSupperBassVolume(tmp_vol);
}

int CAudioAlsa::SetDacEQMode(int mode_val)
{
	int aud_arch_type = GetAudioArchitectureTypeCFG();

	if (aud_arch_type == CC_DAC_T866_INTERNAL_DAC) {
		return 0;
	}

	return SetExternalDacEQ(mode_val);
}

int CAudioAlsa::SetBalanceValue(int balance_val)
{
	int tmp_val = 0;
	int aud_arch_type = GetAudioArchitectureTypeCFG();

	tmp_val = mMainVolumeBalanceVal;
	mMainVolumeBalanceVal = balance_val;

	if (aud_arch_type == CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_FBC) {
		SendCmdToOffBoardFBCExternalDac(AUDIO_CMD_SET_BALANCE, balance_val);
		return 0;
	} else if (aud_arch_type == CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_CUSTOMER_LIB) {
		SendCmdToOffBoardCustomerLibExternalDac(AUDIO_CMD_SET_BALANCE, balance_val);
		return 0;
	}

	return -1;
}

int CAudioAlsa::GetBalanceValue()
{
	int tmp_val = 0;
	tmp_val = mMainVolumeBalanceVal;
	return tmp_val;
}

int CAudioAlsa::SetMainVolumeGain(int gain_val)
{
	int tmp_val = 0;
	tmp_val = mMainVolumeGainVal;
	mMainVolumeGainVal = gain_val;
	return tmp_val;
}

int CAudioAlsa::GetMainVolumeGain()
{
	int tmp_val = 0;
	tmp_val = mMainVolumeGainVal;
	return tmp_val;
}

int CAudioAlsa::SetSupperBassVolumeGain(int gain_val)
{
	int tmp_val = 0;
	tmp_val = mSupperBassVolumeGainVal;
	mSupperBassVolumeGainVal = gain_val;
	return tmp_val;
}

int CAudioAlsa::GetSupperBassVolumeGain()
{
	int tmp_val = 0;
	tmp_val = mSupperBassVolumeGainVal;
	return tmp_val;
}

int CAudioAlsa::SetDacAudioSourceType(int source_type)
{
	int aud_arch_type = GetAudioArchitectureTypeCFG();

	if (aud_arch_type == CC_DAC_T866_EXTERNAL_DAC_ON_BOARD || aud_arch_type == CC_DAC_M6C_EXTERNAL_DAC_ON_BOARD) {
	} else if (aud_arch_type == CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_FBC) {
		SendCmdToOffBoardFBCExternalDac(AUDIO_CMD_SET_SOURCE, source_type);
	} else if (aud_arch_type == CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_CUSTOMER_LIB) {
		SendCmdToOffBoardCustomerLibExternalDac(AUDIO_CMD_SET_SOURCE, source_type);
	} else if (aud_arch_type == CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_DIGITAL) {
	}

	return 0;
}

int CAudioAlsa::CalculateBalanceVol(int max_vol, int balance_val, int vol_buf[])
{
	int bal_mid_vol = 0, bal_cal_len = 0;
	int tmp_val = 0;

	if (balance_val < CC_MIN_SOUND_BALANCE_VAL
			|| balance_val > CC_MAX_SOUND_BALANCE_VAL) {
		return -1;
	}

	vol_buf[0] = max_vol;
	vol_buf[1] = max_vol;

	bal_mid_vol = (CC_MIN_SOUND_BALANCE_VAL + CC_MAX_SOUND_BALANCE_VAL) / 2;
	bal_cal_len = (CC_MAX_SOUND_BALANCE_VAL - CC_MIN_SOUND_BALANCE_VAL) / 2;

	if (balance_val == bal_mid_vol) {
		LOGD(
			"%s, balance value = %d, bal_mid_vol = %d, vol_buf[0] = %d, vol_buf[1] = %d.\n",
			__FUNCTION__, balance_val, bal_mid_vol, vol_buf[0], vol_buf[1]);
		return 0;
	} else if (balance_val < bal_mid_vol) {
		vol_buf[1] = (bal_cal_len - (bal_mid_vol - balance_val)) * max_vol / bal_cal_len;
	} else if (balance_val > bal_mid_vol) {
		vol_buf[0] = (bal_cal_len - (balance_val - bal_mid_vol)) * max_vol / bal_cal_len;
	}

	if (GetAudioAmplifierBalanceExchangeCFG() != 0) {
		tmp_val = vol_buf[0];
		vol_buf[0] = vol_buf[1];
		vol_buf[1] = tmp_val;
	}

	LOGD(
		"%s, balance value = %d, bal_mid_vol = %d, vol_buf[0] = %d, vol_buf[1] = %d.\n",
		__FUNCTION__, balance_val, bal_mid_vol, vol_buf[0], vol_buf[1]);
	return 0;
}

int CAudioAlsa::SetExternalDacMute(int mute_state)
{
	int tmp_ret = 0;
	int aud_arch_type = GetAudioArchitectureTypeCFG();

	if (aud_arch_type == CC_DAC_T866_EXTERNAL_DAC_ON_BOARD || aud_arch_type == CC_DAC_M6C_EXTERNAL_DAC_ON_BOARD) {
		SetExternalDacChannelSwitch(1, mute_state);
		SetExternalDacChannelSwitch(2, mute_state);

		if (GetAudioSupperBassModuleDisableCFG() == 0) {
			SetExternalDacChannelSwitch(3, mute_state);
		}
	} else if (aud_arch_type == CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_FBC) {
		SendCmdToOffBoardFBCExternalDac(AUDIO_CMD_SET_MUTE, mute_state);
	} else if (aud_arch_type == CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_CUSTOMER_LIB) {
		SendCmdToOffBoardCustomerLibExternalDac(AUDIO_CMD_SET_MUTE, mute_state);
	} else if (aud_arch_type == CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_DIGITAL) {
		SetDigitalMute(mute_state);
	}

	return tmp_ret;
}

int CAudioAlsa::SetInternalDacMute(int mute_state)
{
	return 0;
}

int CAudioAlsa::setAudioPcmPlaybackMute(int mute_state)
{
	const char *value;
	char prop[256];
	value = config_get_str ( "TV", "tvin.2d4G.headset.en", "null" );
	property_get("audio.output.double_output", prop, "null");
	if ( strcmp ( value, "enable" ) == 0 && (strcmp(prop, "0") == 0 || strcmp(prop, "null") == 0)) {
		SetAudioPcmPlaybackSwitch(mute_state);
	}
	return 0;
}

int CAudioAlsa::SetExternalDacMainVolume(int main_vol)
{
	int tmp_ret = 0;
	int aud_arch_type = GetAudioArchitectureTypeCFG();

	if (aud_arch_type == CC_DAC_T866_EXTERNAL_DAC_ON_BOARD || aud_arch_type == CC_DAC_M6C_EXTERNAL_DAC_ON_BOARD) {
		int digit_vol = 0;
		int vol_gain_val = 0;
		int balance_val = 0, vol_buf[2] = {0, 0};

		//handle l&r channel volume for balance
		balance_val = GetBalanceValue();
		CalculateBalanceVol(255, balance_val, vol_buf);

		tmp_ret |= SetExternalDacChannelVolume(1, vol_buf[0]);
		tmp_ret |= SetExternalDacChannelVolume(2, vol_buf[1]);

		//handle master channel volume
		digit_vol = TransVolumeBarVolToDigitalVol(mMainDigitLutBuf, main_vol,
					0, 100, CC_MIN_DAC_VOLUME, CC_MAX_DAC_VOLUME);

		vol_gain_val = GetMainVolumeGain();
		digit_vol += vol_gain_val;
		if (digit_vol < CC_MIN_DAC_VOLUME) {
			digit_vol = CC_MIN_DAC_VOLUME;
		} else if (digit_vol > CC_MAX_DAC_VOLUME) {
			digit_vol = CC_MAX_DAC_VOLUME;
		}
		tmp_ret |= SetExternalDacChannelVolume(0, digit_vol);
	} else if (aud_arch_type == CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_FBC) {
		tmp_ret = SendCmdToOffBoardFBCExternalDac(AUDIO_CMD_SET_VOLUME_BAR, main_vol);
	} else if (aud_arch_type == CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_CUSTOMER_LIB) {
		tmp_ret = SendCmdToOffBoardCustomerLibExternalDac(AUDIO_CMD_SET_VOLUME_BAR, main_vol);
	} else if (aud_arch_type == CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_DIGITAL) {
		tmp_ret = SetDigitalMainVolume(main_vol);
	}

	return tmp_ret;
}

int CAudioAlsa::SetInternalDacMainVolume(int main_vol)
{
	int tmp_ret = 0, digit_vol = 0;
	int aud_arch_type = GetAudioArchitectureTypeCFG();

	if (aud_arch_type == CC_DAC_T866_INTERNAL_DAC) {
		digit_vol = TransVolumeBarVolToDigitalVol(mMainDigitLutBuf, main_vol,
					0, 100, CC_MIN_DAC_VOLUME, CC_MAX_DAC_VOLUME);

		if (digit_vol < CC_MIN_DAC_VOLUME) {
			digit_vol = CC_MIN_DAC_VOLUME;
		} else if (digit_vol > CC_MAX_DAC_VOLUME) {
			digit_vol = CC_MAX_DAC_VOLUME;
		}

		return SetInternalDacLineOutPlayBackVolume(digit_vol, digit_vol);
	}

	return 0;
}

int CAudioAlsa::SetExternalDacSupperBassVolume(int tmp_vol)
{
	int tmp_ret = 0;
	int aud_arch_type = GetAudioArchitectureTypeCFG();

	if (GetAudioSupperBassModuleDisableCFG() != 0) {
		return 0;
	}

	if (aud_arch_type == CC_DAC_T866_EXTERNAL_DAC_ON_BOARD || aud_arch_type == CC_DAC_M6C_EXTERNAL_DAC_ON_BOARD) {
		int digit_vol = 0;
		int vol_gain_val = 0;

		digit_vol = TransVolumeBarVolToDigitalVol(mMainDigitLutBuf, tmp_vol,
					0, 100, CC_MIN_DAC_SUB_WOOFER_VOLUME,
					CC_MAX_DAC_SUB_WOOFER_VOLUME);

		vol_gain_val = GetSupperBassVolumeGain();
		digit_vol += vol_gain_val;
		if (digit_vol < CC_MIN_DAC_SUB_WOOFER_VOLUME) {
			digit_vol = CC_MIN_DAC_SUB_WOOFER_VOLUME;
		} else if (digit_vol > CC_MAX_DAC_SUB_WOOFER_VOLUME) {
			digit_vol = CC_MAX_DAC_SUB_WOOFER_VOLUME;
		}

		tmp_ret = SetExternalDacChannelVolume(3, digit_vol);
	} else if (aud_arch_type == CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_FBC) {
		tmp_ret = SendCmdToOffBoardFBCExternalDac(AUDIO_CMD_SET_SUBCHANNEL_VOLUME, tmp_vol);
	} else if (aud_arch_type == CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_CUSTOMER_LIB) {
		tmp_ret = SendCmdToOffBoardCustomerLibExternalDac(AUDIO_CMD_SET_SUBCHANNEL_VOLUME, tmp_vol);
	}

	return tmp_ret;
}

int CAudioAlsa::SetExternalDacEQ(int mode_val)
{
	int tmp_ret = 0;
	int aud_arch_type = GetAudioArchitectureTypeCFG();

	if (aud_arch_type == CC_DAC_T866_EXTERNAL_DAC_ON_BOARD || aud_arch_type == CC_DAC_M6C_EXTERNAL_DAC_ON_BOARD) {
		SetExternalDacEQMode(mode_val);
	} else if (aud_arch_type == CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_FBC) {
		SendCmdToOffBoardFBCExternalDac(AUDIO_CMD_SET_EQ_MODE, mode_val);
	} else if (aud_arch_type == CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_CUSTOMER_LIB) {
		SendCmdToOffBoardCustomerLibExternalDac(AUDIO_CMD_SET_EQ_MODE, mode_val);
	}

	return tmp_ret;
}

int (*HandleDacMute)(int mute_state);
int (*HandleDacMainVolume)(int vol);
int (*HandleDacBalance)(int balance_val);
int (*HandleAudioSourceType)(int source_type);

static void *gExternalDacLibHandler = NULL;
int CAudioAlsa::LoadExternalDacLib(void)
{
	char *error;

	ALOGD("%s, entering...\n", __FUNCTION__);

	if (gExternalDacLibHandler != NULL) {
		return 0;
	}

	const char *config_value = config_get_str("TV", "audio.external.dac.libpath", "/system/lib/libdac.so");
	gExternalDacLibHandler = dlopen(config_value, RTLD_NOW);
	if (!gExternalDacLibHandler) {
		ALOGE("%s, failed to load external dac lib (%s)\n", __FUNCTION__, config_value);
		goto Error;
	}


	HandleDacMute = (int(*)(int))dlsym(gExternalDacLibHandler, "HandleDacMute");
	if (HandleDacMute == NULL) {
		ALOGE("%s, fail find fun HandleDacMute()\n", __FUNCTION__);
		goto Error;
	}
	HandleDacMainVolume = (int(*)(int))dlsym(gExternalDacLibHandler, "HandleDacMainVolume");
	if (HandleDacMainVolume == NULL) {
		ALOGE("%s, fail find fun HandleDacMainVolume()\n", __FUNCTION__);
		goto Error;
	}
	HandleDacBalance = (int(*)(int))dlsym(gExternalDacLibHandler, "HandleDacBalance");
	if (HandleDacBalance == NULL) {
		ALOGE("%s, fail find fun HandleDacBalance()\n", __FUNCTION__);
		goto Error;
	}
	HandleAudioSourceType = (int(*)(int))dlsym(gExternalDacLibHandler, "HandleAudioSourceType");
	if (HandleAudioSourceType == NULL) {
		ALOGE("%s, fail find fun HandleAudioSourceType()\n", __FUNCTION__);
		goto Error;
	}

	return 0;

Error: //
	HandleDacMute = NULL;
	HandleDacMainVolume = NULL;
	HandleDacBalance = NULL;
	HandleAudioSourceType = NULL;

	if (gExternalDacLibHandler != NULL) {
		dlclose(gExternalDacLibHandler);
		gExternalDacLibHandler = NULL;
	}
	return -1;
}

int CAudioAlsa::SendCmdToOffBoardCustomerLibExternalDac(int cmd, int para)
{
	LoadExternalDacLib();
	if (gExternalDacLibHandler != NULL) {
		if (cmd == AUDIO_CMD_SET_MUTE) {
			if (HandleDacMute != NULL) {
				LOGD("%s, call external dac lib HandleDacMute (para = %d).\n", __FUNCTION__, para);
				return (*HandleDacMute)(para);
			}
		} else if (cmd == AUDIO_CMD_SET_VOLUME_BAR) {
			if (HandleDacMainVolume != NULL) {
				LOGD("%s, call external dac lib HandleDacMainVolume (para = %d).\n", __FUNCTION__, para);
				return (*HandleDacMainVolume)(para);
			}
		} else if (cmd == AUDIO_CMD_SET_BALANCE) {
			if (HandleDacBalance != NULL) {
				LOGD("%s, call external dac lib HandleDacBalance (para = %d).\n", __FUNCTION__, para);
				return (*HandleDacBalance)(para);
			}
		} else if (cmd == AUDIO_CMD_SET_SOURCE) {
			if (HandleAudioSourceType != NULL) {
				LOGD("%s, call external dac lib HandleAudioSourceType (para = %d).\n", __FUNCTION__, para);
				return (*HandleAudioSourceType)(para);
			}
		}
	}

	return 0;
}

int CAudioAlsa::SendCmdToOffBoardFBCExternalDac(int cmd, int para)
{
	CFbcCommunication *pFBC = GetSingletonFBC();
	if (pFBC != NULL) {
		if (cmd == AUDIO_CMD_SET_MUTE) {
			int fbc_mute = 0;//0 is mute,1 is unmute
			if(para == CC_AUDIO_MUTE) {
				fbc_mute = 0;
			} else {
				fbc_mute = 1;
			}
			//int fbcIsBute;
			//pFBC->cfbc_Get_Mute(COMM_DEV_SERIAL, &fbcIsBute);
			//LOGD("fbc is mute = %d", fbcIsBute);
			return pFBC->cfbc_Set_Mute(COMM_DEV_SERIAL, fbc_mute);
		} else if (cmd == AUDIO_CMD_SET_VOLUME_BAR) {
			LOGD("%s, send AUDIO_CMD_SET_VOLUME_BAR (para = %d) to fbc.\n", __FUNCTION__, para);
			return pFBC->cfbc_Set_Volume_Bar(COMM_DEV_SERIAL, para);
		} else if (cmd == AUDIO_CMD_SET_BALANCE) {
			LOGD("%s, send AUDIO_CMD_SET_BALANCE (para = %d) to fbc.\n", __FUNCTION__, para);
			return pFBC->cfbc_Set_Balance(COMM_DEV_SERIAL, para);
		} else if (cmd == AUDIO_CMD_SET_SOURCE) {
			LOGD("%s, send AUDIO_CMD_SET_SOURCE (para = %d) to fbc.\n", __FUNCTION__, para);
			return pFBC->cfbc_Set_FBC_Audio_Source(COMM_DEV_SERIAL, para);
		}
	}

	return 0;
}

static int gDigitalMuteStatus = CC_AUDIO_UNMUTE;
static int gDigitalLeftVol = 0;
static int gDigitalRightVol = 0;

int CAudioAlsa::HandleDigitalVolume(int l_val, int r_val)
{
	amAudioSetLeftGain(l_val);
	amAudioSetRightGain(r_val);

	amAudioSetAndroidVolumeEnable(1);
	amAudioSetAndroidVolume(l_val, r_val);

	LOGD("%s, l_val = %d, r_val = %d.\n", __FUNCTION__, l_val, r_val);
	return 0;
}

int CAudioAlsa::HandleDigitalMute(int mute_status)
{
	if (mute_status == CC_AUDIO_MUTE) {
		HandleDigitalVolume(0, 0);
	} else if (mute_status == CC_AUDIO_UNMUTE) {
		HandleDigitalVolume(gDigitalLeftVol, gDigitalRightVol);
	}

	return 0;
}

int CAudioAlsa::SetDigitalMainVolume(int main_vol)
{
	int tmp_ret = 0, l_val = 0, r_val = 0;
	int balance_val = 0, vol_buf[2] = {0, 0};

	//handle l&r channel volume for balance
	balance_val = GetBalanceValue();
	CalculateBalanceVol(main_vol, balance_val, vol_buf);

	//handle master channel volume
	l_val = TransVolumeBarVolToDigitalVol(mMainDigitLutBuf, vol_buf[0],
										  0, 100, CC_MIN_DAC_VOLUME, CC_MAX_DAC_VOLUME);
	r_val = TransVolumeBarVolToDigitalVol(mMainDigitLutBuf, vol_buf[1],
										  0, 100, CC_MIN_DAC_VOLUME, CC_MAX_DAC_VOLUME);

	gDigitalLeftVol = l_val;
	gDigitalRightVol = r_val;
	if (gDigitalMuteStatus == CC_AUDIO_UNMUTE) {
		HandleDigitalVolume(l_val, r_val);
	}

	return tmp_ret;
}

int CAudioAlsa::SetDigitalMute(int mute_status)
{
	if (mute_status == CC_AUDIO_MUTE || mute_status == CC_AUDIO_UNMUTE) {
		gDigitalMuteStatus = mute_status;
		return HandleDigitalMute(mute_status);
	}

	return 0;
}

