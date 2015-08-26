#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

#include <android/log.h>
#include <cutils/properties.h>

#include "../tvsetting/CTvSetting.h"
#include "../tvutils/tvutils.h"

#include "audio_effect.h"
#include "audio_android.h"
#include "audio_android_effect.h"
#include "audio_amaudio.h"

#include "audio_api.h"

#define LOG_TAG "audio_api"
#include "CTvLog.h"

static int mCustomEQGainBuf[] = { 0, 0, 0, 0, 0, 0 };

static int mCurEQGainBuf[] = { 0, 0, 0, 0, 0, 0 };

static int8_t mCurEQGainChBuf[] = { 0, 0, 0, 0, 0, 0 };

static int mWallEffectValueBuf[] = { 0, 0, 1, 2, 2, 0 };

static    const  int   CC_MUTE_TYPE_KEY_MUTE   =    (0);
static    const  int    CC_MUTE_TYPE_SYS_MUTE    =   (1);


//extern int audio_device_switch;
int audio_device_switch = 1;//showboz

CTvAudio::CTvAudio()
{
    mAudioMuteStatusForSystem = CC_AUDIO_UNMUTE;
    mAudioMuteStatusForTv = CC_AUDIO_UNMUTE;
    mCurAudioMasterVolume = CC_DEF_SOUND_VOL;
    mCurAudioBalance = CC_DEF_SOUND_BALANCE_VAL;
    mCurAudioSupperBassVolume = CC_DEF_SUPPERBASS_VOL;
    mCurAudioSupperBassSwitch = CC_SWITCH_OFF;
    mCurAudioSRSSurround = CC_SWITCH_OFF;
    mCurAudioSrsDialogClarity = CC_SWITCH_OFF;
    mCurAudioSrsTruBass = CC_SWITCH_OFF;
    mCurAudioSPDIFSwitch = CC_SWITCH_ON;
    mCurAudioSPDIFMode = CC_SPDIF_MODE_PCM;
    mCurAudioBassVolume = CC_DEF_BASS_TREBLE_VOL;
    mCurAudioTrebleVolume = CC_DEF_BASS_TREBLE_VOL;
    mCurAudioSoundMode = CC_SOUND_MODE_END;
    mCurAudioWallEffect = CC_SWITCH_OFF;
    mCurAudioEQMode = CC_EQ_MODE_START;
    mCustomAudioMasterVolume = CC_DEF_SOUND_VOL;
    mCustomAudioBalance = CC_DEF_SOUND_BALANCE_VAL;
    mCustomAudioSupperBassVolume = CC_DEF_SUPPERBASS_VOL;
    mCustomAudioSupperBassSwitch = CC_SWITCH_OFF;
    mCustomAudioSRSSurround = CC_SWITCH_OFF;
    mCustomAudioSrsDialogClarity = CC_SWITCH_OFF;
    mCustomAudioSrsTruBass = CC_SWITCH_OFF;
    mCustomAudioBassVolume = CC_DEF_BASS_TREBLE_VOL;
    mCustomAudioTrebleVolume = CC_DEF_BASS_TREBLE_VOL;
    mCustomAudioSoundMode = CC_SOUND_MODE_END;
    mCustomAudioWallEffect = CC_SWITCH_OFF;
    mCustomAudioEQMode = CC_EQ_MODE_START;
    mCustomAudioSoundEnhancementSwitch = CC_SWITCH_OFF;
    mVolumeCompensationVal = CC_VOLUME_COMPENSATION_DEF_VAL;
}

CTvAudio::~CTvAudio()
{

}

template<typename T1, typename T2>
int AudioArrayCopy(T1 dst_buf[], T2 src_buf[], int copy_size)
{
    int i = 0;

    for (i = 0; i < copy_size; i++) {
        dst_buf[i] = src_buf[i];
    }

    return 0;
}

//Audio Mute
int CTvAudio::SetAudioMuteForSystem(int muteOrUnmute)
{
    int ret = 0;
    LOGD("SetAudioMuteForSystem sysMuteStats=%d, tvMuteStatus=%d,  toMute=%d", mAudioMuteStatusForSystem, mAudioMuteStatusForTv, muteOrUnmute);
    mAudioMuteStatusForSystem = muteOrUnmute;
    ret |= mAudioAlsa.SetDacMute(mAudioMuteStatusForSystem | mAudioMuteStatusForTv, CC_DAC_MUTE_TYPE_EXTERNAL | CC_DAC_MUTE_TYPE_INTERNAL);
    ret |= SetAudioSPDIFMute(mAudioMuteStatusForSystem | mAudioMuteStatusForTv);
    return ret;
}

int CTvAudio::GetAudioMuteForSystem()
{
    return mAudioMuteStatusForSystem;
}

int CTvAudio::SetAudioMuteForTv(int Mute)
{
    int ret = 0;
    mAudioMuteStatusForTv = Mute;
    LOGD("SetAudioMuteForTv sysMuteStats=%d, tvMuteStatus=%d,  toMute=%d", mAudioMuteStatusForSystem, mAudioMuteStatusForTv, Mute);
    ret |= mAudioAlsa.SetDacMute(mAudioMuteStatusForSystem | mAudioMuteStatusForTv, CC_DAC_MUTE_TYPE_EXTERNAL | CC_DAC_MUTE_TYPE_INTERNAL);
    ret |= SetAudioSPDIFMute(mAudioMuteStatusForSystem | mAudioMuteStatusForTv);
    return ret;
}

int CTvAudio::GetAudioMuteForTv()
{
    return mAudioMuteStatusForTv;
}

int CTvAudio::SetAudioAVOutMute(int muteStatus)
{
    return mAudioAlsa.SetDacMute(muteStatus, CC_DAC_MUTE_TYPE_INTERNAL);
}

int CTvAudio::GetAudioAVOutMute()
{
    return CC_AUDIO_UNMUTE;
}

int CTvAudio::handleAudioSPDIFMute(int muteStatus)
{
    int set_val = 0;

    if (muteStatus == CC_AUDIO_MUTE) {
        set_val = CC_SPDIF_MUTE_ON;
    } else if (muteStatus == CC_AUDIO_UNMUTE) {
        set_val = CC_SPDIF_MUTE_OFF;
    } else {
        return -1;
    }

    return mAudioAlsa.SetSPDIFMute(set_val);
}

int CTvAudio::SetAudioSPDIFMute(int muteStatus)
{
    if (GetCurAudioSPDIFSwitch() == CC_SWITCH_OFF) {
        muteStatus = CC_AUDIO_MUTE;
    }

    return handleAudioSPDIFMute(muteStatus);
}

int CTvAudio::GetAudioSPDIFMute()
{
    return CC_AUDIO_UNMUTE;
}

//Audio SPDIF switch
int CTvAudio::SetAudioSPDIFSwitch(int tmp_val)
{
    int muteStatus = CC_AUDIO_UNMUTE;

    mCurAudioSPDIFSwitch = tmp_val;

    if (tmp_val == CC_SWITCH_OFF || mAudioMuteStatusForSystem == CC_AUDIO_MUTE || mAudioMuteStatusForTv == CC_AUDIO_MUTE) {
        muteStatus = CC_AUDIO_MUTE;
    } else {
        muteStatus = CC_AUDIO_UNMUTE;
    }

    handleAudioSPDIFMute(muteStatus);
    return 0;
}

int CTvAudio::GetCurAudioSPDIFSwitch()
{
    return mCurAudioSPDIFSwitch;
}

int CTvAudio::SaveCurAudioSPDIFSwitch(int tmp_val)
{
    int8_t tmp_ch = 0;
    if (tmp_val != CC_SWITCH_ON && tmp_val != CC_SWITCH_OFF) {
        tmp_val = CC_SWITCH_ON;
    }

    mCurAudioSPDIFSwitch = tmp_val;
    tmp_ch = tmp_val;
    SSMSaveAudioSPDIFSwitchVal(tmp_ch);

    return mCurAudioSPDIFSwitch;
}

int CTvAudio::LoadCurAudioSPDIFSwitch()
{
    int8_t tmp_ch = 0;
    SSMReadAudioSPDIFSwitchVal(&tmp_ch);
    mCurAudioSPDIFSwitch = tmp_ch;
    if (mCurAudioSPDIFSwitch != CC_SWITCH_ON
        && mCurAudioSPDIFSwitch != CC_SWITCH_OFF) {
        SaveCurAudioSPDIFSwitch (CC_SWITCH_ON);
    }
    return mCurAudioSPDIFSwitch;
}

//Audio SPDIF Mode
int CTvAudio::SetAudioSPDIFMode(int tmp_val)
{
    LOGD("%s : tmp_val = %d\n", __FUNCTION__, tmp_val);
    mCurAudioSPDIFMode = tmp_val;

    SetSPDIFMode(mCurAudioSPDIFMode);

    return 0;
}

int CTvAudio::GetCurAudioSPDIFMode()
{
    return mCurAudioSPDIFMode;
}

int CTvAudio::SaveCurAudioSPDIFMode(int tmp_val)
{
    int8_t tmp_ch = 0;
    if (tmp_val != CC_SPDIF_MODE_PCM && tmp_val != CC_SPDIF_MODE_RAW) {
        tmp_val = CC_SPDIF_MODE_PCM;
    }

    mCurAudioSPDIFMode = tmp_val;
    tmp_ch = tmp_val;
    SSMSaveAudioSPDIFModeVal(tmp_ch);

    return mCurAudioSPDIFMode;
}

int CTvAudio::LoadCurAudioSPDIFMode()
{
    int8_t tmp_ch = 0;
    SSMReadAudioSPDIFModeVal(&tmp_ch);
    mCurAudioSPDIFMode = tmp_ch;
    if (mCurAudioSPDIFMode != CC_SPDIF_MODE_PCM
        && mCurAudioSPDIFMode != CC_SPDIF_MODE_RAW) {
        SaveCurAudioSPDIFMode (CC_SPDIF_MODE_PCM);
    }
    return mCurAudioSPDIFMode;
}

//Audio Master Volume
int CTvAudio::SetAudioMasterVolume(int tmp_vol)
{
    const char *value;
    if (tmp_vol < CC_MIN_SOUND_VOL || tmp_vol > CC_MAX_SOUND_VOL) {
        tmp_vol = CC_DEF_SOUND_VOL;
    }

    mCustomAudioMasterVolume = tmp_vol;

    //Volume Compensation
    if (tmp_vol != CC_MIN_SOUND_VOL) {
        tmp_vol += GetAudioVolumeCompensationVal();
    }

    if (tmp_vol > CC_MAX_SOUND_VOL) {
        tmp_vol = CC_MAX_SOUND_VOL;
    }

    if (tmp_vol < CC_MIN_SOUND_VOL) {
        tmp_vol = CC_MIN_SOUND_VOL;
    }

    mAudioAlsa.SetDacMainVolume(tmp_vol);
    value = config_get_str ( "TV", "tvin.2d4G.headset.en", "null" );
    if ( strcmp ( value, "enable" ) == 0 ) {
        setAudioPcmPlaybackVolume(tmp_vol);
    }
    return 0;
}

