#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <android/log.h>
#include <sys/prctl.h>

#include "audio_android_effect.h"

#define msleep(x) usleep(x*1000)

#define INVALID_EQ_FORCED_GAIN          (0x7FFFFFFF)

#define LOG_TAG "CAndroidEffect"
#include "CTvLog.h"

using namespace android;

// For some special cases, we want some band of the EQ to be forced to a specific
// gain. When the value in this array is not invalid, the gain of the corresponding
// band will be forced to this value.
CAndroidEffect::CAndroidEffect()
{
    mplpAudioEffectSrs = NULL;
    mplpAudioEffectEq = NULL;
    mTrackerSessionId = 0;
}

CAndroidEffect::~CAndroidEffect()
{
}

static void audioEffectCallback(int32_t event, void *user, void *info)
{
    LOGD("[%s:%d]", __FUNCTION__, __LINE__);
}

void CAndroidEffect::FreeAudioEffect(AudioEffect **pEffect)
{
    if ((*pEffect) != NULL) {
        (*pEffect)->setEnabled(false);
        delete (*pEffect);
        (*pEffect) = NULL;
    }
}

void CAndroidEffect::initAudioEffect(AudioEffect **pEffect,
                                     const effect_uuid_t *const pEffectType,
                                     const effect_uuid_t *const pEffectUuid, int sessionId)
{
    if (NULL != (*pEffect)) {
        LOGE("[%s:%d] AudioEffect is not NULL. Free it first.\n", __FUNCTION__,
             __LINE__);
        FreeAudioEffect(pEffect);
    }

    status_t status;

    (*pEffect) = new AudioEffect();
    if (NULL == (*pEffect)) {
        LOGE("[%s:%d] Failed to create audio effect.\n", __FUNCTION__,
             __LINE__);
        goto err_exit;
    }

    status = (*pEffect)->set(pEffectType, //type
                             pEffectUuid, //uuid
                             0, //priority
                             audioEffectCallback, //cbf
                             NULL, //user
                             sessionId, //sessionId
                             0 // default output device
                            );

    if (NO_ERROR != status) {
        LOGE("[%s:%d] Failed to set audio effect parameters.\n", __FUNCTION__,
             __LINE__);
        goto err_exit;
    }

    status = (*pEffect)->initCheck();
    if (NO_ERROR != status) {
        LOGE("[%s:%d] Failed to init audio effect.\n", __FUNCTION__, __LINE__);
        goto err_exit;
    }

    status = (*pEffect)->setEnabled(true);
    if (NO_ERROR != status) {
        LOGE("[%s:%d] Failed to enable audio effect.\n", __FUNCTION__,
             __LINE__);
        goto err_exit;
    }

    return;

err_exit:
    if (NULL != (*pEffect)) {
        delete (*pEffect);
        (*pEffect) = NULL;
    }
}

