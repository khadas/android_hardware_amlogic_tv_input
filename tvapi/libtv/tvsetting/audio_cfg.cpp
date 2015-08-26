#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <android/log.h>
#include <cutils/properties.h>

#include "../tvsetting/CTvSetting.h"
#include "../tvconfig/tvconfig.h"
#include "audio_cfg.h"

#define LOG_TAG "audio_cfg"
#include "CTvLog.h"

#define CS_AUDIO_AMP_MASTER_NOLINE_SWITCH_CFG_NAME "audio.amp.master.noline.switch"
#define CS_AUDIO_AMP_MASTER_NOLINE_POINT_CNT_CFG_NAME "audio.amp.master.noline.point.cnt"
#define CS_AUDIO_AMP_MASTER_NOLINE_POINT_IND_BUF_CFG_NAME "audio.amp.master.noline.point.ind.buf"
#define CS_AUDIO_AMP_MASTER_NOLINE_POINT_DATA_DEF_BUF_CFG_NAME "audio.amp.master.noline.point.data.defbuf"
#define CS_AUDIO_AMP_MASTER_NOLINE_POINT_IND_DEF_BUF_CFG_NAME "audio.amp.master.noline.point.ind.defbuf"
#define CS_AUDIO_AMP_MASTER_NOLINE_POINT_DATA_TV_BUF_CFG_NAME "audio.amp.master.noline.point.data.tv"
#define CS_AUDIO_AMP_MASTER_NOLINE_POINT_DATA_AV_BUF_CFG_NAME "audio.amp.master.noline.point.data.av"
#define CS_AUDIO_AMP_MASTER_NOLINE_POINT_DATA_COMP_BUF_CFG_NAME "audio.amp.master.noline.point.data.comp"
#define CS_AUDIO_AMP_MASTER_NOLINE_POINT_DATA_HDMI_BUF_CFG_NAME "audio.amp.master.noline.point.data.hdmi"
#define CS_AUDIO_AMP_MASTER_NOLINE_POINT_DATA_VGA_BUF_CFG_NAME "audio.amp.master.noline.point.data.vga"
#define CS_AUDIO_AMP_MASTER_NOLINE_POINT_DATA_MPEG_BUF_CFG_NAME "audio.amp.master.noline.point.data.mpeg"

#define CS_AUDIO_AMP_SUPPERBASS_NOLINE_SWITCH_CFG_NAME "audio.amp.supperbass.noline.switch"
#define CS_AUDIO_AMP_SUPPERBASS_NOLINE_POINT_CNT_CFG_NAME "audio.amp.supperbass.noline.point.cnt"
#define CS_AUDIO_AMP_SUPPERBASS_NOLINE_POINT_IND_BUF_CFG_NAME "audio.amp.supperbass.noline.point.ind.buf"
#define CS_AUDIO_AMP_SUPPERBASS_NOLINE_POINT_DATA_DEF_BUF_CFG_NAME "audio.amp.supperbass.noline.point.data.defbuf"
#define CS_AUDIO_AMP_SUPPERBASS_NOLINE_POINT_IND_DEF_BUF_CFG_NAME "audio.amp.supperbass.noline.point.ind.defbuf"

#define CS_AUDIO_AMP_BALANCE_EXCHG_CFG_NAME "audio.amp.balance.exchg"

#define CS_EQ_MOUDLE_DISABLE_CFG_NAME  "audio.eq.module.disable"
#define CS_EQ_USING_ANDROID_CFG_NAME  "audio.eq.using.android"
#define CS_EQ_PRESETBUF_CFG_NAME "audio.eq.presetbuf"
#define CS_EQ_GAIN_RANGE_CFG_NAME "audio.eq.gain.range"
#define CS_EQ_UI_GAIN_RANGE_CFG_NAME "audio.eq.uigain.range"
#define CS_EQ_AMP_MASTER_GAIN_CFG_NAME  "audio.eq.ampmaster.gain"
#define CS_EQ_AMP_SUPPERBASS_GAIN_CFG_NAME  "audio.eq.ampsupperbass.gain"

#define CS_AUDIO_AMP_MAINVOL_TV_LUTBUF_CFG_NAME "audio.amp.mainvol.tv.lutbuf"
#define CS_AUDIO_AMP_MAINVOL_AV_LUTBUF_CFG_NAME "audio.amp.mainvol.av.lutbuf"
#define CS_AUDIO_AMP_MAINVOL_COMP_LUTBUF_CFG_NAME "audio.amp.mainvol.comp.lutbuf"
#define CS_AUDIO_AMP_MAINVOL_HDMI_LUTBUF_CFG_NAME "audio.amp.mainvol.hdmi.lutbuf"
#define CS_AUDIO_AMP_MAINVOL_VGA_LUTBUF_CFG_NAME "audio.amp.mainvol.vga.lutbuf"
#define CS_AUDIO_AMP_MAINVOL_MPEG_LUTBUF_CFG_NAME "audio.amp.mainvol.mpeg.lutbuf"

#define CS_AUDIO_AMP_SUPPERBASSVOL_TV_LUTBUF_CFG_NAME "audio.amp.supbassvol.tv.lutbuf"
#define CS_AUDIO_AMP_SUPPERBASSVOL_AV_LUTBUF_CFG_NAME "audio.amp.supbassvol.av.lutbuf"
#define CS_AUDIO_AMP_SUPPERBASSVOL_COMP_LUTBUF_CFG_NAME "audio.amp.supbassvol.comp.lutbuf"
#define CS_AUDIO_AMP_SUPPERBASSVOL_HDMI_LUTBUF_CFG_NAME "audio.amp.supbassvol.hdmi.lutbuf"
#define CS_AUDIO_AMP_SUPPERBASSVOL_VGA_LUTBUF_CFG_NAME "audio.amp.supbassvol.vga.lutbuf"
#define CS_AUDIO_AMP_SUPPERBASSVOL_MPEG_LUTBUF_CFG_NAME "audio.amp.supbassvol.mpeg.lutbuf"

#define CS_SRS_MOUDLE_DISABLE_CFG_NAME  "audio.srs.module.disable"
#define CS_SRS_SOURROUND_DISABLE_CFG_NAME  "audio.srs.sourround.disable"
#define CS_SRS_TRUBASS_DISABLE_CFG_NAME  "audio.srs.trubass.disable"
#define CS_SRS_DIALOGCLARITY_DISABLE_CFG_NAME  "audio.srs.dialogclarity.disable"
#define CS_SRS_SOURROUND_GAIN_CFG_NAME  "audio.srs.sourround.gain"
#define CS_SRS_SOURROUND_AMP_MASTER_GAIN_CFG_NAME  "audio.srs.sourround.ampmaster.gain"
#define CS_SRS_SOURROUND_AMP_SUPPERBASS_GAIN_CFG_NAME  "audio.srs.sourround.ampsupperbass.gain"
#define CS_SRS_TRUBASS_ONLY_GAIN_CFG_NAME  "audio.srs.trubass.only.gain"
#define CS_SRS_TRUBASS_ONLY_AMP_MASTER_GAIN_CFG_NAME  "audio.srs.trubass.only.ampmaster.gain"
#define CS_SRS_TRUBASS_ONLY_AMP_SUPPERBASS_GAIN_CFG_NAME  "audio.srs.trubass.only.ampsupperbass.gain"
#define CS_SRS_DIALOGCLARITY_ONLY_GAIN_CFG_NAME  "audio.srs.dialogclarity.only.gain"
#define CS_SRS_DIALOGCLARITY_ONLY_AMP_MASTER_GAIN_CFG_NAME  "audio.srs.dialogclarity.only.ampmaster.gain"
#define CS_SRS_DIALOGCLARITY_ONLY_AMP_SUPPERBASS_GAIN_CFG_NAME  "audio.srs.dialogclarity.only.ampsupperbass.gain"
#define CS_SRS_DIALOGCLARITY_ONLY_DEFINITION_GAIN_CFG_NAME  "audio.srs.dialogclarity.only.definition.gain"
#define CS_SRS_TRUBASS_DIALOGCLARITY_TRUBASS_GAIN_CFG_NAME  "audio.srs.turbass.dialogclarity.trubass.gain"
#define CS_SRS_TRUBASS_DIALOGCLARITY_DIALOGCLARITY_GAIN_CFG_NAME  "audio.srs.turbass.dialogclarity.dialogclarity.gain"
#define CS_SRS_TRUBASS_DIALOGCLARITY_DEFINITION_GAIN_CFG_NAME  "audio.srs.turbass.dialogclarity.definition.gain"
#define CS_SRS_TRUBASS_DIALOGCLARITY_AMP_MASTER_GAIN_CFG_NAME  "audio.srs.turbass.dialogclarity.ampmaster.gain"
#define CS_SRS_TRUBASS_DIALOGCLARITY_AMP_SUPPERBASS_GAIN_CFG_NAME  "audio.srs.turbass.dialogclarity.ampsupperbass.gain"
#define CS_SRS_SUPPERBASS_ON_TRUBASS_SPEAKER_SIZE_CFG_NAME  "audio.srs.supperbasson.turbass.speakersize"
#define CS_SRS_SUPPERBASS_OFF_TRUBASS_SPEAKER_SIZE_CFG_NAME  "audio.srs.supperbassoff.turbass.speakersize"