int CTvAudio::GetAudioMasterVolume()
{
    return mCustomAudioMasterVolume;
}

int CTvAudio::GetCurAudioMasterVolume()
{
    return mCurAudioMasterVolume;
}

int CTvAudio::SaveCurAudioMasterVolume(int tmp_vol)
{
    int8_t tmp_ch = 0;

    if (tmp_vol < CC_MIN_SOUND_VOL || tmp_vol > CC_MAX_SOUND_VOL) {
        tmp_vol = CC_DEF_SOUND_VOL;
    }

    mCurAudioMasterVolume = tmp_vol;
    tmp_ch = tmp_vol;
    SSMSaveAudioMasterVolume(tmp_ch);

    return mCurAudioMasterVolume;
}

int CTvAudio::LoadCurAudioMasterVolume()
{
    int8_t tmp_ch = 0;
    SSMReadAudioMasterVolume(&tmp_ch);
    mCurAudioMasterVolume = tmp_ch;
    if (mCurAudioMasterVolume < CC_MIN_SOUND_VOL
        || mCurAudioMasterVolume > CC_MAX_SOUND_VOL) {
        SaveCurAudioMasterVolume (CC_DEF_SOUND_VOL);
    }

    mCustomAudioMasterVolume = mCurAudioMasterVolume;

    return mCurAudioMasterVolume;
}

//Audio Balance
int CTvAudio::SetAudioBalance(int tmp_val)
{
    if (tmp_val < CC_MIN_SOUND_BALANCE_VAL
        || tmp_val > CC_MAX_SOUND_BALANCE_VAL) {
        tmp_val = CC_DEF_SOUND_BALANCE_VAL;
    }

    mCustomAudioBalance = tmp_val;

    if (mAudioAlsa.SetBalanceValue(mCustomAudioBalance) < 0) {
        return SetAudioMasterVolume(GetAudioMasterVolume());
    }

    return 0;
}

int CTvAudio::GetAudioBalance()
{
    return mCustomAudioBalance;
}

int CTvAudio::GetCurAudioBalance()
{
    return mCurAudioBalance;
}

int CTvAudio::SaveCurAudioBalance(int tmp_val)
{
    int8_t tmp_ch = 0;

    if (tmp_val < CC_MIN_SOUND_BALANCE_VAL
        || tmp_val > CC_MAX_SOUND_BALANCE_VAL) {
        tmp_val = CC_DEF_SOUND_BALANCE_VAL;
    }

    mCurAudioBalance = tmp_val;
    tmp_ch = tmp_val;
    SSMSaveAudioBalanceVal(tmp_ch);

    return mCurAudioBalance;
}

int CTvAudio::LoadCurAudioBalance()
{
    int8_t tmp_ch = 0;
    SSMReadAudioBalanceVal(&tmp_ch);
    mCurAudioBalance = tmp_ch;
    if (mCurAudioBalance < CC_MIN_SOUND_BALANCE_VAL
        || mCurAudioBalance > CC_MAX_SOUND_BALANCE_VAL) {
        SaveCurAudioBalance (CC_DEF_SOUND_BALANCE_VAL);
    }

    mCustomAudioBalance = mCurAudioBalance;

    return mCurAudioBalance;
}

int CTvAudio::SetAudioVolumeCompensationVal(int tmp_vol_comp_val)
{
    int tmp_val = 0;
    tmp_val = mVolumeCompensationVal;
    mVolumeCompensationVal = tmp_vol_comp_val;
    LOGD("%s, old vol comp value = %d, new vol comp value = %d.\n",
         __FUNCTION__, tmp_val, tmp_vol_comp_val);
    return tmp_val;
}

int CTvAudio::GetAudioVolumeCompensationVal()
{
    int tmp_vol_comp_val = 0;
    tmp_vol_comp_val = mVolumeCompensationVal;
    if (tmp_vol_comp_val < CC_VOLUME_COMPENSATION_MIN_VAL
        || tmp_vol_comp_val > CC_VOLUME_COMPENSATION_MAX_VAL) {
        tmp_vol_comp_val = CC_VOLUME_COMPENSATION_DEF_VAL;
    }
    LOGD("%s, vol comp value = %d, return value = %d.\n", __FUNCTION__,
         mVolumeCompensationVal, tmp_vol_comp_val);

    return tmp_vol_comp_val;
}

//Audio SupperBass Volume
int CTvAudio::SetAudioSupperBassVolume(int tmp_vol)
{
    if (tmp_vol < CC_MIN_SUPPERBASS_VOL || tmp_vol > CC_MAX_SUPPERBASS_VOL) {
        tmp_vol = CC_DEF_SUPPERBASS_VOL;
    }

    mCustomAudioSupperBassVolume = tmp_vol;

    return mAudioAlsa.SetDacSupperBassVolume(mCustomAudioSupperBassVolume);
}

int CTvAudio::GetAudioSupperBassVolume()
{
    return mCustomAudioSupperBassVolume;
}

int CTvAudio::GetCurAudioSupperBassVolume()
{
    return mCurAudioSupperBassVolume;
}

int CTvAudio::SaveCurAudioSupperBassVolume(int tmp_vol)
{
    int8_t tmp_ch = 0;
    if (tmp_vol < CC_MIN_SUPPERBASS_VOL || tmp_vol > CC_MAX_SUPPERBASS_VOL) {
        tmp_vol = CC_DEF_SUPPERBASS_VOL;
    }

    mCurAudioSupperBassVolume = tmp_vol;
    tmp_ch = tmp_vol;
    SSMSaveAudioSupperBassVolume(tmp_ch);

    return mCurAudioSupperBassVolume;
}

int CTvAudio::LoadCurAudioSupperBassVolume()
{
    int8_t tmp_ch = 0;
    SSMReadAudioSupperBassVolume(&tmp_ch);
    mCurAudioSupperBassVolume = tmp_ch;
    if (mCurAudioSupperBassVolume < CC_MIN_SUPPERBASS_VOL
        || mCurAudioSupperBassVolume > CC_MAX_SUPPERBASS_VOL) {
        SaveCurAudioSupperBassVolume (CC_DEF_SUPPERBASS_VOL);
    }

    mCustomAudioSupperBassVolume = mCurAudioSupperBassVolume;

    return mCurAudioSupperBassVolume;
}

//Audio SupperBass Switch
int CTvAudio::SetAudioSupperBassSwitch(int tmp_val)
{
    if (tmp_val != CC_SWITCH_ON && tmp_val != CC_SWITCH_OFF) {
        tmp_val = CC_SWITCH_OFF;
    }

    mCustomAudioSupperBassSwitch = tmp_val;

    if (GetAudioSupperBassSwitch() == CC_SWITCH_OFF) {
        return mAudioAlsa.SetDacSupperBassVolume(CC_MIN_SUPPERBASS_VOL);
    }

    return mAudioAlsa.SetDacSupperBassVolume(GetAudioSupperBassVolume());
}

int CTvAudio::GetAudioSupperBassSwitch()
{
    if (GetAudioSupperBassSwitchDisableCFG() != 0) {
        return CC_SWITCH_ON;
    }

    return mCustomAudioSupperBassSwitch;
}

int CTvAudio::GetCurAudioSupperBassSwitch()
{
    if (GetAudioSupperBassSwitchDisableCFG() != 0) {
        return CC_SWITCH_ON;
    }

    return mCurAudioSupperBassSwitch;
}

int CTvAudio::SaveCurAudioSupperBassSwitch(int tmp_val)
{
    int8_t tmp_ch = 0;
    if (tmp_val != CC_SWITCH_ON && tmp_val != CC_SWITCH_OFF) {
        tmp_val = CC_SWITCH_OFF;
    }

    mCurAudioSupperBassSwitch = tmp_val;
    tmp_ch = tmp_val;
    SSMSaveAudioSupperBassSwitch(tmp_ch);

    HandleSupperBassSwicth();

    return mCurAudioSupperBassSwitch;
}

int CTvAudio::LoadCurAudioSupperBassSwitch()
{
    int8_t tmp_ch = 0;
    SSMReadAudioSupperBassSwitch(&tmp_ch);
    mCurAudioSupperBassSwitch = tmp_ch;
    if (mCurAudioSupperBassSwitch != CC_SWITCH_ON
        && mCurAudioSupperBassSwitch != CC_SWITCH_OFF) {
        SaveCurAudioSupperBassSwitch (CC_SWITCH_OFF);
    }

    mCustomAudioSupperBassSwitch = mCurAudioSupperBassSwitch;

    HandleSupperBassSwicth();

    return mCurAudioSupperBassSwitch;
}

void CTvAudio::HanldeSupperBassSRSSpeakerSize()
{
    int tmp_speakersize = -1;

    if (GetAudioSrsTruBass() == CC_SWITCH_ON) {
        if (GetAudioSupperBassSwitch() == CC_SWITCH_ON) {
            tmp_speakersize = GetAudioSRSSupperBassTrubassSpeakerSizeCfg(1);
        } else {
            tmp_speakersize = GetAudioSRSSupperBassTrubassSpeakerSizeCfg(0);
        }

        if (tmp_speakersize >= 0) {
            mAudioEffect.SetSrsTrubassSpeakerSize(tmp_speakersize);
        }
    }
}

void CTvAudio::HandleSupperBassSwicth()
{
    if (GetAudioSupperBassModuleDisableCFG() != 0) {
        return;
    }

    HanldeSupperBassSRSSpeakerSize();
}

//Audio SRS Surround switch
int CTvAudio::SetAudioSRSSurround(int tmp_val)
{
    if (GetAudioSRSModuleDisableCFG() == 1) {
        return 0;
    }

    if (tmp_val != CC_SWITCH_ON && tmp_val != CC_SWITCH_OFF) {
        tmp_val = CC_SWITCH_OFF;
    }

    mCustomAudioSRSSurround = tmp_val;

    RealSetSRS();
    return 0;
}

int CTvAudio::GetAudioSRSSurround()
{
    if (GetAudioSRSModuleDisableCFG() == 1) {
        return CC_SWITCH_OFF;
    }

    return mCustomAudioSRSSurround;
}

int CTvAudio::GetCurAudioSRSSurround()
{
    if (GetAudioSRSModuleDisableCFG() == 1) {
        return CC_SWITCH_OFF;
    }

    return mCurAudioSRSSurround;
}

int CTvAudio::SaveCurAudioSrsSurround(int tmp_val)
{
    int8_t tmp_ch = 0;
    if (GetAudioSRSModuleDisableCFG() == 1) {
        return 0;
    }

    if (tmp_val != CC_SWITCH_ON && tmp_val != CC_SWITCH_OFF) {
        tmp_val = CC_SWITCH_OFF;
    }

    mCurAudioSRSSurround = tmp_val;
    tmp_ch = tmp_val;
    SSMSaveAudioSRSSurroundSwitch(tmp_ch);

    return mCurAudioSRSSurround;
}

