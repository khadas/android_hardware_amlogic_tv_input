#ifndef __TV_AUDIO_ANDROID_EFFECT_H__
#define __TV_AUDIO_ANDROID_EFFECT_H__



#include <media/AudioEffect.h>
#include <hardware/audio_effect.h>
#include <hardware/audio_effect_srs.h>
#include <hardware/audio_effect_hpeq.h>
#include <audio_effects/effect_equalizer.h>
#include "tvsetting/audio_cfg.h"

#define EQ_BAND_COUNT    (5)
using namespace android;

class CAndroidEffect
{
public:
    CAndroidEffect();
    ~CAndroidEffect();
    int amAndroidNewAudioEffects(void);
    void amAndroidFreeAudioEffects(void);
    int amAndroidGetAudioSessionId(void);

    //EQ
    int amAndroidSetEQGain(const int gain_val_buf[], int band_count);
    int amAndroidGetEQGain(int gain_val_buf[], int band_count);
    int amAndroidSetEQEnable(int enabled);
    int amAndroidGetEQEnabled();

    //SRS
    int amAndroidSetSRSSurroundSwitch(int switch_val);
    int amAndroidSetSRSSurroundGain(int gain_val);
    int amAndroidSetSRSTrubassSwitch(int switch_val);
    int amAndroidSetSRSTrubassGain(int gain_val);
    int amAndroidSetSRSDialogClaritySwitch(int switch_val);
    int amAndroidSetSRSDialogClarityGain(int gain_val);
    int amAndroidSetSRSDefinitionGain(int gain_val);
    int amAndroidSetSRSTrubassSpeakerSize(int tmp_val);


private:
    int gEqForcedGain[EQ_BAND_COUNT];
    int gEqUserSetGain[EQ_BAND_COUNT];
    void FreeAudioEffect(AudioEffect **pEffect);
    void initAudioEffect(AudioEffect **pEffect, const effect_uuid_t *const pEffectType, const effect_uuid_t *const pEffectUuid, int sessionId);
    int setSrsParameterInt(tshd_srs_param_t param, unsigned value);
    int getSrsParameterInt(tshd_srs_param_t param, unsigned *value);
    int setSrsParameterFloat(tshd_srs_param_t param, float value);
    int getSrsParameterFloat(tshd_srs_param_t param, float *value);
    int amAndroidSetSrsParameterInt(tshd_srs_param_t param, unsigned value);
    int amAndroidGetSrsParameterInt(tshd_srs_param_t param, unsigned *value);
    int amAndroidSetSrsParameterFloat(tshd_srs_param_t param, float value);
    int amAndroidGetSrsParameterFloat(tshd_srs_param_t param, float *value);
    int getEqGain(int gain_val_buf[], int band_count);
    int setEqGain(const int gain_val_buf[], int band_count);
    int amAndroidNewSRSAudioEffect(void);
    int amAndroidNewEQAudioEffect(void);
    int amAndroidForceEQGain(int band, int gain);

    AudioEffect *mplpAudioEffectSrs;
    AudioEffect *mplpAudioEffectEq;
    int mTrackerSessionId;
};

#endif //__TV_AUDIO_ANDROID_EFFECT_H__