#define CS_AUDIO_DUMP_DATA_ENABLE_CFG_NAME  "audio.dump.data.en"
#define CS_KEY_MUTE_CFG_NAME  "audio.mute.keymute"
#define CS_AVOUT_INIT_MUTE_CFG_NAME "audio.avout.init.mute"
#define CS_SUPPERBASS_MODULE_DISABLE_CFG_NAME "audio.supperbass.module.disable"
#define CS_SUPPERBASS_SWITCH_DISABLE_CFG_NAME "audio.supperbass.switch.disable"
#define CS_DVI_SUPPORT_ENABLE_CFG_NAME "audio.dvi.support.enable"
#define CS_SRC_INPUT_TV_LINEIN_VOL_CFG_NAME  "audio.srcinput.linein.tv.vol"
#define CS_SRC_INPUT_AV_LINEIN_VOL_CFG_NAME  "audio.srcinput.linein.av.vol"
#define CS_SRC_INPUT_COMP_LINEIN_VOL_CFG_NAME  "audio.srcinput.linein.comp.vol"
#define CS_SRC_INPUT_HDMI_LINEIN_VOL_CFG_NAME  "audio.srcinput.linein.hdmi.vol"
#define CS_SRC_INPUT_VGA_LINEIN_VOL_CFG_NAME  "audio.srcinput.linein.vga.vol"
#define CS_SRC_INPUT_MPEG_LINEIN_VOL_CFG_NAME  "audio.srcinput.linein.mpeg.vol"
#define CS_SRC_INPUT_TV_LINEOUT_VOL_CFG_NAME  "audio.srcinput.lineout.tv.vol"
#define CS_SRC_INPUT_AV_LINEOUT_VOL_CFG_NAME  "audio.srcinput.lineout.av.vol"
#define CS_SRC_INPUT_COMP_LINEOUT_VOL_CFG_NAME  "audio.srcinput.lineout.comp.vol"
#define CS_SRC_INPUT_HDMI_LINEOUT_VOL_CFG_NAME  "audio.srcinput.lineout.hdmi.vol"
#define CS_SRC_INPUT_VGA_LINEOUT_VOL_CFG_NAME  "audio.srcinput.lineout.vga.vol"
#define CS_SRC_INPUT_MPEG_LINEOUT_VOL_CFG_NAME  "audio.srcinput.lineout.mpeg.vol"
#define CS_SRC_INPUT_MPEG_LINEOUT_VOL_CFG_NAME_OLD  "audio.srcinput.lineout.vol"

#define CS_AMP_MAINVOL_LUTBUF_COMPENSATION_TV_CFG_NAME "audio.amp.mainvol.lutbuf.compensation.tv"
#define CS_AMP_MAINVOL_LUTBUF_COMPENSATION_AV_CFG_NAME "audio.amp.mainvol.lutbuf.compensation.av"
#define CS_AMP_MAINVOL_LUTBUF_COMPENSATION_COMP_CFG_NAME "audio.amp.mainvol.lutbuf.compensation.comp"
#define CS_AMP_MAINVOL_LUTBUF_COMPENSATION_HDMI_CFG_NAME "audio.amp.mainvol.lutbuf.compensation.hdmi"
#define CS_AMP_MAINVOL_LUTBUF_COMPENSATION_VGA_CFG_NAME "audio.amp.mainvol.lutbuf.compensation.vga"
#define CS_AMP_MAINVOL_LUTBUF_COMPENSATION_MPEG_CFG_NAME "audio.amp.mainvol.lutbuf.compensation.mpeg"
#define CS_AMP_MAINVOL_LUTBUF_COMPENSATION_DTV_CFG_NAME "audio.amp.mainvol.lutbuf.compensation.dtv"

#define CS_AUDIO_NOISE_GATE_ENABLE_CFG_NAME  "audio.noise.gate.enable"
#define CS_AUDIO_NOISE_GATE_THRESHOLD_CFG_NAME  "audio.noise.gate.threshold"
#define CS_AUDIO_NOISE_GATE_UPPER_BOUND_CFG_NAME  "audio.noise.gate.upper.bound"

#define CS_DISABLE_RESET_SYSTEM_FRAMESIZE_CFG_NAME "audio.reset.system.framesize.disable"
#define CS_AUDIO_SYSTEM_FRAMESIZE_CFG_NAME "audio.reset.system.framesize"

#define CS_AUDIO_ARCHITECTURE_TYPE_CFG_NAME "audio.architecture.type"
#define CS_AUDIO_RESAMPLE_TYPE_CFG_NAME "audio.resample.type"


#define CC_MAX_SUP_LUT_CFG_INDEX        (32)

static int gMasterNoLinePointsIndexBuffer[CC_NO_LINE_POINTS_MAX_CNT] = { -1 };
static int gMasterNoLinePointsDefDataBuffer[CC_NO_LINE_POINTS_MAX_CNT] = { -1 };
static int gMasterNoLinePointsDefIndexBuffer[CC_NO_LINE_POINTS_MAX_CNT] = { -1 };

static int gSupperBassNoLinePointsIndexBuffer[CC_NO_LINE_POINTS_MAX_CNT] =
{ -1 };
static int gSupperBassNoLinePointsDataBuffer[CC_NO_LINE_POINTS_MAX_CNT] = { -1 };
static int gSupperBassNoLinePointsDefDataBuffer[CC_NO_LINE_POINTS_MAX_CNT] = {
    -1
};
static int gSupperBassNoLinePointsDefIndexBuffer[CC_NO_LINE_POINTS_MAX_CNT] = {
    -1
};

static int gTVMasterNoLinePointsDataBuffer[CC_NO_LINE_POINTS_MAX_CNT] = { -1 };
static int gAVMasterNoLinePointsDataBuffer[CC_NO_LINE_POINTS_MAX_CNT] = { -1 };
static int gCOMPMasterNoLinePointsDataBuffer[CC_NO_LINE_POINTS_MAX_CNT] = { -1 };
static int gHDMIMasterNoLinePointsDataBuffer[CC_NO_LINE_POINTS_MAX_CNT] = { -1 };
static int gVGAMasterNoLinePointsDataBuffer[CC_NO_LINE_POINTS_MAX_CNT] = { -1 };
static int gMPEGMasterNoLinePointsDataBuffer[CC_NO_LINE_POINTS_MAX_CNT] = { -1 };
static int gHDMI4K2KMasterNoLinePointsDataBuffer[CC_NO_LINE_POINTS_MAX_CNT] = {
    -1
};
static int gUSB4K2KMasterNoLinePointsDataBuffer[CC_NO_LINE_POINTS_MAX_CNT] = {
    -1
};

static int mTVMainVolumeDigitLutBuf_XX[CC_LUT_BUF_SIZE];
static int mAVMainVolumeDigitLutBuf_XX[CC_LUT_BUF_SIZE];
static int mCOMPMainVolumeDigitLutBuf_XX[CC_LUT_BUF_SIZE];
static int mHDMIMainVolumeDigitLutBuf_XX[CC_LUT_BUF_SIZE];
static int mVGAMainVolumeDigitLutBuf_XX[CC_LUT_BUF_SIZE];
static int mMPEGMainVolumeDigitLutBuf_XX[CC_LUT_BUF_SIZE];

static int mTVSupperBassDigitLutBuf_XX[CC_LUT_BUF_SIZE];
static int mAVSupperBassDigitLutBuf_XX[CC_LUT_BUF_SIZE];
static int mCOMPSupperBassDigitLutBuf_XX[CC_LUT_BUF_SIZE];
static int mHDMISupperBassDigitLutBuf_XX[CC_LUT_BUF_SIZE];
static int mVGASupperBassDigitLutBuf_XX[CC_LUT_BUF_SIZE];
static int mMPEGSupperBassDigitLutBuf_XX[CC_LUT_BUF_SIZE];

static int mEQPresetValueBuf_XX[CC_EQ_PRESET_BUF_SIZE];
static int mEQPresetDefBuf[CC_EQ_PRESET_BUF_SIZE] = {
    //
    50, 50, 50, 50, 50, 50, // SM_STD
    70, 60, 50, 60, 70, 50, // SM_MUSIC
    25, 50, 70, 66, 25, 50, // SM_NEWS
    75, 65, 50, 65, 75, 50, // SM_THEATER
};

static int GetAudioCfgData(const char *key_str, int *buf_item_count, int radix,
                           int data_buf[])
{
    int cfg_item_count = 0;
    char *token = NULL;
    const char *strDelimit = ",";
    const char *config_value;
    char data_str[CC_CFG_VALUE_STR_MAX_LEN] = { 0 };

    config_value = config_get_str("TV", key_str, "null");
    if (strcasecmp(config_value, "null") == 0) {
        //LOGE("%s, can't get config \"%s\"!!!\n", "TV", key_str);
        return -1;
    }

    cfg_item_count = 0;

    memset((void *)data_str, 0, sizeof(data_str));
    strncpy(data_str, config_value, sizeof(data_str) - 1);
    token = strtok(data_str, strDelimit);
    while (token != NULL) {
        if (cfg_item_count < *buf_item_count) {
            data_buf[cfg_item_count] = strtol(token, NULL, radix);

            token = strtok(NULL, strDelimit);
            cfg_item_count += 1;
        } else {
            LOGE("%s, we get data count more than desire count (%d)!!!\n",
                 __FUNCTION__ , *buf_item_count);
            return -1;
        }
    }

    *buf_item_count = cfg_item_count;

    return 0;
}

static int GetAudioCfgBufferData(const char *key_str, int *bufs_count,
                                 int *buf_item_count, int radix, int data_buf[])
{
    int cfg_item_count = 0, tmpTotalItemCount = 0;
    char *token = NULL;
    const char *strDelimit = ",";
    const char *config_value;
    char data_str[CC_CFG_VALUE_STR_MAX_LEN] = { 0 };

    config_value = config_get_str("TV", key_str, "null");
    if (strcasecmp(config_value, "null") == 0) {
        //    LOGE("%s, can't get config \"%s\"!!!\n", "TV", key_str);
        *bufs_count = 0;
        *buf_item_count = 0;
        return -1;
    }

    tmpTotalItemCount = 0;

    memset((void *)data_str, 0, sizeof(data_str));
    strncpy(data_str, config_value, sizeof(data_str) - 1);
    token = strtok(data_str, strDelimit);
    while (token != NULL) {
        if (cfg_item_count == 0) {
            *bufs_count = strtol(token, NULL, radix);
        } else if (cfg_item_count == 1) {
            *buf_item_count = strtol(token, NULL, radix);
        } else if (cfg_item_count >= 2) {
            data_buf[tmpTotalItemCount] = strtol(token, NULL, radix);
            tmpTotalItemCount += 1;
        }

        token = strtok(NULL, strDelimit);
        cfg_item_count += 1;
    }

    if ((*bufs_count) * (*buf_item_count) != tmpTotalItemCount) {
        LOGE("%s, get item count error!!! should be %d, real is %d.\n",
             __FUNCTION__, (*bufs_count) * (*buf_item_count),
             tmpTotalItemCount);
        *bufs_count = 0;
        *buf_item_count = 0;
        return -1;
    }

    return 0;
}