int CTvAudio::LoadCurAudioSrsSurround()
{
    int8_t tmp_ch = 0;
    if (GetAudioSRSModuleDisableCFG() == 1) {
        mCustomAudioSRSSurround = CC_SWITCH_OFF;
        mCustomAudioSRSSurround = mCurAudioSRSSurround;
        return mCurAudioSRSSurround;
    }

    SSMReadAudioSRSSurroundSwitch(&tmp_ch);
    mCurAudioSRSSurround = tmp_ch;
    if (mCurAudioSRSSurround != CC_SWITCH_ON
        && mCurAudioSRSSurround != CC_SWITCH_OFF) {
        SaveCurAudioSrsSurround (CC_SWITCH_OFF);
    }

    mCustomAudioSRSSurround = mCurAudioSRSSurround;

    return mCurAudioSRSSurround;
}

//Audio SRS Dialog Clarity
int CTvAudio::SetAudioSrsDialogClarity(int tmp_val)
{
    if (GetAudioSRSModuleDisableCFG() == 1) {
        return 0;
    }

    if (tmp_val != CC_SWITCH_ON && tmp_val != CC_SWITCH_OFF) {
        tmp_val = CC_SWITCH_OFF;
    }

    mCustomAudioSrsDialogClarity = tmp_val;
    RealSetSRS();

    return 0;
}

int CTvAudio::GetAudioSrsDialogClarity()
{
    if (GetAudioSRSModuleDisableCFG() == 1) {
        return CC_SWITCH_OFF;
    }

    return mCustomAudioSrsDialogClarity;
}

int CTvAudio::GetCurAudioSrsDialogClarity()
{
    if (GetAudioSRSModuleDisableCFG() == 1) {
        return CC_SWITCH_OFF;
    }

    return mCurAudioSrsDialogClarity;
}

int CTvAudio::SaveCurAudioSrsDialogClarity(int tmp_val)
{
    int8_t tmp_ch = 0;
    if (GetAudioSRSModuleDisableCFG() == 1) {
        return 0;
    }

    if (tmp_val != CC_SWITCH_ON && tmp_val != CC_SWITCH_OFF) {
        tmp_val = CC_SWITCH_OFF;
    }

    mCurAudioSrsDialogClarity = tmp_val;
    tmp_ch = tmp_val;
    SSMSaveAudioSRSDialogClaritySwitch(tmp_ch);

    return mCurAudioSrsDialogClarity;
}

int CTvAudio::LoadCurAudioSrsDialogClarity()
{
    int8_t tmp_ch = 0;
    if (GetAudioSRSModuleDisableCFG() == 1) {
        mCurAudioSrsDialogClarity = CC_SWITCH_OFF;
        mCustomAudioSrsDialogClarity = mCurAudioSrsDialogClarity;
        return mCurAudioSRSSurround;
    }

    SSMReadAudioSRSDialogClaritySwitch(&tmp_ch);
    mCurAudioSrsDialogClarity = tmp_ch;
    if (mCurAudioSrsDialogClarity != CC_SWITCH_ON
        && mCurAudioSrsDialogClarity != CC_SWITCH_OFF) {
        SaveCurAudioSrsDialogClarity (CC_SWITCH_OFF);
    }

    mCustomAudioSrsDialogClarity = mCurAudioSrsDialogClarity;

    return mCurAudioSrsDialogClarity;
}

//Audio SRS Trubass
int CTvAudio::SetAudioSrsTruBass(int tmp_val)
{
    if (GetAudioSRSModuleDisableCFG() == 1) {
        return 0;
    }

    if (tmp_val != CC_SWITCH_ON && tmp_val != CC_SWITCH_OFF) {
        tmp_val = CC_SWITCH_OFF;
    }

    mCustomAudioSrsTruBass = tmp_val;
    RealSetSRS();

    return 0;
}

int CTvAudio::GetAudioSrsTruBass()
{
    if (GetAudioSRSModuleDisableCFG() == 1) {
        return CC_SWITCH_OFF;
    }

    return mCustomAudioSrsTruBass;
}

int CTvAudio::GetCurAudioSrsTruBass()
{
    if (GetAudioSRSModuleDisableCFG() == 1) {
        return CC_SWITCH_OFF;
    }

    return mCurAudioSrsTruBass;
}

int CTvAudio::SaveCurAudioSrsTruBass(int tmp_val)
{
    int8_t tmp_ch = 0;
    if (GetAudioSRSModuleDisableCFG() == 1) {
        return 0;
    }

    if (tmp_val != CC_SWITCH_ON && tmp_val != CC_SWITCH_OFF) {
        tmp_val = CC_SWITCH_OFF;
    }

    mCurAudioSrsTruBass = tmp_val;
    tmp_ch = tmp_val;
    SSMSaveAudioSRSTruBassSwitch(tmp_ch);

    return mCurAudioSrsTruBass;
}

int CTvAudio::LoadCurAudioSrsTruBass()
{
    int8_t tmp_ch = 0;
    if (GetAudioSRSModuleDisableCFG() == 1) {
        mCurAudioSrsTruBass = CC_SWITCH_OFF;
        mCustomAudioSrsTruBass = mCurAudioSrsTruBass;
        return mCurAudioSRSSurround;
    }

    SSMReadAudioSRSTruBassSwitch(&tmp_ch);
    mCurAudioSrsTruBass = tmp_ch;
    if (mCurAudioSrsTruBass != CC_SWITCH_ON
        && mCurAudioSrsTruBass != CC_SWITCH_OFF) {
        SaveCurAudioSrsTruBass (CC_SWITCH_OFF);
    }

    mCustomAudioSrsTruBass = mCurAudioSrsTruBass;

    return mCurAudioSrsTruBass;
}

void CTvAudio::RealSetSRS()
{
    int tmp_gain_val = 0;
    int surround_switch = CC_SWITCH_OFF;
    int trubass_switch = CC_SWITCH_OFF;
    int dialogclarity_switch = CC_SWITCH_OFF;

    surround_switch = GetAudioSRSSurround();
    if (surround_switch == CC_SWITCH_ON) {
        mAudioEffect.SetSrsSurroundSwitch(surround_switch);
        tmp_gain_val = GetAudioSRSSourroundGainCfg();
        if (tmp_gain_val >= 0 && tmp_gain_val <= 100) {
            mAudioEffect.SetSrsSurroundGain(tmp_gain_val);
        }

        trubass_switch = GetAudioSrsTruBass();
        dialogclarity_switch = GetAudioSrsDialogClarity();
        if (trubass_switch == CC_SWITCH_ON
            && dialogclarity_switch == CC_SWITCH_OFF) {
            mAudioEffect.SetSrsTruBassSwitch (CC_SWITCH_ON);

            tmp_gain_val = GetAudioSRSTrubassOnlyGainCfg();
            if (tmp_gain_val >= 0 && tmp_gain_val <= 100) {
                mAudioEffect.SetSrsTruBassGain(tmp_gain_val);
            }

            mAudioEffect.SetSrsDialogClaritySwitch (CC_SWITCH_OFF);
        } else if (trubass_switch == CC_SWITCH_OFF
                   && dialogclarity_switch == CC_SWITCH_ON) {
            mAudioEffect.SetSrsTruBassSwitch (CC_SWITCH_OFF);

            mAudioEffect.SetSrsDialogClaritySwitch (CC_SWITCH_ON);

            tmp_gain_val = GetAudioSRSDialogClarityOnlyDefinitionGainCfg();
            if (tmp_gain_val >= 0 && tmp_gain_val <= 100) {
                mAudioEffect.SetSrsDefinitionGain(tmp_gain_val);
            }

            tmp_gain_val = GetAudioSRSDialogClarityOnlyGainCfg();
            if (tmp_gain_val >= 0 && tmp_gain_val <= 100) {
                mAudioEffect.SetSrsDialogClarityGain(tmp_gain_val);
            }
        } else if (trubass_switch == CC_SWITCH_ON
                   && dialogclarity_switch == CC_SWITCH_ON) {
            mAudioEffect.SetSrsTruBassSwitch (CC_SWITCH_ON);

            tmp_gain_val = GetAudioSRSTrubassAndDialogClarityTrubassGainCfg();
            if (tmp_gain_val >= 0 && tmp_gain_val <= 100) {
                mAudioEffect.SetSrsTruBassGain(tmp_gain_val);
            }

            mAudioEffect.SetSrsDialogClaritySwitch(CC_SWITCH_ON);

            tmp_gain_val =
                GetAudioSRSTrubassAndDialogClarityDefinitionGainCfg();
            if (tmp_gain_val >= 0 && tmp_gain_val <= 100) {
                mAudioEffect.SetSrsDefinitionGain(tmp_gain_val);
            }

            tmp_gain_val =
                GetAudioSRSTrubassAndDialogClarityDialogClarityGainCfg();
            if (tmp_gain_val >= 0 && tmp_gain_val <= 100) {
                mAudioEffect.SetSrsDialogClarityGain(tmp_gain_val);
            }
        } else if (trubass_switch == CC_SWITCH_OFF
                   && dialogclarity_switch == CC_SWITCH_OFF) {
            mAudioEffect.SetSrsTruBassSwitch (CC_SWITCH_OFF);
            mAudioEffect.SetSrsDialogClaritySwitch(CC_SWITCH_OFF);
        }

        HanldeSupperBassSRSSpeakerSize();
    } else {
        mAudioEffect.SetSrsSurroundSwitch (CC_SWITCH_OFF);
        mAudioEffect.SetSrsTruBassSwitch(CC_SWITCH_OFF);
        mAudioEffect.SetSrsDialogClaritySwitch(CC_SWITCH_OFF);
    }

    SetDacGain();
}

int CTvAudio::SetDacGain()
{
    int main_gain_val = 0, sb_gain_val = 0;
    int sourround_switch, trubass_switch, dialogclarity_switch;

    sourround_switch = GetAudioSRSSurround();
    trubass_switch = GetAudioSrsTruBass();
    dialogclarity_switch = GetAudioSrsDialogClarity();

    if (sourround_switch == CC_SWITCH_ON) {
        main_gain_val = GetAudioSRSSourroundAmplifierMasterGainCfg();
        sb_gain_val = GetAudioSRSSourroundAmplifierSupperBassGainCfg();

        if (dialogclarity_switch == CC_SWITCH_ON
            && trubass_switch == CC_SWITCH_OFF) {
            main_gain_val = GetAudioSRSDialogClarityOnlyAmplifierMasterGainCfg();
            sb_gain_val = GetAudioSRSDialogClarityOnlyAmplifierSupperBassGainCfg();
        } else if (dialogclarity_switch == CC_SWITCH_OFF
                   && trubass_switch == CC_SWITCH_ON) {
            main_gain_val = GetAudioSRSTrubassOnlyAmplifierMasterGainCfg();
            sb_gain_val = GetAudioSRSTrubassOnlyAmplifierSupperBassGainCfg();
        } else if (dialogclarity_switch == CC_SWITCH_ON
                   && trubass_switch == CC_SWITCH_ON) {
            main_gain_val = GetAudioSRSTrubassAndDialogClarityAmplifierMasterGainCfg();
            sb_gain_val = GetAudioSRSTrubassAndDialogClarityAmplifierSupperBassGainCfg();
        }
    }

    mAudioAlsa.SetMainVolumeGain(main_gain_val);
    mAudioAlsa.SetSupperBassVolumeGain(sb_gain_val);

    return 0;
}

