#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "audio_amaudio.h"
#include "audio_effect.h"

#define LOG_TAG "CAudioEffect"
#include "CTvLog.h"

#define CC_BAND_ITEM_CNT                ( 6 )
#define CC_MIN_EQ_GAIN_VAL              ( -10 )
#define CC_MAX_EQ_GAIN_VAL              ( 10 )
CAudioEffect::CAudioEffect()
{
}
CAudioEffect::~CAudioEffect()
{
}

int CAudioEffect::amAndroidNewAudioEffects()
{
    return mAndroidEffect.amAndroidNewAudioEffects();
}

void CAudioEffect::amAndroidFreeAudioEffects()
{
    return mAndroidEffect.amAndroidFreeAudioEffects();
}

int CAudioEffect::GetEQBandCount()
{
    return CC_BAND_ITEM_CNT;
}

int CAudioEffect::GetEQMinGain()
{
    return CC_MIN_EQ_GAIN_VAL;
}

int CAudioEffect::GetEQMaxGain()
{
    return CC_MAX_EQ_GAIN_VAL;
}

int CAudioEffect::SetEQSwitch(int switch_val)
{
    int tmp_ret = 0;

#if CC_TV_AUDIO_TYPE_ANDROID == 1
    return mAndroidEffect.amAndroidSetEQEnable(switch_val);
#endif

#ifdef CC_TV_AUDIO_TYPE_AMAUDIO == 1
    tmp_ret |= amAudioSetEQEnable(switch_val);
    tmp_ret |= mAndroidEffect.amAndroidSetEQEnable(switch_val);
    return tmp_ret;
#endif
}

int CAudioEffect::GetEQSwitch()
{
    int tmp_ret = 0;

#if CC_TV_AUDIO_TYPE_ANDROID == 1
    return mAndroidEffect.amAndroidGetEQEnabled();
#endif

#if CC_TV_AUDIO_TYPE_AMAUDIO == 1
    return mAndroidEffect.amAndroidGetEQEnabled();
#endif
}

int CAudioEffect::SetEQValue(int gain_val_buf[])
{
    int tmp_ret = 0;

#if CC_TV_AUDIO_TYPE_ANDROID == 1
    return mAndroidEffect.amAndroidSetEQGain(gain_val_buf, CC_BAND_ITEM_CNT);
#endif

#if CC_TV_AUDIO_TYPE_AMAUDIO == 1
    tmp_ret |= amAudioSetEQGain(gain_val_buf, CC_BAND_ITEM_CNT);
    tmp_ret |= mAndroidEffect.amAndroidSetEQGain(gain_val_buf, CC_BAND_ITEM_CNT);
    return tmp_ret;
#endif
}

int CAudioEffect::GetEQValue(int gain_val_buf[])
{
    int tmp_ret = 0;

#if CC_TV_AUDIO_TYPE_ANDROID == 1
    return mAndroidEffect.amAndroidGetEQGain(gain_val_buf, CC_BAND_ITEM_CNT);
#endif

#if CC_TV_AUDIO_TYPE_AMAUDIO == 1
    return mAndroidEffect.amAndroidGetEQGain(gain_val_buf, CC_BAND_ITEM_CNT);
#endif
}

int CAudioEffect::SetSrsSurroundSwitch(int switch_val)
{
    int tmp_ret = 0;

#if CC_TV_AUDIO_TYPE_ANDROID == 1
    return mAndroidEffect.amAndroidSetSRSSurroundSwitch(switch_val);
#endif

#if CC_TV_AUDIO_TYPE_AMAUDIO == 1
    tmp_ret |= amAudioSetSRSSurroundSwitch(switch_val);
    tmp_ret |= mAndroidEffect.amAndroidSetSRSSurroundSwitch(switch_val);
    return tmp_ret;
#endif
}