static int GetAudioCfgSingleIntData(const char *key_str, int def_val)
{
    const char *config_value;

    config_value = config_get_str("TV", key_str, "null");

    if (strcmp(config_value, "null") == 0) {
        return def_val;
    }

    return strtol(config_value, NULL, 10);
}

template<typename T>
static int CheckNoLinePoints(int point_cnt, T points_buf[])
{
    int i = 0, same_cnt = 0, tmp_direct = -1, error_flag = 0;

    if (point_cnt <= 1) {
        LOGD("%s, point is less than 2.\n", __FUNCTION__);
        return 0;
    }

    for (i = 0; i < point_cnt - 1; i++) {
        if (points_buf[i] < points_buf[i + 1]) {
            if (tmp_direct == -1) {
                tmp_direct = 0;
            } else if (tmp_direct == 1) {
                LOGD("%s, check have direct error.\n", __FUNCTION__);
                return -1;
            }
        } else if (points_buf[i] > points_buf[i + 1]) {
            if (tmp_direct == -1) {
                tmp_direct = 1;
            } else if (tmp_direct == 0) {
                LOGD("%s, check have direct error.\n", __FUNCTION__);
                return -1;
            }
        } else {
            same_cnt += 1;
        }
    }

    if (same_cnt == i) {
        LOGD("%s, check have same error.\n", __FUNCTION__);
        return -1;
    }

    LOGD("%s, check no error.\n", __FUNCTION__);
    return 0;
}

int *GetAudioAmplifierNoLinePointsIndexBuffer(int lut_buf_type, int get_type)
{
    if (lut_buf_type == CC_LUT_BUF_SUPPERBASS) {
        return GetAudioAmplifierSupperBassNoLinePointsIndexBuffer(get_type);
    }

    return NULL;
}

int *GetAudioAmplifierNoLinePointsDataBuffer(int lut_buf_type, int get_type)
{
    if (lut_buf_type == CC_LUT_BUF_SUPPERBASS) {
        return GetAudioAmplifierSupperBassNoLinePointsDataBuffer(get_type);
    }

    return NULL;
}

int GetAudioAmplifierMasterNoLineSwitchFlag()
{
    const char *config_value;
    config_value = config_get_str("TV",
                                  CS_AUDIO_AMP_MASTER_NOLINE_SWITCH_CFG_NAME, "null");

    if (strcmp(config_value, "null") == 0) {
        return 0;
    }

    return strtol(config_value, NULL, 10);
}

int GetAudioAmplifierSupperBassNoLineSwitchFlag()
{
    const char *config_value;
    config_value = config_get_str("TV",
                                  CS_AUDIO_AMP_SUPPERBASS_NOLINE_SWITCH_CFG_NAME, "null");

    if (strcmp(config_value, "null") == 0) {
        return 0;
    }

    return strtol(config_value, NULL, 10);
}

int GetAudioAmplifierMasterNoLinePointsCount()
{
    const char *config_value;
    config_value = config_get_str("TV",
                                  CS_AUDIO_AMP_MASTER_NOLINE_POINT_CNT_CFG_NAME, "null");

    if (strcmp(config_value, "null") == 0) {
        return 11;
    }

    return strtol(config_value, NULL, 10);
}

int GetAudioAmplifierSupperBassNoLinePointsCount()
{
    const char *config_value;
    config_value = config_get_str("TV",
                                  CS_AUDIO_AMP_SUPPERBASS_NOLINE_POINT_CNT_CFG_NAME, "null");

    if (strcmp(config_value, "null") == 0) {
        return 5;
    }

    return strtol(config_value, NULL, 10);
}

static int GetAudioAmplifierNoLinePointsBuffer(const char *key_str,
        int buf_item_count, int data_buf[])
{
    int i = 0, tmp_item_count = 0;
    int tmp_buf[CC_NO_LINE_POINTS_MAX_CNT] = { 0 };

    //   if (data_buf[0] >= 0 && data_buf[0] <= 100) {
    //       LOGD("%s, we have get the index buffer.\n", "TV");
    //       return 0;
    //   }

    tmp_item_count = buf_item_count;
    if (GetAudioCfgData(key_str, &tmp_item_count, 10, tmp_buf) < 0) {
        LOGE("%s, get index buffer data error, set default.\n", __FUNCTION__);
        return -1;
    }

    if (tmp_item_count != buf_item_count) {
        LOGE(
            "%s, get index buffer data error(buf item count error), set default.\n",
            __FUNCTION__);
        return -1;
    }

    for (i = 0; i < buf_item_count; i++) {
        data_buf[i] = tmp_buf[i];
    }

    return 0;
}

int *GetAudioAmplifierSupperBassNoLinePointsIndexBuffer(int get_type)
{
    int buf_item_count = 0;

    buf_item_count = GetAudioAmplifierSupperBassNoLinePointsCount();
    if (GetAudioAmplifierNoLinePointsBuffer(
            CS_AUDIO_AMP_SUPPERBASS_NOLINE_POINT_IND_BUF_CFG_NAME,
            buf_item_count, gSupperBassNoLinePointsIndexBuffer) < 0) {
        return GetAudioAmplifierSupperBassNoLinePointsDefIndexBuffer();
    }

    if (CheckNoLinePoints(buf_item_count, gSupperBassNoLinePointsIndexBuffer)
        < 0) {
        return GetAudioAmplifierSupperBassNoLinePointsDefIndexBuffer();
    }

    return gSupperBassNoLinePointsIndexBuffer;
}

int *GetAudioAmplifierMasterNoLinePointsDataBuffer(int get_type)
{
    int i = 0, buf_item_count = 0, tmp_offset = 0;
    int *tmpPtr = NULL;
    unsigned char tmp_buf[CC_NO_LINE_POINTS_MAX_CNT] = { 0 };

    buf_item_count = GetAudioAmplifierMasterNoLinePointsCount();

    if (get_type == CC_GET_LUT_TV) {
        tmpPtr = gTVMasterNoLinePointsDataBuffer;
        tmp_offset = get_type * buf_item_count;
    } else if (get_type == CC_GET_LUT_AV) {
        tmpPtr = gAVMasterNoLinePointsDataBuffer;
        tmp_offset = get_type * buf_item_count;
    } else if (get_type == CC_GET_LUT_COMP) {
        tmpPtr = gCOMPMasterNoLinePointsDataBuffer;
        tmp_offset = get_type * buf_item_count;
    } else if (get_type == CC_GET_LUT_HDMI) {
        tmpPtr = gHDMIMasterNoLinePointsDataBuffer;
        tmp_offset = get_type * buf_item_count;
    } else if (get_type == CC_GET_LUT_VGA) {
        tmpPtr = gVGAMasterNoLinePointsDataBuffer;
        tmp_offset = get_type * buf_item_count;
    } else if (get_type == CC_GET_LUT_MPEG) {
        tmpPtr = gMPEGMasterNoLinePointsDataBuffer;
        tmp_offset = get_type * buf_item_count;
    }

    if (tmpPtr != NULL && tmpPtr[0] != -1) {
        return tmpPtr;
    }

    if (SSMReadAudioNoLinePoints(tmp_offset, buf_item_count, tmp_buf) < 0) {
        return GetAudioAmplifierMasterNoLinePointsDefDataBuffer(get_type);
    }

    if (CheckNoLinePoints(buf_item_count, tmp_buf) < 0) {
        return GetAudioAmplifierMasterNoLinePointsDefDataBuffer(get_type);
    }

    for (i = 0; i < buf_item_count; i++) {
        tmpPtr[i] = tmp_buf[i];
    }

    return tmpPtr;
}

int *GetAudioAmplifierSupperBassNoLinePointsDataBuffer(int get_type)
{
    int i = 0, buf_item_count = 0, tmp_offset = 0;
    int *tmpPtr = NULL;
    unsigned char tmp_buf[CC_NO_LINE_POINTS_MAX_CNT] = { 0 };

    buf_item_count = GetAudioAmplifierSupperBassNoLinePointsCount();

    tmpPtr = gSupperBassNoLinePointsDataBuffer;
    if (tmpPtr != NULL && tmpPtr[0] != -1) {
        return tmpPtr;
    }

    tmp_offset = CC_GET_TYPE_CNT * GetAudioAmplifierMasterNoLinePointsCount();

    if (SSMReadAudioNoLinePoints(tmp_offset, buf_item_count, tmp_buf) < 0) {
        return GetAudioAmplifierSupperBassNoLinePointsDefDataBuffer();
    }

    if (CheckNoLinePoints(buf_item_count, tmp_buf) < 0) {
        return GetAudioAmplifierSupperBassNoLinePointsDefDataBuffer();
    }

    for (i = 0; i < buf_item_count; i++) {
        tmpPtr[i] = tmp_buf[i];
    }

    return tmpPtr;
}