int CAndroidEffect::setSrsParameterInt(tshd_srs_param_t param, unsigned value)
{
    if (NULL == mplpAudioEffectSrs) {
        return -1;
    }

    status_t status;
    effect_param_t *pParam = (effect_param_t *) malloc(
                                 sizeof(effect_param_t) + sizeof(unsigned) + sizeof(unsigned));
    if (NULL == pParam) {
        LOGE("[%s:%d]Failed to allocate memory!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    pParam->status = 0;
    pParam->psize = sizeof(unsigned);
    pParam->vsize = sizeof(unsigned);
    *(unsigned *) (&pParam->data[0]) = (unsigned) param;
    *(unsigned *) (&pParam->data[sizeof(unsigned)]) = value;
    status = mplpAudioEffectSrs->setParameter(pParam);
    if (NO_ERROR != status) {
        LOGE("[%s:%d]Failed to set parameter:%d!\n", __FUNCTION__, __LINE__,
             param);
        free(pParam);
        return -1;
    } else {
        free(pParam);
        return 0;
    }
}

int CAndroidEffect::getSrsParameterInt(tshd_srs_param_t param, unsigned *value)
{
    if (NULL == mplpAudioEffectSrs) {
        return -1;
    }

    status_t status;
    effect_param_t *pParam = (effect_param_t *) malloc(
                                 sizeof(effect_param_t) + sizeof(unsigned) + sizeof(unsigned));
    if (NULL == pParam) {
        LOGE("[%s:%d]Failed to allocate memory!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    pParam->status = 0;
    pParam->psize = sizeof(unsigned);
    pParam->vsize = sizeof(unsigned);
    *(unsigned *) (&pParam->data[0]) = (unsigned) param;
    status = mplpAudioEffectSrs->getParameter(pParam);
    if (NO_ERROR != status) {
        LOGE("[%s:%d]Failed to get parameter:%d!\n", __FUNCTION__, __LINE__,
             param);
        *value = 0;
        free(pParam);
        return -1;
    } else {
        *value = *(unsigned *) (&pParam->data[sizeof(unsigned)]);
        free(pParam);
        return 0;
    }
}

int CAndroidEffect::setSrsParameterFloat(tshd_srs_param_t param, float value)
{
    if (NULL == mplpAudioEffectSrs) {
        return -1;
    }

    status_t status;
    effect_param_t *pParam = (effect_param_t *) malloc(
                                 sizeof(effect_param_t) + sizeof(unsigned) + sizeof(float));
    if (NULL == pParam) {
        LOGE("[%s:%d]Failed to allocate memory!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    pParam->status = 0;
    pParam->psize = sizeof(unsigned);
    pParam->vsize = sizeof(float);
    *(unsigned *) (&pParam->data[0]) = (unsigned) param;
    *(float *) (&pParam->data[sizeof(unsigned)]) = value;
    status = mplpAudioEffectSrs->setParameter(pParam);
    if (NO_ERROR != status) {
        LOGE("[%s:%d]Failed to set parameter:%d!\n", __FUNCTION__, __LINE__,
             param);
        free(pParam);
        return -1;
    } else {
        free(pParam);
        return 0;
    }
}

int CAndroidEffect::getSrsParameterFloat(tshd_srs_param_t param, float *value)
{
    if (NULL == mplpAudioEffectSrs) {
        return -1;
    }

    status_t status;
    effect_param_t *pParam = (effect_param_t *) malloc(
                                 sizeof(effect_param_t) + sizeof(unsigned) + sizeof(float));
    if (NULL == pParam) {
        LOGE("[%s:%d]Failed to allocate memory!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    pParam->status = 0;
    pParam->psize = sizeof(unsigned);
    pParam->vsize = sizeof(float);
    *(unsigned *) (&pParam->data[0]) = (unsigned) param;
    status = mplpAudioEffectSrs->getParameter(pParam);
    if (NO_ERROR != status) {
        LOGE("[%s:%d]Failed to set parameter:%d!\n", __FUNCTION__, __LINE__,
             param);
        *value = 0.0f;
        free(pParam);
        return -1;
    } else {
        *value = *(float *) (&pParam->data[sizeof(unsigned)]);
        free(pParam);
        return 0;
    }
}

int CAndroidEffect::amAndroidSetSrsParameterInt(tshd_srs_param_t param, unsigned value)
{
    return setSrsParameterInt(param, value);
}

int CAndroidEffect::amAndroidGetSrsParameterInt(tshd_srs_param_t param,
        unsigned *value)
{
    return getSrsParameterInt(param, value);
}

int CAndroidEffect::amAndroidSetSrsParameterFloat(tshd_srs_param_t param, float value)
{
    return setSrsParameterFloat(param, value);
}

int CAndroidEffect::amAndroidGetSrsParameterFloat(tshd_srs_param_t param, float *value)
{
    return getSrsParameterFloat(param, value);
}

int CAndroidEffect::amAndroidNewSRSAudioEffect(void)
{
    if ((NULL == mplpAudioEffectSrs) && (GetAudioSRSModuleDisableCFG() == 0)) {
        initAudioEffect(&mplpAudioEffectSrs, NULL, SL_IID_SRS,
                        mTrackerSessionId);
        if (NULL == mplpAudioEffectSrs) {
            LOGE("[%s:%d] Failed to create audio effect SRS!\n", __FUNCTION__,
                 __LINE__);
            return -1;
        }
    }

    return 0;
}

int CAndroidEffect::amAndroidNewEQAudioEffect(void)
{
    if ((NULL == mplpAudioEffectEq) && (GetAudioEqModuleDisableCFG() == 0)) {
        if (GetAudioEqUsingAndroidCFG()) {
            initAudioEffect(&mplpAudioEffectEq, SL_IID_EQUALIZER, NULL,
                            mTrackerSessionId);
        } else {
            initAudioEffect(&mplpAudioEffectEq, NULL, SL_IID_HPEQ,
                            mTrackerSessionId);
        }

        if (NULL == mplpAudioEffectEq) {
            LOGE("[%s:%d] Failed to create audio effect EQ(android:%d)!\n",
                 __FUNCTION__, __LINE__, GetAudioEqUsingAndroidCFG());
            return -1;
        } else {
            // Init all forced gains to be invalid.
            for (int i = 0; i < EQ_BAND_COUNT; i++) {
                gEqForcedGain[i] = INVALID_EQ_FORCED_GAIN;
            }
            getEqGain(gEqUserSetGain, EQ_BAND_COUNT);
            LOGD(
                "[%s:%d] Audio effect EQ(android:%d) was created successfully!\n",
                __FUNCTION__, __LINE__, GetAudioEqUsingAndroidCFG());
        }
    }

    return 0;
}

int CAndroidEffect::amAndroidNewAudioEffects(void)
{
    int tmp_ret = 0;

    tmp_ret |= amAndroidNewSRSAudioEffect();
    tmp_ret |= amAndroidNewEQAudioEffect();

    return tmp_ret;
}

void CAndroidEffect::amAndroidFreeAudioEffects(void)
{
    {
        FreeAudioEffect(&mplpAudioEffectEq);
        FreeAudioEffect(&mplpAudioEffectSrs);

        if (mTrackerSessionId != 0) {
            AudioSystem::releaseAudioSessionId(mTrackerSessionId, -1);
            mTrackerSessionId = 0;
        }
    }
}

int CAndroidEffect::amAndroidGetAudioSessionId(void)
{
    return mTrackerSessionId;
}

int CAndroidEffect::amAndroidSetSRSSurroundSwitch(int switch_val)
{
    return amAndroidSetSrsParameterInt(SRS_PARAM_SURROUND_ENABLE,
                                       (unsigned) switch_val);
}

int CAndroidEffect::amAndroidSetSRSSurroundGain(int gain_val)
{
    return amAndroidSetSrsParameterFloat(SRS_PARAM_SURROUND_GAIN,
                                         (float) gain_val / 100.0f);
}

int CAndroidEffect::amAndroidSetSRSTrubassSwitch(int switch_val)
{
    return amAndroidSetSrsParameterInt(SRS_PARAM_TRUEBASS_ENABLE,
                                       (unsigned) switch_val);
}

int CAndroidEffect::amAndroidSetSRSTrubassGain(int gain_val)
{
    return amAndroidSetSrsParameterFloat(SRS_PARAM_TRUEBASS_GAIN,
                                         (float) gain_val / 100.0f);
}

int CAndroidEffect::amAndroidSetSRSDialogClaritySwitch(int switch_val)
{
    return amAndroidSetSrsParameterInt(SRS_PARAM_DIALOGCLARITY_ENABLE,
                                       (unsigned) switch_val);
}

int CAndroidEffect::amAndroidSetSRSDialogClarityGain(int gain_val)
{
    return amAndroidSetSrsParameterFloat(SRS_PARAM_DIALOGCLARTY_GAIN,
                                         (float) gain_val / 100.0f);
}

int CAndroidEffect::amAndroidSetSRSDefinitionGain(int gain_val)
{
    return amAndroidSetSrsParameterFloat(SRS_PARAM_DEFINITION_GAIN,
                                         (float) gain_val / 100.0f);
}

int CAndroidEffect::amAndroidSetSRSTrubassSpeakerSize(int tmp_val)
{
    int tmp_buf[8] = { 40, 60, 100, 150, 200, 250, 300, 400 };
    int set_val = 40;

    if (tmp_val >= 0 && tmp_val < sizeof(tmp_buf)) {
        set_val = tmp_buf[tmp_val];
    }

    return amAndroidSetSrsParameterInt(SRS_PARAM_TRUEBASS_SPKER_SIZE, set_val);
}

int CAndroidEffect::setEqGain(const int gain_val_buf[], int band_count)
{
    if (NULL == mplpAudioEffectEq) {
        LOGE("[%s:%d]\n", __FUNCTION__, __LINE__);
        return -1;
    }

    status_t status;
    int i;
    int gain;
    effect_param_t *pParam;
    int bAndroidEq = GetAudioEqUsingAndroidCFG();

    if (bAndroidEq) {
        pParam = (effect_param_t *) malloc(
                     sizeof(effect_param_t) + sizeof(int) + sizeof(int)
                     + sizeof(short));
    } else {
        pParam = (effect_param_t *) malloc(
                     sizeof(effect_param_t) + sizeof(int) + sizeof(int));
    }

    if (NULL == pParam) {
        LOGE("[%s:%d]Failed to allocate memory!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    for (i = 0; i < band_count; i++) {
        if (gEqForcedGain[i] != INVALID_EQ_FORCED_GAIN) {
            gain = gEqForcedGain[i];
        } else {
            gain = gain_val_buf[i];
        }

        if (bAndroidEq) {
            pParam->status = 0;
            pParam->psize = sizeof(int) * 2;
            pParam->vsize = sizeof(short);
            *(int *) (&pParam->data[0]) = EQ_PARAM_BAND_LEVEL;
            *(int *) (&pParam->data[sizeof(int)]) = i;
            *(short *) (&pParam->data[sizeof(int) * 2]) = (short) gain;
        } else {
            pParam->status = 0;
            pParam->psize = sizeof(int);
            pParam->vsize = sizeof(int);
            *(int *) (&pParam->data[0]) = i;
            *(int *) (&pParam->data[sizeof(int)]) = gain;
        }

        status = mplpAudioEffectEq->setParameter(pParam);
        if (NO_ERROR != status) {
            LOGE("[%s:%d]Failed to set parameter:%d!\n", __FUNCTION__, __LINE__,
                 i);
            free(pParam);
            return -1;
        }
    }

    free(pParam);
    return 0;
}

int CAndroidEffect::getEqGain(int gain_val_buf[], int band_count)
{
    if (NULL == mplpAudioEffectEq) {
        return -1;
    }

    status_t status;
    effect_param_t *pParam;
    int bAndroidEq = GetAudioEqUsingAndroidCFG();

    if (bAndroidEq) {
        pParam = (effect_param_t *) malloc(
                     sizeof(effect_param_t) + sizeof(int) + sizeof(int)
                     + sizeof(short));
    } else {
        pParam = (effect_param_t *) malloc(
                     sizeof(effect_param_t) + sizeof(int) + sizeof(int));
    }

    if (NULL == pParam) {
        LOGE("[%s:%d]Failed to allocate memory!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    for (int i = 0; i < EQ_BAND_COUNT; i++) {
        if (gEqForcedGain[i] != INVALID_EQ_FORCED_GAIN) {
            gain_val_buf[i] = gEqUserSetGain[i];
        } else {
            if (bAndroidEq) {
                pParam->status = 0;
                pParam->psize = sizeof(int) * 2;
                pParam->vsize = sizeof(short);
                *(int *) (&pParam->data[0]) = EQ_PARAM_BAND_LEVEL;
                *(int *) (&pParam->data[sizeof(int)]) = i;
            } else {
                pParam->status = 0;
                pParam->psize = sizeof(int);
                pParam->vsize = sizeof(int);
                *(int *) (&pParam->data[0]) = i;
            }
            status = mplpAudioEffectEq->getParameter(pParam);
            if (NO_ERROR != status) {
                LOGE("[%s:%d]Failed to set parameter:%d!\n", __FUNCTION__,
                     __LINE__, i);
                free(pParam);
                return -1;
            }

            if (bAndroidEq) {
                gain_val_buf[i] = *(short *) (&pParam->data[sizeof(int) * 2]);
            } else {
                gain_val_buf[i] = *(int *) (&pParam->data[sizeof(int)]);
            }
        }
    }

    free(pParam);
    return 0;
}

int CAndroidEffect::amAndroidSetEQGain(const int gain_val_buf[], int band_count)
{
    int i;
    int gain_factor = 1;

    if (band_count > EQ_BAND_COUNT) {
        band_count = EQ_BAND_COUNT;
    }

    // convert dB to 100th dB for android EQ
    if (GetAudioEqUsingAndroidCFG()) {
        gain_factor = 100;
    }

    // store the gains set by the user, and convert it to the internal representation.
    for (i = 0; i < band_count; i++) {
        gEqUserSetGain[i] = gain_val_buf[i] * gain_factor;
    }

    return setEqGain(gEqUserSetGain, band_count);
}

int CAndroidEffect::amAndroidGetEQGain(int gain_val_buf[], int band_count)
{
    int i;
    int ret;

    if (band_count > EQ_BAND_COUNT) {
        band_count = EQ_BAND_COUNT;
    }

    ret = getEqGain(gain_val_buf, band_count);

    // convert 100th dB to dB for android EQ
    if (GetAudioEqUsingAndroidCFG()) {
        for (i = 0; i < band_count; i++) {
            gain_val_buf[i] /= 100;
        }
    }

    return ret;
}

int CAndroidEffect::amAndroidSetEQEnable(int enabled)
{
    if (NULL == mplpAudioEffectEq) {
        return -1;
    }

    if (NO_ERROR != mplpAudioEffectEq->setEnabled(enabled)) {
        return -1;
    } else {
        return 0;
    }
}

int CAndroidEffect::amAndroidGetEQEnabled()
{
    if (NULL == mplpAudioEffectEq) {
        return 0;
    }

    if (mplpAudioEffectEq->getEnabled()) {
        return 1;
    } else {
        return 0;
    }
}

int CAndroidEffect::amAndroidForceEQGain(int band, int gain)
{
    if (band >= EQ_BAND_COUNT) {
        LOGE("[%s:%d] band(%d) is out of bound!\n", __FUNCTION__, __LINE__,
             band);
        return -1;
    } else {
        if (GetAudioEqUsingAndroidCFG() && (gain != INVALID_EQ_FORCED_GAIN)) {
            gEqForcedGain[band] = gain * 100;
        } else {
            gEqForcedGain[band] = gain;
        }

        // reset the EQ bands to take effect of the forced one.
        if (setEqGain(gEqUserSetGain, EQ_BAND_COUNT) != 0) {
            LOGE("[%s:%d]Failed to reset EQ gain!\n", __FUNCTION__, __LINE__);
            return -1;
        }

        LOGD("[%s:%d] band(%d) is forced to %d!\n", __FUNCTION__, __LINE__,
             band, gain);
        return 0;
    }
}