//Audio Bass
int CTvAudio::SetAudioBassVolume(int tmp_vol)
{
    int nMinBassVol = 0, nMaxBassVol = 0;

    nMinBassVol = GetBassUIMinGainVal();
    nMaxBassVol = GetBassUIMaxGainVal();

    if (tmp_vol < nMinBassVol || tmp_vol > nMaxBassVol) {
        tmp_vol = (nMaxBassVol + nMinBassVol) / 2;
    }

    mCustomAudioBassVolume = tmp_vol;

    tmp_vol = MappingTrebleBassAndEqualizer(GetAudioBassVolume(), 0,
                                            nMinBassVol, nMaxBassVol);

    return SetSpecialIndexEQGain(CC_EQ_BASS_IND, tmp_vol);
}

int CTvAudio::GetAudioBassVolume()
{
    return mCustomAudioBassVolume;
}

int CTvAudio::GetCurAudioBassVolume()
{
    return mCurAudioBassVolume;
}

int CTvAudio::SaveCurAudioBassVolume(int tmp_vol)
{
    int nMinBassVol = 0, nMaxBassVol = 0;

    nMinBassVol = GetBassUIMinGainVal();
    nMaxBassVol = GetBassUIMaxGainVal();

    if (tmp_vol < nMinBassVol || tmp_vol > nMaxBassVol) {
        tmp_vol = (nMaxBassVol + nMinBassVol) / 2;
    }

    RealSaveCurAudioBassVolume(tmp_vol, 1);

    tmp_vol = MappingTrebleBassAndEqualizer(GetCurAudioBassVolume(), 0,
                                            nMinBassVol, nMaxBassVol);

    return SaveSpecialIndexEQGain(CC_EQ_BASS_IND, tmp_vol);
}

int CTvAudio::RealSaveCurAudioBassVolume(int tmp_vol, int sound_mode_judge)
{
    int8_t tmp_ch = 0;
    mCurAudioBassVolume = tmp_vol;
    tmp_ch = tmp_vol;
    SSMSaveAudioBassVolume(tmp_ch);

    if (sound_mode_judge == 1) {
        if (GetAudioSoundMode() != CC_SOUND_MODE_USER) {
            SaveCurAudioSoundMode (CC_SOUND_MODE_USER);
            mCustomAudioSoundMode = mCurAudioSoundMode;
        }
    }

    return mCurAudioBassVolume;
}

int CTvAudio::LoadCurAudioBassVolume()
{
    int nMinBassVol = 0, nMaxBassVol = 0;
    int8_t tmp_ch = 0;

    nMinBassVol = GetBassUIMinGainVal();
    nMaxBassVol = GetBassUIMaxGainVal();

    SSMReadAudioBassVolume(&tmp_ch);
    mCurAudioBassVolume = tmp_ch;
    if (mCurAudioBassVolume < nMinBassVol
        || mCurAudioBassVolume > nMaxBassVol) {
        RealSaveCurAudioBassVolume((nMaxBassVol + nMinBassVol) / 2, 0);
    }

    mCustomAudioBassVolume = mCurAudioBassVolume;

    return mCurAudioBassVolume;
}

//Audio Treble
int CTvAudio::SetAudioTrebleVolume(int tmp_vol)
{
    int nMinTrebleVol = 0, nMaxTrebleVol = 0;

    nMinTrebleVol = GetTrebleUIMinGainVal();
    nMaxTrebleVol = GetTrebleUIMaxGainVal();

    if (tmp_vol < nMinTrebleVol || tmp_vol > nMaxTrebleVol) {
        tmp_vol = (nMaxTrebleVol + nMinTrebleVol) / 2;
    }

    mCustomAudioTrebleVolume = tmp_vol;

    tmp_vol = MappingTrebleBassAndEqualizer(GetAudioTrebleVolume(), 0,
                                            nMinTrebleVol, nMaxTrebleVol);

    return SetSpecialIndexEQGain(CC_EQ_TREBLE_IND, tmp_vol);
}

int CTvAudio::GetAudioTrebleVolume()
{
    return mCustomAudioTrebleVolume;
}

int CTvAudio::GetCurAudioTrebleVolume()
{
    return mCurAudioTrebleVolume;
}

int CTvAudio::SaveCurAudioTrebleVolume(int tmp_vol)
{
    int nMinTrebleVol = 0, nMaxTrebleVol = 0;

    nMinTrebleVol = GetTrebleUIMinGainVal();
    nMaxTrebleVol = GetTrebleUIMaxGainVal();

    if (tmp_vol < nMinTrebleVol || tmp_vol > nMaxTrebleVol) {
        tmp_vol = (nMaxTrebleVol + nMinTrebleVol) / 2;
    }

    RealSaveCurAudioTrebleVolume(tmp_vol, 1);

    tmp_vol = MappingTrebleBassAndEqualizer(GetCurAudioTrebleVolume(), 0,
                                            nMinTrebleVol, nMaxTrebleVol);

    return SaveSpecialIndexEQGain(CC_EQ_TREBLE_IND, tmp_vol);
}

int CTvAudio::RealSaveCurAudioTrebleVolume(int tmp_vol, int sound_mode_judge)
{
    int8_t tmp_ch = 0;
    mCurAudioTrebleVolume = tmp_vol;
    tmp_ch = tmp_vol;
    SSMSaveAudioTrebleVolume(tmp_ch);

    if (sound_mode_judge == 1) {
        if (GetAudioSoundMode() != CC_SOUND_MODE_USER) {
            SaveCurAudioSoundMode (CC_SOUND_MODE_USER);
            mCustomAudioSoundMode = mCurAudioSoundMode;
        }
    }

    return mCurAudioTrebleVolume;
}

int CTvAudio::LoadCurAudioTrebleVolume()
{
    int nMinTrebleVol = 0, nMaxTrebleVol = 0;
    int8_t tmp_ch = 0;

    nMinTrebleVol = GetTrebleUIMinGainVal();
    nMaxTrebleVol = GetTrebleUIMaxGainVal();

    SSMReadAudioTrebleVolume(&tmp_ch);
    mCurAudioTrebleVolume = tmp_ch;
    if (mCurAudioTrebleVolume < nMinTrebleVol
        || mCurAudioTrebleVolume > nMaxTrebleVol) {
        RealSaveCurAudioTrebleVolume((nMaxTrebleVol + nMinTrebleVol) / 2, 0);
    }

    mCustomAudioTrebleVolume = mCurAudioTrebleVolume;

    return mCurAudioTrebleVolume;
}

//Audio Sound Mode
int CTvAudio::SetAudioSoundMode(int tmp_val)
{
    if (tmp_val < CC_SOUND_MODE_START || tmp_val > CC_SOUND_MODE_END) {
        tmp_val = CC_SOUND_MODE_STD;
    }

    LOGD("%s : sound mode = %d\n", __FUNCTION__, tmp_val);

    mCustomAudioSoundMode = tmp_val;
    SetSpecialModeEQGain(mCustomAudioSoundMode);

    HandleTrebleBassVolume();

    return 0;
}

int CTvAudio::GetAudioSoundMode()
{
    return mCustomAudioSoundMode;
}

int CTvAudio::GetCurAudioSoundMode()
{
    return mCurAudioSoundMode;
}

int CTvAudio::SaveCurAudioSoundMode(int tmp_val)
{
    int8_t tmp_ch = 0;
    if (tmp_val < CC_SOUND_MODE_START || tmp_val > CC_SOUND_MODE_END) {
        tmp_val = CC_SOUND_MODE_STD;
    }

    mCurAudioSoundMode = tmp_val;
    tmp_ch = tmp_val;
    SSMSaveAudioSoundModeVal(tmp_ch);

    return mCurAudioSoundMode;
}

int CTvAudio::LoadCurAudioSoundMode()
{
    int8_t tmp_ch = 0;
    SSMReadAudioSoundModeVal(&tmp_ch);
    mCurAudioSoundMode = tmp_ch;
    if (mCurAudioSoundMode < CC_SOUND_MODE_START
        || mCurAudioSoundMode > CC_SOUND_MODE_END) {
        SaveCurAudioSoundMode (CC_SOUND_MODE_STD);
    }

    mCustomAudioSoundMode = mCurAudioSoundMode;

    return mCurAudioSoundMode;
}

int CTvAudio::HandleTrebleBassVolume()
{
    int tmp_vol = 0;
    int tmpEQGainBuf[128] = { 0 };
    int8_t tmp_ch = 0;

    GetCustomEQGain(tmpEQGainBuf);

    tmp_vol = MappingTrebleBassAndEqualizer(tmpEQGainBuf[CC_EQ_TREBLE_IND], 1,
                                            GetTrebleUIMinGainVal(), GetTrebleUIMaxGainVal());
    mCustomAudioTrebleVolume = tmp_vol;
    mCurAudioTrebleVolume = mCustomAudioTrebleVolume;
    tmp_ch = mCustomAudioTrebleVolume;
    SSMSaveAudioTrebleVolume(tmp_ch);

    tmp_vol = MappingTrebleBassAndEqualizer(tmpEQGainBuf[CC_EQ_BASS_IND], 1,
                                            GetBassUIMinGainVal(), GetBassUIMaxGainVal());
    mCustomAudioBassVolume = tmp_vol;
    mCurAudioBassVolume = mCustomAudioBassVolume;
    tmp_ch = mCustomAudioBassVolume;
    SSMSaveAudioBassVolume(tmp_ch);

    return 0;
}

//Audio Wall Effect
int CTvAudio::SetAudioWallEffect(int tmp_val)
{
    int tmp_treble_val;
    int tmp_type = 0;

    if (tmp_val != CC_SWITCH_ON && tmp_val != CC_SWITCH_OFF) {
        tmp_val = CC_SWITCH_OFF;
    }

    mCustomAudioWallEffect = tmp_val;

    tmp_type = GetAudioWallEffectTypeCfg();
    if (tmp_type == 0) {
        SetCustomEQGain();
    } else if (tmp_type == 1) {
        mAudioAlsa.SetDacEQMode(tmp_val);
    }

    return 0;
}

int CTvAudio::GetAudioWallEffect()
{
    return mCustomAudioWallEffect;
}

int CTvAudio::GetCurAudioWallEffect()
{
    return mCurAudioWallEffect;
}

int CTvAudio::SaveCurAudioWallEffect(int tmp_val)
{
    int8_t tmp_ch = 0;
    if (tmp_val != CC_SWITCH_ON && tmp_val != CC_SWITCH_OFF) {
        tmp_val = CC_SWITCH_OFF;
    }

    mCurAudioWallEffect = tmp_val;
    tmp_ch = tmp_val;
    SSMSaveAudioWallEffectSwitch(tmp_ch);

    return mCurAudioWallEffect;
}