int *GetAudioAmplifierMasterNoLinePointsDefIndexBuffer()
{
    int i = 0, buf_item_count = 0;

    buf_item_count = GetAudioAmplifierMasterNoLinePointsCount();
    if (GetAudioAmplifierNoLinePointsBuffer(
            CS_AUDIO_AMP_MASTER_NOLINE_POINT_IND_DEF_BUF_CFG_NAME,
            buf_item_count, gMasterNoLinePointsDefIndexBuffer) < 0) {
        gMasterNoLinePointsDefIndexBuffer[0] = 1;

        for (i = 1; i <= 10; i++) {
            gMasterNoLinePointsDefIndexBuffer[i] = i * 10;
        }
    }

    if (CheckNoLinePoints(buf_item_count, gMasterNoLinePointsDefIndexBuffer)
        < 0) {
        gMasterNoLinePointsDefIndexBuffer[0] = 1;

        for (i = 1; i <= 10; i++) {
            gMasterNoLinePointsDefIndexBuffer[i] = i * 10;
        }
    }

    return gMasterNoLinePointsDefIndexBuffer;
}

int *GetAudioAmplifierSupperBassNoLinePointsDefIndexBuffer()
{
    int i = 0, buf_item_count = 0;

    buf_item_count = GetAudioAmplifierSupperBassNoLinePointsCount();
    if (GetAudioAmplifierNoLinePointsBuffer(
            CS_AUDIO_AMP_SUPPERBASS_NOLINE_POINT_IND_DEF_BUF_CFG_NAME,
            buf_item_count, gSupperBassNoLinePointsDefIndexBuffer) < 0) {
        gSupperBassNoLinePointsDefIndexBuffer[0] = 1;

        for (i = 1; i <= 4; i++) {
            gSupperBassNoLinePointsDefIndexBuffer[i] = i * 25;
        }
    }

    if (CheckNoLinePoints(buf_item_count, gSupperBassNoLinePointsDefIndexBuffer)
        < 0) {
        gSupperBassNoLinePointsDefIndexBuffer[0] = 1;

        for (i = 1; i <= 4; i++) {
            gSupperBassNoLinePointsDefIndexBuffer[i] = i * 25;
        }
    }

    return gSupperBassNoLinePointsDefIndexBuffer;
}

int *GetAudioAmplifierMasterNoLinePointsDefDataBuffer(int buf_type)
{
    int i = 0, buf_item_count = 0, point_buffer_flag = 0;

    buf_item_count = GetAudioAmplifierMasterNoLinePointsCount();

    switch (buf_type) {
    case CC_GET_LUT_TV:
        point_buffer_flag = GetAudioAmplifierNoLinePointsBuffer(
                                CS_AUDIO_AMP_MASTER_NOLINE_POINT_DATA_TV_BUF_CFG_NAME,
                                buf_item_count, gMasterNoLinePointsDefDataBuffer);
        break;
    case CC_GET_LUT_AV:
        point_buffer_flag = GetAudioAmplifierNoLinePointsBuffer(
                                CS_AUDIO_AMP_MASTER_NOLINE_POINT_DATA_AV_BUF_CFG_NAME,
                                buf_item_count, gMasterNoLinePointsDefDataBuffer);
        break;
    case CC_GET_LUT_COMP:
        point_buffer_flag = GetAudioAmplifierNoLinePointsBuffer(
                                CS_AUDIO_AMP_MASTER_NOLINE_POINT_DATA_COMP_BUF_CFG_NAME,
                                buf_item_count, gMasterNoLinePointsDefDataBuffer);
        break;
    case CC_GET_LUT_HDMI:
        point_buffer_flag = GetAudioAmplifierNoLinePointsBuffer(
                                CS_AUDIO_AMP_MASTER_NOLINE_POINT_DATA_HDMI_BUF_CFG_NAME,
                                buf_item_count, gMasterNoLinePointsDefDataBuffer);
        break;
    case CC_GET_LUT_VGA:
        point_buffer_flag = GetAudioAmplifierNoLinePointsBuffer(
                                CS_AUDIO_AMP_MASTER_NOLINE_POINT_DATA_VGA_BUF_CFG_NAME,
                                buf_item_count, gMasterNoLinePointsDefDataBuffer);
        break;
    case CC_GET_LUT_MPEG:
        point_buffer_flag = GetAudioAmplifierNoLinePointsBuffer(
                                CS_AUDIO_AMP_MASTER_NOLINE_POINT_DATA_MPEG_BUF_CFG_NAME,
                                buf_item_count, gMasterNoLinePointsDefDataBuffer);
        break;
    default:
        point_buffer_flag = -1;
        //    LOGE("buffer type is wrong ,please check input Parameter !");
        break;
    }

    if (point_buffer_flag < 0) {
        for (i = 0; i < buf_item_count; i++) {
            gMasterNoLinePointsDefDataBuffer[i] = 100 + i;
        }

        return gMasterNoLinePointsDefDataBuffer;
    }

    if (CheckNoLinePoints(buf_item_count, gMasterNoLinePointsDefDataBuffer)
        < 0) {
        for (i = 0; i < buf_item_count; i++) {
            gMasterNoLinePointsDefDataBuffer[i] = 100 + i;
        }
    }

    return gMasterNoLinePointsDefDataBuffer;
}

int *GetAudioAmplifierSupperBassNoLinePointsDefDataBuffer()
{
    int i = 0, buf_item_count = 0;

    buf_item_count = GetAudioAmplifierSupperBassNoLinePointsCount();
    if (GetAudioAmplifierNoLinePointsBuffer(
            CS_AUDIO_AMP_SUPPERBASS_NOLINE_POINT_DATA_DEF_BUF_CFG_NAME,
            buf_item_count, gSupperBassNoLinePointsDefDataBuffer) < 0) {
        for (i = 0; i < buf_item_count; i++) {
            gSupperBassNoLinePointsDefDataBuffer[i] = 100 + i;
        }
    }

    if (CheckNoLinePoints(buf_item_count, gSupperBassNoLinePointsDefDataBuffer)
        < 0) {
        for (i = 0; i < buf_item_count; i++) {
            gSupperBassNoLinePointsDefDataBuffer[i] = 100 + i;
        }
    }

    return gSupperBassNoLinePointsDefDataBuffer;
}

int GetAudioAmplifierBalanceExchangeCFG()
{
    const char *config_value;
    config_value = config_get_str("TV", CS_AUDIO_AMP_BALANCE_EXCHG_CFG_NAME,
                                  "null");
    if (strcmp(config_value, "null") == 0) {
        return 0; //return 0 to disable balance exchange
    }

    return strtol(config_value, NULL, 10);
}