int CAudioEffect::SetSrsSurroundGain(int gain_val)
{
    int tmp_ret = 0;

#if CC_TV_AUDIO_TYPE_ANDROID == 1
    return mAndroidEffect.amAndroidSetSRSSurroundGain(gain_val);
#endif

#if CC_TV_AUDIO_TYPE_AMAUDIO == 1
    tmp_ret |= amAudioSetSRSSurroundGain(gain_val);
    tmp_ret |= mAndroidEffect.amAndroidSetSRSSurroundGain(gain_val);
    return tmp_ret;
#endif
}

int CAudioEffect::SetSrsTruBassSwitch(int switch_val)
{
    int tmp_ret = 0;

#if CC_TV_AUDIO_TYPE_ANDROID == 1
    return mAndroidEffect.amAndroidSetSRSTrubassSwitch(switch_val);
#endif

#if CC_TV_AUDIO_TYPE_AMAUDIO == 1
    tmp_ret |= amAudioSetSRSTrubassSwitch(switch_val);
    tmp_ret |= mAndroidEffect.amAndroidSetSRSTrubassSwitch(switch_val);
    return tmp_ret;
#endif
}

int CAudioEffect::SetSrsTruBassGain(int gain_val)
{
    int tmp_ret = 0;

#if CC_TV_AUDIO_TYPE_ANDROID == 1
    return amAndroidSetSRSTrubassGain(gain_val);
#endif

#if CC_TV_AUDIO_TYPE_AMAUDIO == 1
    tmp_ret |= amAudioSetSRSTrubassGain(gain_val);
    tmp_ret |= mAndroidEffect.amAndroidSetSRSTrubassGain(gain_val);
    return tmp_ret;
#endif
}

int CAudioEffect::SetSrsDialogClaritySwitch(int switch_val)
{
    int tmp_ret = 0;

#if CC_TV_AUDIO_TYPE_ANDROID == 1
    return mAndroidEffect.amAndroidSetSRSDialogClaritySwitch(switch_val);
#endif

#if CC_TV_AUDIO_TYPE_AMAUDIO == 1
    tmp_ret |= amAudioSetSRSDialogClaritySwitch(switch_val);
    tmp_ret |= mAndroidEffect.amAndroidSetSRSDialogClaritySwitch(switch_val);
    return tmp_ret;
#endif
}

int CAudioEffect::SetSrsDialogClarityGain(int gain_val)
{
    int tmp_ret = 0;

#if CC_TV_AUDIO_TYPE_ANDROID == 1
    return mAndroidEffect.amAndroidSetSRSDialogClarityGain(gain_val);
#endif

#if CC_TV_AUDIO_TYPE_AMAUDIO == 1
    tmp_ret |= amAudioSetSRSDialogClarityGain(gain_val);
    tmp_ret |= mAndroidEffect.amAndroidSetSRSDialogClarityGain(gain_val);
    return tmp_ret;
#endif
}

int CAudioEffect::SetSrsDefinitionGain(int gain_val)
{
    int tmp_ret = 0;

#if CC_TV_AUDIO_TYPE_ANDROID == 1
    return mAndroidEffect.amAndroidSetSRSDefinitionGain(gain_val);
#endif

#if CC_TV_AUDIO_TYPE_AMAUDIO == 1
    tmp_ret |= amAudioSetSRSDefinitionGain(gain_val);
    tmp_ret |= mAndroidEffect.amAndroidSetSRSDefinitionGain(gain_val);
    return tmp_ret;
#endif
}

int CAudioEffect::SetSrsTrubassSpeakerSize(int set_val)
{
    int tmp_ret = 0;

#if CC_TV_AUDIO_TYPE_ANDROID == 1
    return mAndroidEffect.amAndroidSetSRSTrubassSpeakerSize(set_val);
#endif

#if CC_TV_AUDIO_TYPE_AMAUDIO == 1
    tmp_ret |= amAudioSetSRSTrubassSpeakerSize(set_val);
    tmp_ret |= mAndroidEffect.amAndroidSetSRSTrubassSpeakerSize(set_val);
    return tmp_ret;
#endif
}