int CTvAudio::LoadCurAudioWallEffect()
{
    int8_t tmp_ch = 0;
    SSMReadAudioWallEffectSwitch(&tmp_ch);
    mCurAudioWallEffect = tmp_ch;
    if (mCurAudioWallEffect != CC_SWITCH_ON
        && mCurAudioWallEffect != CC_SWITCH_OFF) {
        SaveCurAudioWallEffect (CC_SWITCH_OFF);
    }

    mCustomAudioWallEffect = mCurAudioWallEffect;

    return mCurAudioWallEffect;
}

//Audio EQ Mode
int CTvAudio::SetAudioEQMode(int tmp_val)
{
    if (tmp_val < CC_EQ_MODE_START || tmp_val > CC_EQ_MODE_END) {
        tmp_val = CC_EQ_MODE_START;
    }

    mCustomAudioEQMode = tmp_val;

    return 0;
}

int CTvAudio::GetAudioEQMode()
{
    return mCustomAudioEQMode;
}

int CTvAudio::GetCurAudioEQMode()
{
    return mCurAudioEQMode;
}

int CTvAudio::SaveCurAudioEQMode(int tmp_val)
{
    int8_t tmp_ch = 0;
    if (tmp_val < CC_EQ_MODE_START || tmp_val > CC_EQ_MODE_END) {
        tmp_val = CC_EQ_MODE_START;
    }

    mCurAudioEQMode = tmp_val;
    tmp_ch = tmp_val;
    SSMSaveAudioEQModeVal(tmp_ch);

    return tmp_val;
}

int CTvAudio::LoadCurAudioEQMode()
{
    int8_t tmp_ch = 0;
    SSMReadAudioEQModeVal(&tmp_ch);
    mCurAudioEQMode = tmp_ch;
    if (mCurAudioEQMode < CC_EQ_MODE_START
        || mCurAudioEQMode > CC_EQ_MODE_END) {
        SaveCurAudioEQMode (CC_EQ_MODE_START);
    }

    mCustomAudioEQMode = mCurAudioEQMode;

    return mCurAudioEQMode;
}

//Audio EQ Gain
int CTvAudio::GetAudioEQRange(int range_buf[])
{
    range_buf[0] = GetEQMinGainVal();
    range_buf[1] = GetEQMaxGainVal();
    return 0;
}

int CTvAudio::GetAudioEQBandCount()
{
    return mAudioEffect.GetEQBandCount();
}

int CTvAudio::SetAudioEQGain(int gain_buf[])
{
    return AudioSetEQGain(gain_buf);
}

int CTvAudio::GetAudioEQGain(int gain_buf[])
{
    return GetCustomEQGain(gain_buf);
}

int CTvAudio::GetCurAudioEQGain(int gain_buf[])
{
    RealReadCurAudioEQGain(gain_buf);
    return 0;
}

int CTvAudio::SaveCurAudioEQGain(int gain_buf[])
{
    return RealSaveCurAudioEQGain(gain_buf, 1);
}

int CTvAudio::RealReadCurAudioEQGain(int gain_buf[])
{
    if (CheckAudioEQGain(mCurEQGainBuf, 0) < 0) {
        RestoreToAudioDefEQGain(mCurEQGainBuf);
    }

    AudioArrayCopy(gain_buf, mCurEQGainBuf, GetAudioEQBandCount());

    return 0;
}

int CTvAudio::RealSaveCurAudioEQGain(int gain_buf[], int sound_mode_judge)
{
    if (CheckAudioEQGain(gain_buf, 0) < 0) {
        RestoreToAudioDefEQGain(gain_buf);
        SetAudioEQGain(gain_buf);
        return -1;
    }

    AudioArrayCopy(mCurEQGainBuf, gain_buf, GetAudioEQBandCount());
    AudioArrayCopy(mCurEQGainChBuf, gain_buf, GetAudioEQBandCount());
    SSMSaveAudioEQGain(0, GetAudioEQBandCount(), mCurEQGainChBuf);

    if (sound_mode_judge == 1) {
        HandleTrebleBassVolume();
        SaveCurAudioSoundMode (CC_SOUND_MODE_USER);
        mCustomAudioSoundMode = mCurAudioSoundMode;
    }

    return 0;
}

int CTvAudio::LoadCurAudioEQGain()
{
    int i = 0;

    SSMReadAudioEQGain(0, GetAudioEQBandCount(), mCurEQGainChBuf);
    AudioArrayCopy(mCurEQGainBuf, mCurEQGainChBuf, GetAudioEQBandCount());

    for (i = 0; i < GetAudioEQBandCount(); i++) {
        if (mCurEQGainBuf[i] & 0x80) {
            mCurEQGainBuf[i] = mCurEQGainBuf[i] - 256;
        }
    }

    if (CheckAudioEQGain(mCurEQGainBuf, 0) < 0) {
        RestoreToAudioDefEQGain(mCurEQGainBuf);
        SetAudioEQGain(mCurEQGainBuf);
        return -1;
    }

    return 0;
}

int CTvAudio::SetAudioEQSwitch(int switch_val)
{
    return mAudioEffect.SetEQSwitch(switch_val);
}

int CTvAudio::GetEQMinGainVal()
{
    int min_val = 0, max_val = 0;

    if (GetAudioEQGainRangeCFG(&min_val, &max_val) < 0) {
        return mAudioEffect.GetEQMinGain();
    }

    return min_val;
}

int CTvAudio::GetEQMaxGainVal()
{
    int min_val = 0, max_val = 0;

    if (GetAudioEQGainRangeCFG(&min_val, &max_val) < 0) {
        return mAudioEffect.GetEQMaxGain();
    }

    return max_val;
}

int CTvAudio::GetEQUIMinGainVal()
{
    int min_val = 0, max_val = 0;

    if (GetAudioEQUIGainRangeCFG(&min_val, &max_val) < 0) {
        return CC_EQ_DEF_UI_MIN_GAIN;
    }

    return min_val;
}

int CTvAudio::GetEQUIMaxGainVal()
{
    int min_val = 0, max_val = 0;

    if (GetAudioEQUIGainRangeCFG(&min_val, &max_val) < 0) {
        return CC_EQ_DEF_UI_MAX_GAIN;
    }

    return max_val;
}

int CTvAudio::GetBassUIMinGainVal()
{
    return 0;
}

int CTvAudio::GetBassUIMaxGainVal()
{
    return 100;
}

int CTvAudio::GetTrebleUIMinGainVal()
{
    return 0;
}

int CTvAudio::GetTrebleUIMaxGainVal()
{
    return 100;
}

int CTvAudio::MappingLine(int map_val, int src_min, int src_max, int dst_min,
                          int dst_max)
{
    if (dst_min < 0) {
        return (map_val - (src_max + src_min) / 2) * (dst_max - dst_min)
               / (src_max - src_min);
    } else {
        return (map_val - src_min) * (dst_max - dst_min) / (src_max - src_min);
    }
}

int CTvAudio::MappingTrebleBassAndEqualizer(int tmp_vol, int direct, int tb_min,
        int tb_max)
{
    int tmp_ret = 0;

    if (direct == 0) {
        tmp_ret = MappingLine(tmp_vol, tb_min, tb_max, GetEQUIMinGainVal(),
                              GetEQUIMaxGainVal());
    } else {
        tmp_ret = MappingLine(tmp_vol, GetEQUIMinGainVal(), GetEQUIMaxGainVal(),
                              tb_min, tb_max);
    }

    LOGD("%s, tmp_vol = %d, direct = %d, tmp_ret = %d\n", __FUNCTION__, tmp_vol,
         direct, tmp_ret);

    return tmp_ret;
}

int CTvAudio::MappingEQGain(int src_gain_buf[], int dst_gain_buf[], int direct)
{
    int i = 0;
    int nMinUIVal = 0, nMaxUIVal = 0, nMinVal = 0, nMaxVal = 0;

    nMinUIVal = GetEQUIMinGainVal();
    nMaxUIVal = GetEQUIMaxGainVal();
    nMinVal = GetEQMinGainVal();
    nMaxVal = GetEQMaxGainVal();

    if (CheckAudioEQGain(src_gain_buf, direct) < 0) {
        return -1;
    }

    if (nMinUIVal >= nMinVal && nMaxUIVal <= nMaxVal) {
        for (i = 0; i < GetAudioEQBandCount(); i++) {
            dst_gain_buf[i] = src_gain_buf[i];
        }
    } else {
        for (i = 0; i < GetAudioEQBandCount(); i++) {
            if (direct == 0) {
                dst_gain_buf[i] = MappingLine(src_gain_buf[i], nMinUIVal,
                                              nMaxUIVal, nMinVal, nMaxVal);
            } else {
                dst_gain_buf[i] = MappingLine(src_gain_buf[i], nMinVal, nMaxVal,
                                              nMinUIVal, nMaxUIVal);
            }
        }
    }

    return 0;
}

int CTvAudio::RestoreToAudioDefEQGain(int gain_buf[])
{
    int i = 0;
    int *tmpEQPresetBufPtr = GetAudioEQPresetBufferPtr();

    for (i = 0; i < GetAudioEQBandCount(); i++) {
        gain_buf[i] = (GetEQUIMaxGainVal() + GetEQUIMinGainVal()) / 2;
    }

    AudioArrayCopy(mCurEQGainBuf, gain_buf, GetAudioEQBandCount());
    AudioArrayCopy(mCurEQGainChBuf, gain_buf, GetAudioEQBandCount());
    SSMSaveAudioEQGain(0, GetAudioEQBandCount(), mCurEQGainChBuf);

    HandleTrebleBassVolume();
    SaveCurAudioSoundMode (CC_SOUND_MODE_STD);
    mCustomAudioSoundMode = mCurAudioSoundMode;

    return 0;
}

int CTvAudio::CheckAudioEQGain(int gain_buf[], int direct)
{
    int i;

    if (direct == 0) {
        for (i = 0; i < GetAudioEQBandCount(); i++) {
            if (gain_buf[i] < GetEQUIMinGainVal()
                || gain_buf[i] > GetEQUIMaxGainVal()) {
                return -1;
            }
        }
    } else {
        for (i = 0; i < GetAudioEQBandCount(); i++) {
            if (gain_buf[i] < GetEQMinGainVal()
                || gain_buf[i] > GetEQMaxGainVal()) {
                return -1;
            }
        }
    }

    return 0;
}

int CTvAudio::GetCustomEQGain(int gain_buf[])
{
    AudioArrayCopy(gain_buf, mCustomEQGainBuf, GetAudioEQBandCount());
    return 0;
}

int CTvAudio::SetCustomEQGain()
{
    int tmpEQGainBuf[128] = { 0 };

    if (MappingEQGain(mCustomEQGainBuf, tmpEQGainBuf, 0) < 0) {
        return -1;
    }

    return RealSetEQGain(tmpEQGainBuf);
}

int CTvAudio::AudioSetEQGain(int gain_buf[])
{
    int tmpEQGainBuf[128] = { 0 };

    AudioArrayCopy(mCustomEQGainBuf, gain_buf, GetAudioEQBandCount());

    if (MappingEQGain(mCustomEQGainBuf, tmpEQGainBuf, 0) < 0) {
        return -1;
    }

    return RealSetEQGain(tmpEQGainBuf);
}