int GetAudioMainVolLutBufCFGIndexModifyEnable()
{
    const char *config_value;

    config_value = config_get_str("TV", "audio.amp.master.cfgindex.mod.en",
                                  "null");

    if (strcmp(config_value, "null") == 0) {
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}

int GetAudioMainVolLutBufNameModifyUbootenvCFG()
{
    char config_value[PROPERTY_VALUE_MAX];

    memset(config_value, '\0', 32);

    property_get("ubootenv.var.ampindex", config_value, "null");
    if (strcasecmp(config_value, "null") == 0) {
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}

int GetAudioMainVolLutBufNameModifyFromUbootenv()
{
    const char *config_value;

    config_value = config_get_str("TV",
                                  "audio.amp.mainvol.lutbufname.mod.ubootenv", "null");

    if (strcmp(config_value, "null") == 0) {
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}

int GetAudioMainVolLutBufNameModifyEnableCFG()
{
    const char *config_value;

    config_value = config_get_str("TV", "audio.amp.mainvol.lutbufname.mod.en",
                                  "null");

    if (strcmp(config_value, "null") == 0) {
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}

int GetAudioMainVolLutBufNameCFG(int get_type, char name_buf[])
{
    if (get_type == CC_GET_LUT_TV) {
        strcpy(name_buf, "audio.amp.mainvol.tv.lb.name");
    } else if (get_type == CC_GET_LUT_AV) {
        strcpy(name_buf, "audio.amp.mainvol.av.lb.name");
    } else if (get_type == CC_GET_LUT_COMP) {
        strcpy(name_buf, "audio.amp.mainvol.comp.lb.name");
    } else if (get_type == CC_GET_LUT_HDMI) {
        strcpy(name_buf, "audio.amp.mainvol.hdmi.lb.name");
    } else if (get_type == CC_GET_LUT_VGA) {
        strcpy(name_buf, "audio.amp.mainvol.vga.lb.name");
    } else if (get_type == CC_GET_LUT_MPEG) {
        strcpy(name_buf, "audio.amp.mainvol.mpeg.lb.name");
    } else if (get_type == CC_GET_LUT_HDMI_4K2K) {
        strcpy(name_buf, "audio.amp.mainvol.hdmi4k2k.lb.name");
    } else if (get_type == CC_GET_LUT_USB_4K2K) {
        strcpy(name_buf, "audio.amp.mainvol.usb4k2k.lb.name");
    } else {
        return -1;
    }

    return 0;
}

static int GetAudioMainVolLutBufDataByModifyName(int get_type,
        int *VolumeDigitLutBuf_XX)
{
    int i = 0, buf_count = 0, buf_item_count = 0;
    char key_buf[128];
    const char *value_buf;

    memset(key_buf, '\0', 128);

    GetAudioMainVolLutBufNameCFG(get_type, key_buf);

    value_buf = config_get_str("TV", key_buf, (char *) "");

    if (strcmp(value_buf, "") != 0) {
        if (GetAudioCfgBufferData(value_buf, &buf_count, &buf_item_count, 10,
                                  VolumeDigitLutBuf_XX) == 0) {
            return 0;
        }
    }

    return -1;
}

int *GetAudioAmplifierMasterVolumeLUTBufferPtr(int get_type, int audio_std)
{
    int buf_count = 0, buf_item_count = 0;
    int name_en = GetAudioMainVolLutBufNameModifyEnableCFG();

    if (name_en == 1) {
        if (get_type == CC_GET_LUT_TV) {
            if (GetAudioMainVolLutBufDataByModifyName(get_type,
                    mTVMainVolumeDigitLutBuf_XX) == 0) {
                return mTVMainVolumeDigitLutBuf_XX;
            }
        } else if (get_type == CC_GET_LUT_AV) {
            if (GetAudioMainVolLutBufDataByModifyName(get_type,
                    mAVMainVolumeDigitLutBuf_XX) == 0) {
                return mAVMainVolumeDigitLutBuf_XX;
            }
        } else if (get_type == CC_GET_LUT_COMP) {
            if (GetAudioMainVolLutBufDataByModifyName(get_type,
                    mCOMPMainVolumeDigitLutBuf_XX) == 0) {
                return mCOMPMainVolumeDigitLutBuf_XX;
            }
        } else if (get_type == CC_GET_LUT_HDMI) {
            if (GetAudioMainVolLutBufDataByModifyName(get_type,
                    mHDMIMainVolumeDigitLutBuf_XX) == 0) {
                return mHDMIMainVolumeDigitLutBuf_XX;
            }
        } else if (get_type == CC_GET_LUT_VGA) {
            if (GetAudioMainVolLutBufDataByModifyName(get_type,
                    mVGAMainVolumeDigitLutBuf_XX) == 0) {
                return mVGAMainVolumeDigitLutBuf_XX;
            }
        } else if (get_type == CC_GET_LUT_MPEG) {
            if (GetAudioMainVolLutBufDataByModifyName(get_type,
                    mMPEGMainVolumeDigitLutBuf_XX) == 0) {
                return mMPEGMainVolumeDigitLutBuf_XX;
            }
        }
    } else {
        if (get_type == CC_GET_LUT_TV) {
            if (GetAudioCfgBufferData(CS_AUDIO_AMP_MAINVOL_TV_LUTBUF_CFG_NAME,
                                      &buf_count, &buf_item_count, 10,
                                      mTVMainVolumeDigitLutBuf_XX) == 0) {
                return mTVMainVolumeDigitLutBuf_XX;
            }
        } else if (get_type == CC_GET_LUT_AV) {
            if (GetAudioCfgBufferData(CS_AUDIO_AMP_MAINVOL_AV_LUTBUF_CFG_NAME,
                                      &buf_count, &buf_item_count, 10,
                                      mAVMainVolumeDigitLutBuf_XX) == 0) {
                return mAVMainVolumeDigitLutBuf_XX;
            }
        } else if (get_type == CC_GET_LUT_COMP) {
            if (GetAudioCfgBufferData(CS_AUDIO_AMP_MAINVOL_COMP_LUTBUF_CFG_NAME,
                                      &buf_count, &buf_item_count, 10,
                                      mCOMPMainVolumeDigitLutBuf_XX) == 0) {
                return mCOMPMainVolumeDigitLutBuf_XX;
            }
        } else if (get_type == CC_GET_LUT_HDMI) {
            if (GetAudioCfgBufferData(CS_AUDIO_AMP_MAINVOL_HDMI_LUTBUF_CFG_NAME,
                                      &buf_count, &buf_item_count, 10,
                                      mHDMIMainVolumeDigitLutBuf_XX) == 0) {
                return mHDMIMainVolumeDigitLutBuf_XX;
            }
        } else if (get_type == CC_GET_LUT_VGA) {
            if (GetAudioCfgBufferData(CS_AUDIO_AMP_MAINVOL_VGA_LUTBUF_CFG_NAME,
                                      &buf_count, &buf_item_count, 10,
                                      mVGAMainVolumeDigitLutBuf_XX) == 0) {
                return mVGAMainVolumeDigitLutBuf_XX;
            }
        } else if (get_type == CC_GET_LUT_MPEG) {
            if (GetAudioCfgBufferData(CS_AUDIO_AMP_MAINVOL_MPEG_LUTBUF_CFG_NAME,
                                      &buf_count, &buf_item_count, 10,
                                      mMPEGMainVolumeDigitLutBuf_XX) == 0) {
                return mMPEGMainVolumeDigitLutBuf_XX;
            }
        }
    }
    return NULL;
}

int GetAudioSupperBassVolLutBufCFGIndexModifyEnable()
{
    const char *config_value;
    config_value = config_get_str("TV", "audio.amp.supperbass.cfgindex.mod.en",
                                  "null");
    //LOGD("%s, get audio.amp.supperbass.cfgindex.mod.en is %s \n", "TV",
    //        config_value);

    if (strcmp(config_value, "null") == 0) {
        //    LOGD(
        //            "%s, get config  modify value \"%s\", return  audio.amp.supperbass.cfgindex.mod.en.\n",
        //            "TV", config_value);
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}

int GetAudioSupperBassVolLutBufNameModifyEnableCFG()
{
    const char *config_value;

    config_value = config_get_str("TV",
                                  "audio.amp.supbassvol.lutbufname.mod.en", "null");
    //LOGD("%s, audio.amp.supbassvol.lutbufname.mod.en is %s \n", "TV",
    //        config_value);

    if (strcmp(config_value, "null") == 0) {
        //    LOGD(
        //            "%s, get config  enable value \"%s\", return  0 to not enable moidfy lut buffer name.\n",
        //            "TV", config_value);
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}

int GetAudioSupperBassVolLutBufNameCFG(int get_type, char name_buf[])
{
    if (get_type == CC_GET_LUT_TV) {
        strcpy(name_buf, "audio.amp.supbassvol.tv.lb.name");
    } else if (get_type == CC_GET_LUT_AV) {
        strcpy(name_buf, "audio.amp.supbassvol.av.lb.name");
    } else if (get_type == CC_GET_LUT_COMP) {
        strcpy(name_buf, "audio.amp.supbassvol.comp.lb.name");
    } else if (get_type == CC_GET_LUT_HDMI) {
        strcpy(name_buf, "audio.amp.supbassvol.hdmi.lb.name");
    } else if (get_type == CC_GET_LUT_VGA) {
        strcpy(name_buf, "audio.amp.supbassvol.vga.lb.name");
    } else if (get_type == CC_GET_LUT_MPEG) {
        strcpy(name_buf, "audio.amp.supbassvol.mpeg.lb.name");
    } else if (get_type == CC_GET_LUT_HDMI_4K2K) {
        strcpy(name_buf, "audio.amp.supbassvol.hdmi4k2k.lb.name");
    } else if (get_type == CC_GET_LUT_USB_4K2K) {
        strcpy(name_buf, "audio.amp.supbassvol.usb4k2k.lb.name");
    } else {
        return -1;
    }

    return 0;
}

static int GetAudioSupperBassByModifyName(int get_type,
        int *VolumeDigitLutBuf_XX)
{
    int i = 0, buf_count = 0, buf_item_count = 0;
    char key_buf[128];
    const char *value_buf;

    memset(key_buf, '\0', 128);

    GetAudioSupperBassVolLutBufNameCFG(get_type, key_buf);

    value_buf = config_get_str("TV", key_buf, (char *) "");
    //LOGD("%s, get_type = %d, key_buf = %s, value_buf = %s \n", "TV",
    //        get_type, key_buf, value_buf);

    if (strcmp(value_buf, "") != 0) {
        if (GetAudioCfgBufferData(value_buf, &buf_count, &buf_item_count, 10,
                                  VolumeDigitLutBuf_XX) == 0) {
            return 0;
        }
    }

    return -1;
}

int *GetAudioAmplifierSupperBassVolumeLUTBufferPtr(int get_type,
        int audio_std)
{
    int buf_count = 0, buf_item_count = 0;

    int name_en = GetAudioSupperBassVolLutBufNameModifyEnableCFG();

    if (name_en == 1) {
        if (get_type == CC_GET_LUT_TV) {
            if (GetAudioSupperBassByModifyName(get_type,
                                               mTVSupperBassDigitLutBuf_XX) == 0) {
                return mTVSupperBassDigitLutBuf_XX;
            }
        } else if (get_type == CC_GET_LUT_AV) {
            if (GetAudioSupperBassByModifyName(get_type,
                                               mAVSupperBassDigitLutBuf_XX) == 0) {
                return mAVSupperBassDigitLutBuf_XX;
            }
        } else if (get_type == CC_GET_LUT_COMP) {
            if (GetAudioSupperBassByModifyName(get_type,
                                               mCOMPSupperBassDigitLutBuf_XX) == 0) {
                return mCOMPSupperBassDigitLutBuf_XX;
            }
        } else if (get_type == CC_GET_LUT_HDMI) {
            if (GetAudioSupperBassByModifyName(get_type,
                                               mHDMISupperBassDigitLutBuf_XX) == 0) {
                return mHDMISupperBassDigitLutBuf_XX;
            }
        } else if (get_type == CC_GET_LUT_VGA) {
            if (GetAudioSupperBassByModifyName(get_type,
                                               mVGASupperBassDigitLutBuf_XX) == 0) {
                return mVGASupperBassDigitLutBuf_XX;
            }
        } else if (get_type == CC_GET_LUT_MPEG) {
            if (GetAudioSupperBassByModifyName(get_type,
                                               mMPEGSupperBassDigitLutBuf_XX) == 0) {
                return mMPEGSupperBassDigitLutBuf_XX;
            }
        }
    } else {
        if (get_type == CC_GET_LUT_TV) {
            if (GetAudioCfgBufferData(
                    CS_AUDIO_AMP_SUPPERBASSVOL_TV_LUTBUF_CFG_NAME, &buf_count,
                    &buf_item_count, 10, mTVSupperBassDigitLutBuf_XX) == 0) {
                return mTVSupperBassDigitLutBuf_XX;
            }
        } else if (get_type == CC_GET_LUT_AV) {
            if (GetAudioCfgBufferData(
                    CS_AUDIO_AMP_SUPPERBASSVOL_AV_LUTBUF_CFG_NAME, &buf_count,
                    &buf_item_count, 10, mAVSupperBassDigitLutBuf_XX) == 0) {
                return mAVSupperBassDigitLutBuf_XX;
            }
        } else if (get_type == CC_GET_LUT_COMP) {
            if (GetAudioCfgBufferData(
                    CS_AUDIO_AMP_SUPPERBASSVOL_COMP_LUTBUF_CFG_NAME, &buf_count,
                    &buf_item_count, 10, mCOMPSupperBassDigitLutBuf_XX) == 0) {
                return mCOMPSupperBassDigitLutBuf_XX;
            }
        } else if (get_type == CC_GET_LUT_HDMI) {
            if (GetAudioCfgBufferData(
                    CS_AUDIO_AMP_SUPPERBASSVOL_HDMI_LUTBUF_CFG_NAME, &buf_count,
                    &buf_item_count, 10, mHDMISupperBassDigitLutBuf_XX) == 0) {
                return mHDMISupperBassDigitLutBuf_XX;
            }
        } else if (get_type == CC_GET_LUT_VGA) {
            if (GetAudioCfgBufferData(
                    CS_AUDIO_AMP_SUPPERBASSVOL_VGA_LUTBUF_CFG_NAME, &buf_count,
                    &buf_item_count, 10, mVGASupperBassDigitLutBuf_XX) == 0) {
                return mVGASupperBassDigitLutBuf_XX;
            }
        } else if (get_type == CC_GET_LUT_MPEG) {
            if (GetAudioCfgBufferData(
                    CS_AUDIO_AMP_SUPPERBASSVOL_MPEG_LUTBUF_CFG_NAME, &buf_count,
                    &buf_item_count, 10, mMPEGSupperBassDigitLutBuf_XX) == 0) {
                return mMPEGSupperBassDigitLutBuf_XX;
            }
        }
    }

    return NULL;
}

int *GetAudioEQPresetBufferPtr()
{
    int buf_count = 0, buf_item_count = 0;
    int *tmpDataBufPtr = NULL;

    tmpDataBufPtr = mEQPresetValueBuf_XX;

    memset((void *) tmpDataBufPtr, 0, sizeof(int) * CC_EQ_PRESET_BUF_SIZE);

    if (GetAudioCfgBufferData(CS_EQ_PRESETBUF_CFG_NAME, &buf_count,
                              &buf_item_count, 10, tmpDataBufPtr) == 0) {
        return tmpDataBufPtr;
    }

    return mEQPresetDefBuf;
}

int GetAudioDVISupportEnable()
{
    const char *config_value;

    config_value = config_get_str("TV", CS_DVI_SUPPORT_ENABLE_CFG_NAME, "null");

    if (strcmp(config_value, "null") == 0) {
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}

int GetTvAudioCardID(char tv_card_id_buf[])
{
    const char *value;
    value = config_get_str("TV", "audio.tv.card.id", "null");
    //LOGD("%s, get card id is \"%s\".\n", "TV", tv_card_id_buf);

    strcpy(tv_card_id_buf, value);
    if (strcmp(value, "null") == 0) {
        strcpy(tv_card_id_buf, "0");
        //    LOGD("%s, card id not config, we set to default \"%s\".\n",
        //            "TV", "0");
    }

    return 0;
}

int GetTvAudioCardName(char tv_card_name_buf[])
{
    const char *value;
    value = config_get_str("TV", "audio.tv.card.name", "null");
    //LOGD("%s, get card name is \"%s\".\n", "TV", tv_card_name_buf);

    strcpy(tv_card_name_buf, value);
    if (strcmp(value, "null") == 0) {
        strcpy(tv_card_name_buf, "AMLSYNO9629");
        //    LOGD("%s, card name not config, we set to default \"%s\".\n",
        //            "TV", "AML-M2");
    }

    return 0;
}

int GetTvAudioCardNeedSet()
{
    const char *config_value;

    config_value = config_get_str("TV", "audio.tv.card.needset", "null");
    //LOGD("%s, get \"%s\" is \"%s\".\n", "TV", "audio.tv.card.needset",
    //        config_value);

    if (strcmp(config_value, "null") == 0) {
        return 0;
    }

    if (strtoul(config_value, NULL, 10) == 0) {
        return 0;
    }

    return 1;
}

static int GetAudioEffectAmplifierGainCfg(const char *cfg_name,
        int def_gain_val, int max_gain_val)
{
    int tmp_val;
    const char *config_value;

    config_value = config_get_str("TV", cfg_name, "null");
    //LOGD("%s, get %s is \"%s\".\n", "TV", cfg_name, config_value);

    if (strcmp(config_value, "null") == 0) {
        //    LOGD(
        //            "%s, get config \"%s\" is \"%s\", return default audio effect amplifier gain value(%d).\n",
        //            "TV", cfg_name, config_value, def_gain_val);
        return def_gain_val;
    }

    tmp_val = strtoul(config_value, NULL, 10);
    if (tmp_val < 0 || tmp_val > max_gain_val) {
        //   LOGD(
        //            "%s, get config \"%s\" is error, it(%d) should between %d and %d, return default audio effect amplifier gain value(%d)\n",
        //            "TV", cfg_name, tmp_val, 0, max_gain_val, def_gain_val);
        tmp_val = def_gain_val;
    }

    return tmp_val;
}

int GetAudioSRSModuleDisableCFG()
{
    const char *config_value;

    config_value = config_get_str("TV", CS_SRS_MOUDLE_DISABLE_CFG_NAME, "null");

    if (strcmp(config_value, "null") == 0) {
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}

int GetAudioSRSSourroundDisableCFG()
{
    const char *config_value;

    config_value = config_get_str("TV", CS_SRS_SOURROUND_DISABLE_CFG_NAME,
                                  "null");

    if (strcmp(config_value, "null") == 0) {
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}

int GetAudioSRSTrubassDisableCFG()
{
    const char *config_value;

    config_value = config_get_str("TV", CS_SRS_TRUBASS_DISABLE_CFG_NAME,
                                  "null");

    if (strcmp(config_value, "null") == 0) {
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}

int GetAudioSRSDialogClarityDisableCFG()
{
    const char *config_value;

    config_value = config_get_str("TV", CS_SRS_DIALOGCLARITY_DISABLE_CFG_NAME,
                                  "null");

    if (strcmp(config_value, "null") == 0) {
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}

static int GetAudioSRSGainCfg(const char *cfg_name, int def_gain_val)
{
    int tmp_val;
    const char *config_value;

    config_value = config_get_str("TV", cfg_name, "null");
    //LOGD("%s, get %s is \"%s\".\n", "TV", cfg_name, config_value);

    if (strcmp(config_value, "null") == 0) {
        //    LOGD(
        //            "%s, get config \"%s\" is \"%s\", return -1 to use default gain value.\n",
        //            "TV", cfg_name, config_value);
        return -1;
    }

    tmp_val = strtoul(config_value, NULL, 10);
    if (tmp_val < 0 || tmp_val > 100) {
        tmp_val = def_gain_val;
        //    LOGD("%s, get config \"%s\" is error, we set it to %d\n", "TV",
        //            cfg_name, tmp_val);
    }

    return tmp_val;
}

static int GetAudioSRSAmplifierGainCfg(const char *cfg_name, int def_gain_val)
{
    return GetAudioEffectAmplifierGainCfg(cfg_name, def_gain_val, 24);
}

static int GetAudioEQAmplifierGainCfg(const char *cfg_name, int def_gain_val)
{
    return GetAudioEffectAmplifierGainCfg(cfg_name, def_gain_val, 24);
}

int GetAudioSRSSourroundGainCfg()
{
    return GetAudioSRSGainCfg(CS_SRS_SOURROUND_GAIN_CFG_NAME,
                              CC_SRS_SURROUND_GAIN_VAL);
}

int GetAudioSRSSourroundAmplifierMasterGainCfg()
{
    return GetAudioSRSAmplifierGainCfg(
               CS_SRS_SOURROUND_AMP_MASTER_GAIN_CFG_NAME,
               CC_SRS_SURROUND_AMP_MASTER_GAIN_VAL);
}

int GetAudioSRSSourroundAmplifierSupperBassGainCfg()
{
    return GetAudioSRSAmplifierGainCfg(
               CS_SRS_SOURROUND_AMP_SUPPERBASS_GAIN_CFG_NAME,
               CC_SRS_SURROUND_AMP_SUPPERBASS_GAIN_VAL);
}

int GetAudioSRSTrubassOnlyGainCfg()
{
    return GetAudioSRSGainCfg(CS_SRS_TRUBASS_ONLY_GAIN_CFG_NAME,
                              CC_SRS_TRUBASS_ONLY_GAIN_VAL);
}

int GetAudioSRSTrubassOnlyAmplifierMasterGainCfg()
{
    return GetAudioSRSAmplifierGainCfg(
               CS_SRS_TRUBASS_ONLY_AMP_MASTER_GAIN_CFG_NAME,
               CC_SRS_TRUBASS_ONLY_AMP_MASTER_GAIN_VAL);
}

int GetAudioSRSTrubassOnlyAmplifierSupperBassGainCfg()
{
    return GetAudioSRSAmplifierGainCfg(
               CS_SRS_TRUBASS_ONLY_AMP_SUPPERBASS_GAIN_CFG_NAME,
               CC_SRS_TRUBASS_ONLY_AMP_SUPPERBASS_GAIN_VAL);
}

int GetAudioSRSDialogClarityOnlyGainCfg()
{
    return GetAudioSRSGainCfg(CS_SRS_DIALOGCLARITY_ONLY_GAIN_CFG_NAME,
                              CC_SRS_DIALOGCLARITY_ONLY_GAIN_VAL);
}

int GetAudioSRSDialogClarityOnlyAmplifierMasterGainCfg()
{
    return GetAudioSRSAmplifierGainCfg(
               CS_SRS_DIALOGCLARITY_ONLY_AMP_MASTER_GAIN_CFG_NAME,
               CC_SRS_DIALOGCLARITY_ONLY_AMP_MASTER_GAIN_VAL);
}

int GetAudioSRSDialogClarityOnlyAmplifierSupperBassGainCfg()
{
    return GetAudioSRSAmplifierGainCfg(
               CS_SRS_DIALOGCLARITY_ONLY_AMP_SUPPERBASS_GAIN_CFG_NAME,
               CC_SRS_DIALOGCLARITY_ONLY_AMP_SUPPERBASS_GAIN_VAL);
}

int GetAudioSRSDialogClarityOnlyDefinitionGainCfg()
{
    return GetAudioSRSGainCfg(
               CS_SRS_DIALOGCLARITY_ONLY_DEFINITION_GAIN_CFG_NAME,
               CC_SRS_DIALOGCLARITY_ONLY_DEFINITION_GAIN_VAL);
}

int GetAudioSRSTrubassAndDialogClarityTrubassGainCfg()
{
    return GetAudioSRSGainCfg(
               CS_SRS_TRUBASS_DIALOGCLARITY_TRUBASS_GAIN_CFG_NAME,
               CC_SRS_TRUBASS_DIALOGCLARITY_TRUBASS_GAIN_VAL);
}

int GetAudioSRSTrubassAndDialogClarityDialogClarityGainCfg()
{
    return GetAudioSRSGainCfg(
               CS_SRS_TRUBASS_DIALOGCLARITY_DIALOGCLARITY_GAIN_CFG_NAME,
               CC_SRS_TRUBASS_DIALOGCLARITY_DIALOGCLARITY_GAIN_VAL);
}

int GetAudioSRSTrubassAndDialogClarityDefinitionGainCfg()
{
    return GetAudioSRSGainCfg(
               CS_SRS_TRUBASS_DIALOGCLARITY_DEFINITION_GAIN_CFG_NAME,
               CC_SRS_TRUBASS_DIALOGCLARITY_DEFINITION_GAIN_VAL);
}

int GetAudioSRSTrubassAndDialogClarityAmplifierMasterGainCfg()
{
    return GetAudioSRSAmplifierGainCfg(
               CS_SRS_TRUBASS_DIALOGCLARITY_AMP_MASTER_GAIN_CFG_NAME,
               CC_SRS_TRUBASS_DIALOGCLARITY_AMP_MASTER_GAIN_VAL);
}

int GetAudioSRSTrubassAndDialogClarityAmplifierSupperBassGainCfg()
{
    return GetAudioSRSAmplifierGainCfg(
               CS_SRS_TRUBASS_DIALOGCLARITY_AMP_SUPPERBASS_GAIN_CFG_NAME,
               CC_SRS_TRUBASS_DIALOGCLARITY_AMP_SUPPERBASS_GAIN_VAL);
}

int GetAudioSRSSupperBassTrubassSpeakerSizeCfg(int get_flag)
{
    int tmp_val;
    char cfg_name[128] = { 0 };
    const char *config_value;

    if (get_flag == 0) {
        strcpy(cfg_name, CS_SRS_SUPPERBASS_OFF_TRUBASS_SPEAKER_SIZE_CFG_NAME);
    } else {
        strcpy(cfg_name, CS_SRS_SUPPERBASS_ON_TRUBASS_SPEAKER_SIZE_CFG_NAME);
    }

    config_value = config_get_str("TV", cfg_name, "null");

    if (strcmp(config_value, "null") == 0) {
        return -1;
    }

    tmp_val = strtoul(config_value, NULL, 10);
    if (tmp_val < 0 || tmp_val > 7) {
        tmp_val = -1;
    }

    return tmp_val;
}

int GetAudioEQAmplifierMasterAddGainCfg()
{
    return GetAudioEQAmplifierGainCfg(CS_EQ_AMP_MASTER_GAIN_CFG_NAME,
                                      CC_EQ_AMP_MASTER_GAIN_VAL);
}

int GetAudioEQAmplifierSupperBassAddGainCfg()
{
    return GetAudioEQAmplifierGainCfg(CS_EQ_AMP_SUPPERBASS_GAIN_CFG_NAME,
                                      CC_EQ_AMP_SUPPERBASS_GAIN_VAL);
}

int GetAudioEqModuleDisableCFG()
{
    const char *config_value;

    config_value = config_get_str("TV", CS_EQ_MOUDLE_DISABLE_CFG_NAME, "null");

    if (strcmp(config_value, "null") == 0) {
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}

int GetAudioEqUsingAndroidCFG()
{
    const char *config_value;

    config_value = config_get_str("TV", CS_EQ_USING_ANDROID_CFG_NAME, "null");

    if (strcmp(config_value, "null") == 0) {
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}

int GetAudioEQGainRangeCFG(int *min_val, int *max_val)
{
    int i = 0, buf_item_count = 0, tmp_item_count = 0;
    int tmp_buf[16] = { 0 };

    buf_item_count = 2;
    tmp_item_count = buf_item_count;
    if (GetAudioCfgData(CS_EQ_GAIN_RANGE_CFG_NAME, &tmp_item_count, 10, tmp_buf)
        < 0) {
        return -1;
    }

    if (tmp_item_count != buf_item_count) {
        return -1;
    }

    *min_val = tmp_buf[0];
    *max_val = tmp_buf[1];

    return 0;
}

int GetAudioEQUIGainRangeCFG(int *min_val, int *max_val)
{
    int i = 0, buf_item_count = 0, tmp_item_count = 0;
    int tmp_buf[16] = { 0 };

    buf_item_count = 2;
    tmp_item_count = buf_item_count;
    if (GetAudioCfgData(CS_EQ_UI_GAIN_RANGE_CFG_NAME, &tmp_item_count, 10,
                        tmp_buf) < 0) {
        return -1;
    }

    if (tmp_item_count != buf_item_count) {
        return -1;
    }

    *min_val = tmp_buf[0];
    *max_val = tmp_buf[1];

    return 0;
}

int GetAudioDumpDataEnableFlagCfg()
{
    const char *config_value;

    config_value = config_get_str("TV", CS_AUDIO_DUMP_DATA_ENABLE_CFG_NAME,
                                  "null");

    if (strcmp(config_value, "null") == 0) {
        return 0;
    }

    return strtol(config_value, NULL, 10);
}

int GetAudioKeyMuteCfg()
{
    const char *config_value;

    config_value = config_get_str("TV", CS_KEY_MUTE_CFG_NAME, "null");

    if (strcmp(config_value, "null") == 0) {
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}

int GetAudioAVOutInitMuteStatusCFG()
{
    const char *config_value;
    int status = 0;

    config_value = config_get_str("TV", CS_AVOUT_INIT_MUTE_CFG_NAME, "null");
    LOGD("%s, get %s is \"%s\".\n", __FUNCTION__, CS_AVOUT_INIT_MUTE_CFG_NAME,
         config_value);

    if (strcmp(config_value, "enable") == 0) {
        status = 1;
    } else {
        status = 0;
    }

    return status;
}

int GetAudioSupperBassModuleDisableCFG()
{
    const char *config_value;

    config_value = config_get_str("TV", CS_SUPPERBASS_MODULE_DISABLE_CFG_NAME,
                                  "null");

    if (strcmp(config_value, "null") == 0) {
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}

int GetAudioSupperBassSwitchDisableCFG()
{
    const char *config_value;

    config_value = config_get_str("TV", CS_SUPPERBASS_SWITCH_DISABLE_CFG_NAME,
                                  "null");

    if (strcmp(config_value, "null") == 0) {
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}

static int GetAudioSrcInputLineInVol(const char *cfg_str, int *l_vol,
                                     int *r_vol)
{
    int buf_item_count = 2;
    int data_buf[2] = { -1, -1 };

    if (GetAudioCfgData(cfg_str, &buf_item_count, 10, data_buf) < 0) {
        return -1;
    }

    if (buf_item_count < 2) {
        return -1;
    }

    *l_vol = data_buf[0];
    *r_vol = data_buf[1];

    return 0;
}

int GetAudioSrcInputTVLineInVol(int *l_vol, int *r_vol)
{
    return GetAudioSrcInputLineInVol(CS_SRC_INPUT_TV_LINEIN_VOL_CFG_NAME, l_vol,
                                     r_vol);
}

int GetAudioSrcInputAVLineInVol(int *l_vol, int *r_vol)
{
    return GetAudioSrcInputLineInVol(CS_SRC_INPUT_AV_LINEIN_VOL_CFG_NAME, l_vol,
                                     r_vol);
}

int GetAudioSrcInputCOMPLineInVol(int *l_vol, int *r_vol)
{
    return GetAudioSrcInputLineInVol(CS_SRC_INPUT_COMP_LINEIN_VOL_CFG_NAME,
                                     l_vol, r_vol);
}

int GetAudioSrcInputHDMILineInVol(int *l_vol, int *r_vol)
{
    return GetAudioSrcInputLineInVol(CS_SRC_INPUT_HDMI_LINEIN_VOL_CFG_NAME,
                                     l_vol, r_vol);
}

int GetAudioSrcInputVGALineInVol(int *l_vol, int *r_vol)
{
    return GetAudioSrcInputLineInVol(CS_SRC_INPUT_VGA_LINEIN_VOL_CFG_NAME,
                                     l_vol, r_vol);
}

int GetAudioSrcInputMPEGLineInVol(int *l_vol, int *r_vol)
{
    return GetAudioSrcInputLineInVol(CS_SRC_INPUT_MPEG_LINEIN_VOL_CFG_NAME,
                                     l_vol, r_vol);
}

static int GetAudioSrcInputLineOutVol(const char *cfg_str, int *l_vol,
                                      int *r_vol)
{
    int buf_item_count = 2;
    int data_buf[2] = { -1, -1 };

    if (GetAudioCfgData(cfg_str, &buf_item_count, 10, data_buf) < 0) {
        return -1;
    }

    if (buf_item_count < 2) {
        return -1;
    }

    *l_vol = data_buf[0];
    *r_vol = data_buf[1];

    return 0;
}

int GetAudioSrcInputTVLineOutVol(int *l_vol, int *r_vol)
{
    return GetAudioSrcInputLineOutVol(CS_SRC_INPUT_TV_LINEOUT_VOL_CFG_NAME,
                                      l_vol, r_vol);
}

int GetAudioSrcInputAVLineOutVol(int *l_vol, int *r_vol)
{
    return GetAudioSrcInputLineOutVol(CS_SRC_INPUT_AV_LINEOUT_VOL_CFG_NAME,
                                      l_vol, r_vol);
}

int GetAudioSrcInputCOMPLineOutVol(int *l_vol, int *r_vol)
{
    return GetAudioSrcInputLineOutVol(CS_SRC_INPUT_COMP_LINEOUT_VOL_CFG_NAME,
                                      l_vol, r_vol);
}

int GetAudioSrcInputHDMILineOutVol(int *l_vol, int *r_vol)
{
    return GetAudioSrcInputLineOutVol(CS_SRC_INPUT_HDMI_LINEOUT_VOL_CFG_NAME,
                                      l_vol, r_vol);
}

int GetAudioSrcInputVGALineOutVol(int *l_vol, int *r_vol)
{
    return GetAudioSrcInputLineOutVol(CS_SRC_INPUT_VGA_LINEOUT_VOL_CFG_NAME,
                                      l_vol, r_vol);
}

int GetAudioSrcInputMPEGLineOutVol(int *l_vol, int *r_vol)
{
    int tmp_val = 0;
    const char *config_value;

    config_value = config_get_str("TV",
                                  CS_SRC_INPUT_MPEG_LINEOUT_VOL_CFG_NAME_OLD, "null");

    if (strcmp(config_value, "null") == 0) {
        return GetAudioSrcInputLineOutVol(
                   CS_SRC_INPUT_MPEG_LINEOUT_VOL_CFG_NAME, l_vol, r_vol);
    }

    tmp_val = strtol(config_value, NULL, 10);

    *l_vol = tmp_val;
    *r_vol = tmp_val;

    return 0;
}

int GetAudioWallEffectTypeCfg()
{
    return GetAudioCfgSingleIntData("audio.walleffect.type", 0);
}

static int GetAudioMainVolumeLutBufCompensation(const char *cfg_str,
        int *cfg_flag)
{
    const char *config_value;

    config_value = config_get_str("TV", cfg_str, "null");
    //LOGD("%s, get \"%s\" is \"%s\".\n", "TV", cfg_str, config_value);

    if (strcmp(config_value, "null") == 0) {
        if (cfg_flag != NULL) {
            *cfg_flag = 0;
        }

        //LOGD(
        //        "%s, get config \"%s\" is \"%s\", return 0 to not compensation lut buffer.\n",
        //        "TV", cfg_str, config_value);
        return 0;
    }

    if (cfg_flag != NULL) {
        *cfg_flag = 1;
    }

    return strtol(config_value, NULL, 10);
}

int GetAudioMainVolumeLutBufTVCompensation()
{
    return GetAudioMainVolumeLutBufCompensation(
               CS_AMP_MAINVOL_LUTBUF_COMPENSATION_TV_CFG_NAME, NULL);
}

int GetAudioMainVolumeLutBufAVCompensation()
{
    return GetAudioMainVolumeLutBufCompensation(
               CS_AMP_MAINVOL_LUTBUF_COMPENSATION_AV_CFG_NAME, NULL);
}

int GetAudioMainVolumeLutBufCOMPCompensation()
{
    return GetAudioMainVolumeLutBufCompensation(
               CS_AMP_MAINVOL_LUTBUF_COMPENSATION_COMP_CFG_NAME, NULL);
}

int GetAudioMainVolumeLutBufHDMICompensation()
{
    return GetAudioMainVolumeLutBufCompensation(
               CS_AMP_MAINVOL_LUTBUF_COMPENSATION_HDMI_CFG_NAME, NULL);
}

int GetAudioMainVolumeLutBufVGACompensation()
{
    return GetAudioMainVolumeLutBufCompensation(
               CS_AMP_MAINVOL_LUTBUF_COMPENSATION_VGA_CFG_NAME, NULL);
}

int GetAudioMainVolumeLutBufMPEGCompensation()
{
    int tmp_ret = 0, cfg_flag = 0;

    tmp_ret = GetAudioMainVolumeLutBufCompensation(
                  CS_AMP_MAINVOL_LUTBUF_COMPENSATION_MPEG_CFG_NAME, &cfg_flag);

    if (cfg_flag == 0) {
        tmp_ret = GetAudioMainVolumeLutBufDTVCompensation();
    }

    return tmp_ret;
}

int GetAudioMainVolumeLutBufDTVCompensation()
{
    return GetAudioMainVolumeLutBufCompensation(
               CS_AMP_MAINVOL_LUTBUF_COMPENSATION_DTV_CFG_NAME, NULL);
}

int GetAudioNoiseGateEnableCFG()
{
    const char *config_value;

    config_value = config_get_str("TV", CS_AUDIO_NOISE_GATE_ENABLE_CFG_NAME,
                                  "null");

    if (strcmp(config_value, "null") == 0) {
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}

int GetAudioNoiseGateUpperBoundCFG()
{
    const char *config_value;

    config_value = config_get_str("TV",
                                  CS_AUDIO_NOISE_GATE_UPPER_BOUND_CFG_NAME, "254");

    return strtol(config_value, NULL, 10);
}

int GetAudioNoiseGateThresholdCFG()
{
    const char *config_value;

    config_value = config_get_str("TV", CS_AUDIO_NOISE_GATE_THRESHOLD_CFG_NAME,
                                  "-1");

    return strtol(config_value, NULL, 10);
}

int GetDisableResetSystemFrameSize()
{
    const char *config_value;
    config_value = config_get_str("TV",
                                  CS_DISABLE_RESET_SYSTEM_FRAMESIZE_CFG_NAME, "null");

    if (strcmp(config_value, "null") == 0) {
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}

static int gDisableResetSystemFrameSizeFlag = -1;
int GetDisableReset_System_framesizeCFG(void)
{
    if (gDisableResetSystemFrameSizeFlag == -1) {
        const char *config_value;

        config_value = config_get_str("TV", CS_AUDIO_SYSTEM_FRAMESIZE_CFG_NAME,
                                      "null");
        LOGD("%s, get %s is \"%s\".\n",
             __FUNCTION__, CS_AUDIO_SYSTEM_FRAMESIZE_CFG_NAME, config_value);

        if (strcmp(config_value, "disable") == 0) {
            gDisableResetSystemFrameSizeFlag = 0x55;
        } else {
            gDisableResetSystemFrameSizeFlag = 0;
        }
    }

    return gDisableResetSystemFrameSizeFlag;
}

int GetAudioArchitectureTypeCFG()
{
    static int architecture_type = -1;
    const char *config_value = NULL;

    if (architecture_type == -1) {
        config_value = config_get_str ( "TV", CS_AUDIO_ARCHITECTURE_TYPE_CFG_NAME, "null" );
        if (strcasecmp(config_value, "null") == 0) {
            architecture_type = CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_FBC;
        } else if (strcasecmp(config_value, "t866_external_dac_offboard_fbc") == 0) {
            architecture_type = CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_FBC;
        } else if (strcasecmp(config_value, "t866_external_dac_offboard_customer_lib") == 0) {
            architecture_type = CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_CUSTOMER_LIB;
        } else if (strcasecmp(config_value, "t866_external_dac_offboard_digital") == 0) {
            architecture_type = CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_DIGITAL;
        } else if (strcasecmp(config_value, "t866_external_dac_onboard") == 0) {
            architecture_type = CC_DAC_T866_EXTERNAL_DAC_ON_BOARD;
        } else if (strcasecmp(config_value, "t866_iternal_dac") == 0) {
            architecture_type = CC_DAC_T866_INTERNAL_DAC;
        } else if (strcasecmp(config_value, "m6c") == 0) {
            architecture_type = CC_DAC_M6C_EXTERNAL_DAC_ON_BOARD;
        } else {
            architecture_type = CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_FBC;
        }
    }

    return architecture_type;
}

static int gAudioResampleType = -1;
int GetAudioResampleTypeCFG()
{
    char *token = NULL;
    const char *strDelimit = ",";
    const char *config_value = NULL;
    char cfg_buf[1024];

    if (gAudioResampleType == -1) {
        gAudioResampleType = 0;

        config_value = config_get_str ( "TV", CS_AUDIO_RESAMPLE_TYPE_CFG_NAME, "null" );
        strncpy(cfg_buf, config_value, sizeof(cfg_buf));
        if (strcmp(cfg_buf, "") != 0) {
            token = strtok(cfg_buf, strDelimit);
            while (token != NULL) {
                if (strcasecmp(token, "hw") == 0) {
                    gAudioResampleType |= CC_AUD_RESAMPLE_TYPE_HW;
                } else if (strcasecmp(token, "sw") == 0) {
                    gAudioResampleType |= CC_AUD_RESAMPLE_TYPE_SW;
                }

                token = strtok(NULL, strDelimit);
            }
        } else {
            gAudioResampleType = CC_AUD_RESAMPLE_TYPE_SW; //if can't find config string, allow sw resample
        }
    }

    return gAudioResampleType;
}

int GetamAudioDisableFlagCFG()
{
    static int am_audio_disable_flag = -1;
    const char *config_value = NULL;

    if (am_audio_disable_flag == -1) {
        config_value = config_get_str ( "TV", "audio.amaudio.disable", "null" );
        if (strcasecmp(config_value, "null") == 0 || strcasecmp(config_value, "false") == 0) {
            am_audio_disable_flag = 0;
        } else if (strcasecmp(config_value, "true") == 0) {
            am_audio_disable_flag = 1;
        } else {
            am_audio_disable_flag = 0;
        }
    }

    return am_audio_disable_flag;
}

int GetAudioFirstInitSetTypeCFG()
{
    const char *config_value;
    config_value = config_get_str ( "TV", "audio.firstinitset.type", "null" );

    if (strcmp(config_value, "null") == 0) {
        return 0;
    }

    return strtoul(config_value, NULL, 10);
}
