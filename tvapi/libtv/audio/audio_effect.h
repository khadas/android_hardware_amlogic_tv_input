#ifndef __TV_AUDIO_EFFECT_H__
#define __TV_AUDIO_EFFECT_H__
#include "audio_android_effect.h"

class CAudioEffect
{
public:
    CAudioEffect();
    virtual ~CAudioEffect();
    int amAndroidNewAudioEffects();
    void amAndroidFreeAudioEffects();
    virtual int GetEQBandCount();
    virtual int GetEQMinGain();
    virtual int GetEQMaxGain();
    virtual int SetEQSwitch(int switch_val);
    virtual int GetEQSwitch();
    virtual int SetEQValue(int gain_val_buf[]);
    virtual int GetEQValue(int gain_val_buf[]);

    virtual int SetSrsSurroundSwitch(int switch_val);
    virtual int SetSrsSurroundGain(int gain_val);
    virtual int SetSrsTruBassSwitch(int switch_val);
    virtual int SetSrsTruBassGain(int gain_val);
    virtual int SetSrsDialogClaritySwitch(int switch_val);
    virtual int SetSrsDialogClarityGain(int gain_val);
    virtual int SetSrsDefinitionGain(int gain_val);
    virtual int SetSrsTrubassSpeakerSize(int tmp_val);

private:
    CAndroidEffect mAndroidEffect;
};
#endif //__TV_AUDIO_EFFECT_H__