int CTvAudio::handleEQGainBeforeSet(int src_buf[], int dst_buf[])
{
    int i = 0, nMinGain, nMaxGain, handle_wall_effect_flag;

    nMinGain = GetEQMinGainVal();
    nMaxGain = GetEQMaxGainVal();

    handle_wall_effect_flag = 0;

    if (GetAudioWallEffect() == CC_SWITCH_ON) {
        if (GetAudioWallEffectTypeCfg() == 0) {
            handle_wall_effect_flag = 1;
        }
    }

    if (handle_wall_effect_flag) {
        for (i = 0; i < GetAudioEQBandCount(); i++) {
            dst_buf[i] = mWallEffectValueBuf[i] + src_buf[i];
        }
    } else {
        for (i = 0; i < GetAudioEQBandCount(); i++) {
            dst_buf[i] = src_buf[i];
        }
    }

    for (i = 0; i < GetAudioEQBandCount(); i++) {
        if (dst_buf[i] < nMinGain) {
            dst_buf[i] = nMinGain;
        }

        if (dst_buf[i] > nMaxGain) {
            dst_buf[i] = nMaxGain;
        }

        LOGD("%s, dst_buf[%d] = %d\n", __FUNCTION__, i, dst_buf[i]);
    }

    return 0;
}

int CTvAudio::RealSetEQGain(int gain_buf[])
{
    int tmpEQGainBuf[128] = { 0 };

    if (CheckAudioEQGain(gain_buf, 1) < 0) {
        return -1;
    }

    handleEQGainBeforeSet(gain_buf, tmpEQGainBuf);

    mAudioEffect.SetEQValue(tmpEQGainBuf);

    return 0;
}

int CTvAudio::SetSpecialModeEQGain(int tmp_val)
{
    int i = 0;
    int *tmpEQPresetBufPtr = GetAudioEQPresetBufferPtr();
    int tmpEQGainBuf[128] = { 0 };

    if (tmp_val < CC_SOUND_MODE_START || tmp_val > CC_SOUND_MODE_END) {
        tmp_val = CC_SOUND_MODE_STD;
    }

    if (tmp_val == CC_SOUND_MODE_USER) {
        RealReadCurAudioEQGain(tmpEQGainBuf);
    } else {
        AudioArrayCopy(tmpEQGainBuf,
                       tmpEQPresetBufPtr + tmp_val * GetAudioEQBandCount(),
                       GetAudioEQBandCount());

        for (i = 0; i < GetAudioEQBandCount(); i++) {
            if (GetEQUIMinGainVal() != CC_EQ_DEF_UI_MIN_GAIN
                || GetEQUIMaxGainVal() != CC_EQ_DEF_UI_MAX_GAIN) {
                tmpEQGainBuf[i] = MappingLine(tmpEQGainBuf[i],
                                              CC_EQ_DEF_UI_MIN_GAIN, CC_EQ_DEF_UI_MAX_GAIN,
                                              GetEQUIMinGainVal(), GetEQUIMaxGainVal());
            }
        }
    }

    AudioSetEQGain(tmpEQGainBuf);

    return tmp_val;
}

int CTvAudio::SetSpecialIndexEQGain(int buf_index, int w_val)
{
    int tmpEQGainBuf[128] = { 0 };

    if (buf_index >= 0 && buf_index < GetAudioEQBandCount()) {
        RealReadCurAudioEQGain(tmpEQGainBuf);
        tmpEQGainBuf[buf_index] = w_val;

        return AudioSetEQGain(tmpEQGainBuf);
    }

    return -1;
}

int CTvAudio::SaveSpecialIndexEQGain(int buf_index, int w_val)
{
    int tmpEQGainBuf[128] = { 0 };

    if (buf_index >= 0 && buf_index < GetAudioEQBandCount()) {
        RealReadCurAudioEQGain(tmpEQGainBuf);
        tmpEQGainBuf[buf_index] = w_val;

        return RealSaveCurAudioEQGain(tmpEQGainBuf, 0);
    }

    return 0;
}

// amAudio
int CTvAudio::OpenAmAudio(unsigned int sr, int output_device)
{
    if (GetamAudioDisableFlagCFG() == 1) {
        return 0;
    }

    return amAudioOpen(sr, CC_IN_USE_I2S_DEVICE, output_device);
}

int CTvAudio::CloseAmAudio(void)
{
    if (GetamAudioDisableFlagCFG() == 1) {
        return 0;
    }

    return amAudioClose();
}

int CTvAudio::SetAmAudioInputSr(unsigned int sr, int output_device)
{
    return amAudioSetInputSr(sr, CC_IN_USE_I2S_DEVICE, output_device);
}

int CTvAudio::SetAmAudioOutputMode(int mode)
{
    if (mode != CC_AMAUDIO_OUT_MODE_MIX && mode != CC_AMAUDIO_OUT_MODE_DIRECT) {
        LOGE("%s, mode error, it should be mix or direct!\n", __FUNCTION__);
        return -1;
    }

    return amAudioSetOutputMode(mode);
}

int CTvAudio::SetAmAudioMusicGain(int gain)
{
    return amAudioSetMusicGain(gain);
}

int CTvAudio::SetAmAudioLeftGain(int gain)
{
    return amAudioSetLeftGain(gain);
}

int CTvAudio::SetAmAudioRightGain(int gain)
{
    return amAudioSetRightGain(gain);
}

//dump data
int CTvAudio::SetAudioDumpDataFlag(int tmp_flag)
{
#if CC_TV_AUDIO_TYPE_ANDROID == 1
    return amAndroidSetDumpDataFlag(tmp_flag);
#endif

#ifdef CC_TV_AUDIO_TYPE_AMAUDIO == 1
    return amAudioSetDumpDataFlag(tmp_flag);
#endif
}

int GetAudioDumpDataFlag()
{
#if CC_TV_AUDIO_TYPE_ANDROID == 1
    return amAndroidGetDumpDataFlag();
#endif

#ifdef CC_TV_AUDIO_TYPE_AMAUDIO == 1
    return amAudioGetDumpDataFlag();
#endif
}

//LUT buffer
static int RealCalVolDigitLUTBuf(int start_ind, int end_ind, int start_val,
                                 int end_val, int lut_buf[])
{
    int i = 0;
    float tmp_step = 0;

    if (end_ind < start_ind) {
        return 0;
    }

    tmp_step = (float) (end_val - start_val) / (float) (end_ind - start_ind);

    for (i = start_ind; i < end_ind; i++) {
        lut_buf[i] = start_val + (int) (tmp_step * (i - start_ind));
    }

    lut_buf[end_ind] = end_val;

    return 1;
}

static int CalVolumeDigitLUTBuf(int lut_buf_type, int lut_sel_flag, int lut_buf[])
{
    int i = 0, point_cnt = 0;
    int *tmpNoLinePointsDataBuffer = NULL;
    int *tmpNoLinePointsIndexBuffer = NULL;

    if (lut_sel_flag == CC_LUT_SEL_TV) {
        tmpNoLinePointsIndexBuffer = GetAudioAmplifierNoLinePointsIndexBuffer(
                                         lut_buf_type, CC_GET_LUT_TV);
        tmpNoLinePointsDataBuffer = GetAudioAmplifierNoLinePointsDataBuffer(
                                        lut_buf_type, CC_GET_LUT_TV);
    } else if (lut_sel_flag == CC_LUT_SEL_AV) {
        tmpNoLinePointsIndexBuffer = GetAudioAmplifierNoLinePointsIndexBuffer(
                                         lut_buf_type, CC_GET_LUT_AV);
        tmpNoLinePointsDataBuffer = GetAudioAmplifierNoLinePointsDataBuffer(
                                        lut_buf_type, CC_GET_LUT_AV);
    } else if (lut_sel_flag == CC_LUT_SEL_COMP) {
        tmpNoLinePointsIndexBuffer = GetAudioAmplifierNoLinePointsIndexBuffer(
                                         lut_buf_type, CC_GET_LUT_COMP);
        tmpNoLinePointsDataBuffer = GetAudioAmplifierNoLinePointsDataBuffer(
                                        lut_buf_type, CC_GET_LUT_COMP);
    } else if (lut_sel_flag == CC_LUT_SEL_HDMI) {
        tmpNoLinePointsIndexBuffer = GetAudioAmplifierNoLinePointsIndexBuffer(
                                         lut_buf_type, CC_GET_LUT_HDMI);
        tmpNoLinePointsDataBuffer = GetAudioAmplifierNoLinePointsDataBuffer(
                                        lut_buf_type, CC_GET_LUT_HDMI);
    } else if (lut_sel_flag == CC_LUT_SEL_VGA) {
        tmpNoLinePointsIndexBuffer = GetAudioAmplifierNoLinePointsIndexBuffer(
                                         lut_buf_type, CC_GET_LUT_VGA);
        tmpNoLinePointsDataBuffer = GetAudioAmplifierNoLinePointsDataBuffer(
                                        lut_buf_type, CC_GET_LUT_VGA);
    } else if (lut_sel_flag == CC_LUT_SEL_MPEG) {
        tmpNoLinePointsIndexBuffer = GetAudioAmplifierNoLinePointsIndexBuffer(
                                         lut_buf_type, CC_GET_LUT_MPEG);
        tmpNoLinePointsDataBuffer = GetAudioAmplifierNoLinePointsDataBuffer(
                                        lut_buf_type, CC_GET_LUT_MPEG);
    }

    lut_buf[0] = CC_MIN_DAC_VOLUME;
    point_cnt = GetAudioAmplifierMasterNoLinePointsCount();

    if (tmpNoLinePointsIndexBuffer != NULL
        && tmpNoLinePointsDataBuffer != NULL) {
        for (i = 0; i < (point_cnt - 1); i++) {
            RealCalVolDigitLUTBuf(tmpNoLinePointsIndexBuffer[i],
                                  tmpNoLinePointsIndexBuffer[i + 1],
                                  tmpNoLinePointsDataBuffer[i],
                                  tmpNoLinePointsDataBuffer[i + 1], lut_buf);
        }
    }

    return 1;
}

void CTvAudio::RealHandleVolumeDigitLUTBuf(int lut_buf_type, int lut_sel_flag,
        int vol_buf[], int add_srs_gain_flag, int add_eq_gain_flag)
{
    int i;
    int TempVolumeDigitLutBuf[CC_LUT_BUF_SIZE] = { 0 };

    if (vol_buf == NULL) {
        CalVolumeDigitLUTBuf(lut_buf_type, lut_sel_flag, TempVolumeDigitLutBuf);
    } else {
        for (i = 0; i < CC_LUT_BUF_SIZE; i++) {
            TempVolumeDigitLutBuf[i] = vol_buf[i];
        }
    }

    if (lut_buf_type == CC_LUT_BUF_MASTER) {
        mAudioAlsa.SetMainVolDigitLutBuf(TempVolumeDigitLutBuf);
    } else if (lut_buf_type == CC_LUT_BUF_SUPPERBASS) {
        mAudioAlsa.SetSupperBassVolDigitLutBuf(TempVolumeDigitLutBuf);
    }
}

void CTvAudio::AudioSetVolumeDigitLUTBuf(int lut_sel_flag, int add_srs_gain_flag,
        int add_eq_gain_flag, int audio_std)
{
    int cfg_get_index = CC_LUT_SEL_MPEG;
    int *tmpDefDigitLutBuf = NULL;

    if (lut_sel_flag == CC_LUT_SEL_TV) {
        cfg_get_index = CC_GET_LUT_TV;
    } else if (lut_sel_flag == CC_LUT_SEL_AV) {
        cfg_get_index = CC_GET_LUT_AV;
    } else if (lut_sel_flag == CC_LUT_SEL_COMP) {
        cfg_get_index = CC_GET_LUT_COMP;
    } else if (lut_sel_flag == CC_LUT_SEL_HDMI) {
        cfg_get_index = CC_GET_LUT_HDMI;
    } else if (lut_sel_flag == CC_LUT_SEL_VGA) {
        cfg_get_index = CC_GET_LUT_VGA;
    } else if (lut_sel_flag == CC_LUT_SEL_MPEG) {
        cfg_get_index = CC_GET_LUT_MPEG;
    }

    if (GetAudioAmplifierMasterNoLineSwitchFlag() == 0) {
        tmpDefDigitLutBuf = GetAudioAmplifierMasterVolumeLUTBufferPtr(
                                cfg_get_index, audio_std);
        if (tmpDefDigitLutBuf != NULL) {
            RealHandleVolumeDigitLUTBuf(CC_LUT_BUF_MASTER, lut_sel_flag,
                                        tmpDefDigitLutBuf, add_srs_gain_flag, add_eq_gain_flag);
        }
    } else {
        RealHandleVolumeDigitLUTBuf(CC_LUT_BUF_MASTER, lut_sel_flag, NULL,
                                    add_srs_gain_flag, add_eq_gain_flag);
    }

    if (GetAudioSupperBassModuleDisableCFG() == 0) {
        if (GetAudioAmplifierSupperBassNoLineSwitchFlag() == 0) {
            tmpDefDigitLutBuf = GetAudioAmplifierSupperBassVolumeLUTBufferPtr(
                                    cfg_get_index, audio_std);
            if (tmpDefDigitLutBuf != NULL) {
                RealHandleVolumeDigitLUTBuf(CC_LUT_BUF_SUPPERBASS, lut_sel_flag,
                                            tmpDefDigitLutBuf, add_srs_gain_flag, add_eq_gain_flag);
            }
        } else {
            RealHandleVolumeDigitLUTBuf(CC_LUT_BUF_SUPPERBASS, lut_sel_flag,
                                        NULL, add_srs_gain_flag, add_eq_gain_flag);
        }
    }
}

int CTvAudio::openTvAudio(int sr)
{

    char prop1[256];
    char prop2[256];
    property_get("audio.headset_plug.enable", prop1, "null");
    property_get("audio.output.double_output", prop2, "null");
    if (strcmp(prop1, "1") == 0) {
        if (strcmp(prop2, "1") == 0) {
            amAudioSetTvDoubleOutput(1, sr, CC_IN_USE_I2S_DEVICE, CC_OUT_USE_AMAUDIO);
            audio_device_switch = 1;
        } else {
            amAudioSetTvDoubleOutput(0, sr, CC_IN_USE_I2S_DEVICE, CC_OUT_USE_ANDROID);
            audio_device_switch = 0;
        }
    } else {
        amAudioSetTvDoubleOutput(1, sr, CC_IN_USE_I2S_DEVICE, CC_OUT_USE_AMAUDIO);
        audio_device_switch = 1;
    }
    return 0;
}

// audio data init & uninit
int CTvAudio::AudioDataInitForDtv()
{
    RealSetSRS();
    SetCustomEQGain();

    return 0;
}

int CTvAudio::AudioDataInit(int sr, bool enableNoiseGate)
{
    const char *value;
    sr = HanldeAudioInputSr(sr);
#if CC_TV_AUDIO_TYPE_ANDROID == 1
    if (GetDisableResetSystemFrameSize() == 0) {
        if (GetDisableReset_System_framesizeCFG() != 0x55)
            reset_system_framesize(48000, sr);
    }

    amAndroidInit(0, CC_FLAG_CREATE_RECORD | CC_FLAG_CREATE_TRACK, 48000, sr, enableNoiseGate);
#endif

#ifdef CC_TV_AUDIO_TYPE_AMAUDIO == 1
    value = config_get_str ( "TV", "tvin.2d4G.headset.en", "null" );
    if ( strcmp ( value, "enable" ) == 0 )
        openTvAudio(sr);
    else
        OpenAmAudio(sr, CC_OUT_USE_AMAUDIO);
#endif

    RealSetSRS();
    SetCustomEQGain();

    return 0;
}

int CTvAudio::AudioDataUnInit()
{
#if CC_TV_AUDIO_TYPE_ANDROID == 1
    return amAndroidUninit(0);
#endif

#ifdef CC_TV_AUDIO_TYPE_AMAUDIO == 1
    return CloseAmAudio();
#endif
}

int CTvAudio::AudioChangeSampleRate(int sr)
{
    sr = HanldeAudioInputSr(sr);

#if CC_TV_AUDIO_TYPE_ANDROID == 1
    if (amAndroidSetTrackerSr(sr) != 0) {
        return -1;
    }
#endif

#ifdef CC_TV_AUDIO_TYPE_AMAUDIO == 1
    if (SetAmAudioInputSr(sr, CC_OUT_USE_AMAUDIO) != 0) {
        return -1;
    }
#endif

    RealSetSRS();
    SetCustomEQGain();

    return 0;
}

int CTvAudio::AudioNewAudioEffects(void)
{
    return mAudioEffect.amAndroidNewAudioEffects();
}

void CTvAudio::AudioFreeAudioEffects(void)
{
    return mAudioEffect.amAndroidFreeAudioEffects();
}

int CTvAudio::AudioSetAudioInSource(int audio_src_in_type)
{
    return mAudioAlsa.SetAudioInSource(audio_src_in_type);
}

int CTvAudio::AudioSetAudioSourceType(int source_type)
{
    return mAudioAlsa.SetDacAudioSourceType(source_type);
}

int CTvAudio::AudioLineInSelectChannel(int audio_channel)
{
    LOGD ("%s, audio_channel = %d", __FUNCTION__, audio_channel);
    mAudioAlsa.SetInternalDacLineInSelectChannel(audio_channel);
    return 0;
}
int CTvAudio::SetKalaokIO(int level)
{
    LOGD ("%s, SetKalaokIO = %d", __FUNCTION__, level);
    {
        const char *config_value;
        config_value = config_get_str("TV", "audio.switch.karaok.av.enable", "null");

        if (strtoul(config_value, NULL, 10) == 1) {
            mAudioAlsa.SetAudioSwitchIO(level);
        }
    }
    return 0;
}

int CTvAudio::AudioSetLineInCaptureVolume(int l_vol, int r_vol)
{
    mAudioAlsa.SetInternalDacLineInCaptureVolume(l_vol, r_vol);
    return 0;
}

// Audio Ctl Init
void CTvAudio::AudioCtlInitializeLoad()
{
    int tmp_val = 0;
    int init_mute_type = 0;

    LOGD("%s, entering...\n", __FUNCTION__);

    createMonitorUsbHostBusThread();

    init_mute_type = GetAudioAVOutInitMuteStatusCFG();

    UnInitSetTvAudioCard();

    if (init_mute_type == 1) {
        SetAudioAVOutMute (CC_AUDIO_MUTE);
    } else {
        SetAudioAVOutMute (CC_AUDIO_UNMUTE);
    }

    SetAudioSPDIFMute (CC_AUDIO_UNMUTE);

    tmp_val = GetAudioDumpDataEnableFlagCfg();
    SetAudioDumpDataFlag(tmp_val);

    mAudioAlsa.SetMixerBypassSwitch (CC_SWITCH_OFF);
    mAudioAlsa.SetMixerDacSwitch (CC_SWITCH_ON);

    LoadAudioCtl();

    AudioSetVolumeDigitLUTBuf(CC_LUT_SEL_MPEG, 1, 1, -1);

    AudioCtlInitializeSet();
}

void CTvAudio::LoadAudioCtl()
{
    // Get Current Audio Volume
    LoadCurAudioMasterVolume();

    // Get Current Audio Balance
    LoadCurAudioBalance();

    // Get Current Supper Bass Switch
    LoadCurAudioSupperBassSwitch();

    // Get Current Supper Bass Volume
    LoadCurAudioSupperBassVolume();

    // Get Current SRSSurround
    LoadCurAudioSrsSurround();

    // Get Current SRS DialogClarity
    LoadCurAudioSrsDialogClarity();

    // Get Current SRS TruBass
    LoadCurAudioSrsTruBass();

    // Get Current Audio Sound Mode
    LoadCurAudioSoundMode();

    // Get Current Audio Bass and Treble
    LoadCurAudioBassVolume();
    LoadCurAudioTrebleVolume();

    // Get Current Wall Effect
    LoadCurAudioWallEffect();

    // Get Current spdif switch
    LoadCurAudioSPDIFSwitch();

    // Get Current spdif mode
    LoadCurAudioSPDIFMode();

    // Get Current EQ mode
    LoadCurAudioEQMode();

    // Get Current EQ Gain
    LoadCurAudioEQGain();
}

void CTvAudio::InitSetAudioCtl()
{
    // Set Current Audio balance value
    SetAudioBalance(GetAudioBalance());

    // Set Current Audio Volume
    SetAudioMasterVolume(GetAudioMasterVolume());

    // Set Current Supper Bass Volume
    SetAudioSupperBassVolume(GetAudioSupperBassVolume());

    // Set Current Audio Sound Mode
    SetAudioSoundMode(GetAudioSoundMode());

    // Set Current Audio SPDIF Switch
    SetAudioSPDIFSwitch(GetCurAudioSPDIFSwitch());

    // Set Current Audio SPDIF mode
    SetAudioSPDIFMode(GetCurAudioSPDIFMode());
}

static int gAudioFirstInitSetFlag = 0;
static pthread_mutex_t first_init_set_flag_mutex = PTHREAD_MUTEX_INITIALIZER;

int CTvAudio::GetAudioFirstInitSetFlag()
{
    int tmp_val = 0;

    pthread_mutex_lock(&first_init_set_flag_mutex);
    tmp_val = gAudioFirstInitSetFlag;
    pthread_mutex_unlock(&first_init_set_flag_mutex);

    return tmp_val;
}

void *CTvAudio::AudioCtlInitSetMain(void *data)
{
    char prop_value[PROPERTY_VALUE_MAX];

    LOGD("%s, entering...\n", __FUNCTION__);
    pthread_mutex_lock(&first_init_set_flag_mutex);

    if (gAudioFirstInitSetFlag == 0) {
        AudioNewAudioEffects();
        RealSetSRS();
        AudioSetVolumeDigitLUTBuf(CC_LUT_SEL_MPEG, 1, 1, -1);
        InitSetAudioCtl();
        gAudioFirstInitSetFlag = 1;
    }

    pthread_mutex_unlock(&first_init_set_flag_mutex);

    LOGD("%s, exiting...\n", __FUNCTION__);

    return NULL;
}

void CTvAudio::AudioCtlInitializeSet()
{
    AudioCtlInitSetMain (NULL);
    return;
}

void CTvAudio::AudioCtlInit()
{
    InitSetTvAudioCard();
}

void CTvAudio::AudioCtlUninit()
{
    int l_vol = 0, r_vol = 0;
    int audio_channel = -1;
    int oldMuteStatus;

    oldMuteStatus = GetAudioMuteForTv();
    SetAudioMuteForTv (CC_AUDIO_MUTE);

    AudioSetAudioInSource (CC_AUDIO_IN_SOURCE_LINEIN);
    AudioSetAudioSourceType (AUDIO_MPEG_SOURCE);

    AudioDataUnInit();

    SetAudioVolumeCompensationVal(0);

    AudioSetVolumeDigitLUTBuf(CC_LUT_SEL_MPEG, 1, 1, -1);
    SetAudioMasterVolume(GetAudioMasterVolume());

    SetAudioMuteForTv(oldMuteStatus);

    UnInitSetTvAudioCard();
}

int CTvAudio::AudioSetRecordDataZero(bool flag)
{
    return 0;
}

int CTvAudio::AudioHandleHeadsetPlugIn()
{
    return 0;
}

int CTvAudio::AudioHandleHeadsetPullOut()
{
    return 0;
}

int CTvAudio::amAudioSetDoubleOutputSwitch(int en_val)
{
    if (en_val == 1) {
        property_set("audio.output.enable", "1");
        property_set("audio.output.double_output", "1");
    }
    if (en_val == 0) {
        property_set("audio.output.enable", "0");
        property_set("audio.output.double_output", "0");
    }
    return 0;
}

int CTvAudio::setAudioPcmPlaybackVolume(int val)
{
    int pcm_volume = 0;
    pcm_volume = val / 2;
    if (pcm_volume > 24) pcm_volume = 24;
    //return SetAudioPcmPlaybackVolume(pcm_volume);
    return 0;
}

int CTvAudio::UsbAudioOuputModuleSwitch(int sw)
{
    if (sw == 1)
        property_set("persist.tv.audio_output_enable", "1");
    if (sw == 0)
        property_set("persist.tv.audio_output_enable", "0");
    return 0;
}

int CTvAudio::GetAmAudioSetDoubleOutputSwitch()
{
    char prop2[256];
    property_get("audio.output.double_output", prop2, "null");
    if (strcmp(prop2, "0") == 0 || strcmp(prop2, "null") == 0) {
        return 0;
    } else if (strcmp(prop2, "1") == 0) {
        return 1;
    }
    return 0;
}

int CTvAudio::GetCurUsbAudioOuputModuleSwitch()
{
    char prop1[256];
    property_get("audio.output.enable", prop1, "null");
    if (strcmp(prop1, "0") == 0 || strcmp(prop1, "null") == 0) {
        return 0;
    } else if (strcmp(prop1, "1") == 0) {
        return 1;
    }
    return 0;
}

int CTvAudio::amAudioSetTvDoubleOutput(int en_val, unsigned int sr, int input_device,
                                       int output_device)
{
    return amAudioSetDoubleOutput(en_val, sr, input_device, output_device);
}
int CTvAudio::HanldeAudioInputSr(unsigned int sr)
{
    int tmp_cfg = 0;

    tmp_cfg = GetAudioResampleTypeCFG();
    if (tmp_cfg & CC_AUD_RESAMPLE_TYPE_HW) {
        mAudioAlsa.SetHardwareResample(sr);
    } else {
        mAudioAlsa.SetHardwareResample(-1);
    }

    if (!(tmp_cfg & CC_AUD_RESAMPLE_TYPE_SW)) {
        sr = 48000;
    }

    return sr;
}

void CTvAudio::AudioSetNoiseGateThreshold(int thresh)
{
#if CC_TV_AUDIO_TYPE_ANDROID == 1
    amAndroidSetNoiseGateThreshold(thresh);
#endif

#ifdef CC_TV_AUDIO_TYPE_AMAUDIO == 1
    return;
#endif
}

int CTvAudio::AudioSSMRestoreDefaultSetting()
{
    int i = 0, tmp_val = 0;
    int nMinUIVol = 0, nMaxUIVol = 0;
    int *tmp_ptr = NULL;
    int tmpEQGainBuf[128] = { 0 };
    unsigned char tmp_buf[CC_NO_LINE_POINTS_MAX_CNT] = { 0 };

    // Save noline points
    tmp_val = GetAudioAmplifierMasterNoLinePointsCount();

    for (i = 0; i < CC_LUT_SEL_COUNT; i++) {
        tmp_ptr = GetAudioAmplifierMasterNoLinePointsDefDataBuffer(i);
        AudioArrayCopy(tmp_buf, tmp_ptr, CC_NO_LINE_POINTS_MAX_CNT);
        SSMSaveAudioNoLinePoints(i * tmp_val, tmp_val, tmp_buf);
    }

    // Save Current Audio Volume
    SaveCurAudioMasterVolume (CC_DEF_SOUND_VOL);

    // Save Current Audio Balance
    SaveCurAudioBalance (CC_DEF_SOUND_BALANCE_VAL);

    // Save Current Supper Bass Switch
    SaveCurAudioSupperBassSwitch (CC_SWITCH_OFF);

    // Save Current Supper Bass Volume
    SaveCurAudioSupperBassVolume (CC_DEF_SUPPERBASS_VOL);

    // Save Current SRSSurround
    SaveCurAudioSrsSurround(CC_SWITCH_OFF);

    // Save Current SRS DialogClarity
    SaveCurAudioSrsDialogClarity(CC_SWITCH_OFF);

    // Save Current SRS TruBass
    SaveCurAudioSrsTruBass(CC_SWITCH_OFF);

    // Save Current Audio Sound Mode
    SaveCurAudioSoundMode (CC_SOUND_MODE_STD);

    // Save Current Wall Effect
    SaveCurAudioWallEffect(CC_SWITCH_OFF);

    // Save Current spdif switch
    SaveCurAudioSPDIFSwitch (CC_SWITCH_ON);

    // Save Current spdif mode
    SaveCurAudioSPDIFMode (CC_SPDIF_MODE_PCM);

    // Save Current EQ mode
    SaveCurAudioEQMode (CC_EQ_MODE_NOMAL);

    // Save Current EQ Gain
    RestoreToAudioDefEQGain(tmpEQGainBuf);

    // Save Current Audio Bass and Treble
    nMinUIVol = GetBassUIMinGainVal();
    nMaxUIVol = GetBassUIMaxGainVal();
    RealSaveCurAudioBassVolume((nMinUIVol + nMaxUIVol) / 2, 0);

    nMinUIVol = GetTrebleUIMinGainVal();
    nMaxUIVol = GetTrebleUIMaxGainVal();
    RealSaveCurAudioTrebleVolume((nMinUIVol + nMaxUIVol) / 2, 0);

    return 0;
}


static const char *gDefCapturePropName = "snd.card.default.card.capture";

int CTvAudio::InitSetTvAudioCard()
{
    int i = 0, totle_num = 0, capture_ind = 0;
    char tmp_buf[32] = { 0 };
    char tv_card_name[64] = { 0 };
    char prop_value[PROPERTY_VALUE_MAX];

#ifdef BOARD_ALSA_AUDIO_TINY
#else // legacy alsa lib
    snd_card_refresh_info();
#endif

    if (GetTvAudioCardNeedSet()) {
        memset(prop_value, '\0', PROPERTY_VALUE_MAX);
        property_get("snd.card.totle.num", prop_value, "0");

        totle_num = strtoul(prop_value, NULL, 10);
        LOGD("%s, totle number = %d\n", __FUNCTION__, totle_num);
        totle_num = 8;

        GetTvAudioCardName(tv_card_name);

        for (i = 0; i < totle_num; i++) {
            sprintf(tmp_buf, "snd.card.%d.name", i);
            memset(prop_value, '\0', PROPERTY_VALUE_MAX);
            property_get(tmp_buf, prop_value, "null");

            LOGD("%s, key string \"%s\", value string \"%s\".\n", __FUNCTION__,
                 tmp_buf, prop_value);

            if (strcmp(prop_value, tv_card_name) == 0) {
                break;
            }
        }

        capture_ind = i;

        if (capture_ind == totle_num) {
            capture_ind = -1;
            LOGE("%s, can't find tv card \"%s\".\n", __FUNCTION__,
                 tv_card_name);
        }
    } else {
        capture_ind = -1;
    }

    sprintf(tmp_buf, "%d", capture_ind);
    property_set(gDefCapturePropName, tmp_buf);
    LOGD("%s, set \"%s\" as \"%s\".\n", __FUNCTION__, gDefCapturePropName,
         tmp_buf);

    return 0;
}

int CTvAudio::UnInitSetTvAudioCard()
{
    char tmp_buf[32] = { 0 };

#ifdef BOARD_ALSA_AUDIO_TINY
#else // legacy alsa lib
    snd_card_refresh_info();
#endif // BOARD_ALSA_AUDIO_TINY
    sprintf(tmp_buf, "%d", -1);
    property_set(gDefCapturePropName, tmp_buf);
    LOGD("%s, set \"%s\" as \"%s\".\n", __FUNCTION__, gDefCapturePropName,
         tmp_buf);

    return 0;
}

#define CS_SPDIF_MODE_DEV_PATH "/sys/class/audiodsp/digital_raw"

int CTvAudio::SetSPDIFMode(int mode_val)
{
    FILE *spdif_mode_fp = NULL;

    spdif_mode_fp = fopen(CS_SPDIF_MODE_DEV_PATH, "w");
    if (spdif_mode_fp == NULL) {
        LOGE("%s, Open file %s error(%s)!\n", __FUNCTION__, CS_SPDIF_MODE_DEV_PATH, strerror(errno));
        return -1;
    }

    if (mode_val == CC_SPDIF_MODE_PCM) {
        fprintf(spdif_mode_fp, "%d", 0);
    } else if (mode_val == CC_SPDIF_MODE_RAW ) {
        fprintf(spdif_mode_fp, "%d", 1);
    }

    fclose(spdif_mode_fp);
    spdif_mode_fp = NULL;

    return 0;
}
//0   is  ???            1  is  ????????
int CTvAudio::SwitchAVOutBypass(int sw)
{
    if (sw == 0 ) {
        mAudioAlsa.SetMixerBypassSwitch ( CC_SWITCH_OFF );
        mAudioAlsa.SetMixerDacSwitch ( CC_SWITCH_ON );
    } else {
        mAudioAlsa.SetMixerBypassSwitch ( CC_SWITCH_ON );
        mAudioAlsa.SetMixerDacSwitch ( CC_SWITCH_OFF );
    }
    return 0;
}

int CTvAudio::SetAudioSwitchIO(int value)
{
    return mAudioAlsa.SetAudioSwitchIO( value);
}
int CTvAudio::SetOutput_Swap(int value)
{
    return mAudioAlsa.SetOutput_Swap( value);
}
