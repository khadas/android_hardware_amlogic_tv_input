#include "CTvin.h"
#include <CTvLog.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <errno.h>

#include <cutils/log.h>

#include "../tvutils/tvutils.h"
#include "../tvconfig/tvconfig.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "CSourceConnectDetect"
#endif

#define CC_DETECT_SOURCE_AV2                 (0)

#define CC_DEBUG_AV_SAMPLE_BUF               (1)

#define CC_AV_PLUG_OUT_ADC_DEF_THRESHOLD     (28) // voltage / 1800 * 1023
#define CC_AV_PLUG_IN_ADC_DEF_THRESHOLD      (70) // voltage / 1800 * 1023

CTvin::CSourceConnectDetect::CSourceConnectDetect(CTvin *pTvin)
{
    for (int i = 0; i < SOURCE_MAX; i++) {
        mSourceDetectTable[i] = SOURCE_INVALID;
        mSourceDetectPreStatusBuf[i] = CC_SOURCE_PLUG_OUT;
        mSourceDetectCurStatusBuf[i] = CC_SOURCE_PLUG_OUT;
    }

    for (int i = 0; i < CC_AV_DETECT_SAMPLE_DATA_MAX_LEN; i++) {
        mAV1SampleDataBuf[i] = 0;
        mAV2SampleDataBuf[i] = 0;
    }

#if CC_DETECT_SOURCE_AV2 == 1
    mSourceDetectTable[0] = SOURCE_AV1;
    mSourceDetectTable[1] = SOURCE_AV2;
    mSourceDetectTable[2] = SOURCE_HDMI1;
    mSourceDetectTable[3] = SOURCE_HDMI2;
    mSourceDetectTable[4] = SOURCE_HDMI3;
#else
    mSourceDetectTable[0] = SOURCE_AV1;
    mSourceDetectTable[1] = SOURCE_HDMI1;
    mSourceDetectTable[2] = SOURCE_HDMI2;
    mSourceDetectTable[3] = SOURCE_HDMI3;
#endif

    mSourceDetectSleepTime = 200; //default 200ms

    mAVDetectMethod = CC_AV_DETECT_METHOD_ADC_PLUG_OUT;
    mAVDetectPlugOutADCThreshold = CC_AV_PLUG_OUT_ADC_DEF_THRESHOLD;
    mAVDetectPlugInADCThreshold = CC_AV_PLUG_IN_ADC_DEF_THRESHOLD;
    mAVDetectPlugInDutyCycle = 70;
    mAVDetectPlugOutDutyCycle = 70;
    mAVDetectSampleSize = 15;
    mAV1DetectADCChan = 2;
    mAV2DetectADCChan = 3;

    mpObserver = NULL;
    mpTvin = pTvin;
}

CTvin::CSourceConnectDetect::~CSourceConnectDetect()
{
}

int CTvin::CSourceConnectDetect::startDetect()
{
    refreshDetectSources();
    refreshDetectAVInfo();
    this->run();

    return 0;
}

int CTvin::CSourceConnectDetect::refreshDetectSources()
{
    int cur_index = 0;
    char *token = NULL;
    const char *strDelimit = ",";
    const char *config_value = NULL;
    char data_str[CC_CFG_VALUE_STR_MAX_LEN] = { 0 };

    config_value = config_get_str("TV", "tvin.SourceConnectDetect.sources", "null");
    if (strcasecmp(config_value, "null") == 0) {
        return 0;
    }

    for (int i = 0; i < SOURCE_MAX; i++) {
        mSourceDetectTable[i] = SOURCE_INVALID;
        mSourceDetectPreStatusBuf[i] = CC_SOURCE_PLUG_OUT;
        mSourceDetectCurStatusBuf[i] = CC_SOURCE_PLUG_OUT;
    }

    cur_index = 0;
    memset((void *)data_str, 0, sizeof(data_str));
    strncpy(data_str, config_value, sizeof(data_str) - 1);
    token = strtok(data_str, strDelimit);
    while (token != NULL) {
        if (strcasecmp(token, "SOURCE_AV1") == 0) {
            mSourceDetectTable[cur_index] = SOURCE_AV1;
            cur_index += 1;
            //LOGD("%s, add detect source SOURCE_AV1.\n", __FUNCTION__);
        } else if (strcasecmp(token, "SOURCE_AV2") == 0) {
            mSourceDetectTable[cur_index] = SOURCE_AV2;
            cur_index += 1;
            //LOGD("%s, add detect source SOURCE_AV2.\n", __FUNCTION__);
        } else if (strcasecmp(token, "SOURCE_HDMI1") == 0) {
            mSourceDetectTable[cur_index] = SOURCE_HDMI1;
            cur_index += 1;
            //LOGD("%s, add detect source SOURCE_HDMI1.\n", __FUNCTION__);
        } else if (strcasecmp(token, "SOURCE_HDMI2") == 0) {
            mSourceDetectTable[cur_index] = SOURCE_HDMI2;
            cur_index += 1;
            //LOGD("%s, add detect source SOURCE_HDMI2.\n", __FUNCTION__);
        } else if (strcasecmp(token, "SOURCE_HDMI3") == 0) {
            mSourceDetectTable[cur_index] = SOURCE_HDMI3;
            cur_index += 1;
            //LOGD("%s, add detect source SOURCE_HDMI3.\n", __FUNCTION__);
        }

        token = strtok(NULL, strDelimit);
    }

    return 0;
}

int CTvin::CSourceConnectDetect::refreshDetectAVInfo()
{
    int cur_index = 0;
    char *token = NULL;
    const char *strDelimit = ",";
    const char *config_value = NULL;
    char data_str[CC_CFG_VALUE_STR_MAX_LEN] = { 0 };

    config_value = config_get_str("TV", "tvin.SourceConnectDetect.av.info", "null");
    if (strcasecmp(config_value, "null") == 0) {
        return 0;
    }

    cur_index = 0;
    memset((void *)data_str, 0, sizeof(data_str));
    strncpy(data_str, config_value, sizeof(data_str) - 1);
    token = strtok(data_str, strDelimit);
    while (token != NULL) {
        if (cur_index == 0) {
            mAV1DetectADCChan = strtol(token, NULL, 10);
        } else if (cur_index == 1) {
            mAV2DetectADCChan = strtol(token, NULL, 10);
        } else if (cur_index == 2) {
            mAVDetectSampleSize = strtol(token, NULL, 10);
        } else if (cur_index == 3) {
            mAVDetectMethod = strtol(token, NULL, 10);
        } else if (cur_index == 4) {
            mAVDetectPlugInDutyCycle = strtol(token, NULL, 10);
        } else if (cur_index == 5) {
            mAVDetectPlugOutDutyCycle = strtol(token, NULL, 10);
        } else if (cur_index == 6) {
            mAVDetectPlugInADCThreshold = strtol(token, NULL, 10);
        } else if (cur_index == 7) {
            mAVDetectPlugOutADCThreshold = strtol(token, NULL, 10);
        }

        cur_index += 1;
        token = strtok(NULL, strDelimit);
    }

    return 0;
}

int CTvin::CSourceConnectDetect::GetSourceConnectStatus(int source_input)
{
    for (int i = 0; i < SOURCE_MAX; i++) {
        if (mSourceDetectTable[i] == source_input) {
            return mSourceDetectCurStatusBuf[i];
        }
    }

    return CC_SOURCE_PLUG_OUT;
}

bool CTvin::CSourceConnectDetect::threadLoop()
{
    if ( mpObserver == NULL ) {
        return false;
    }

    LOGD("%s, entering...\n", "TV");

    prctl(PR_SET_NAME, (unsigned long)"CSourceConnectDetect thread loop");

    mpTvin->VDIN_OpenHDMIPinMuxOn(true);

    while (!exitPending()) { //requietexit() or requietexitWait() not call
        DetectSources();

        usleep(mSourceDetectSleepTime * 1000);
    }

    LOGD("%s, exiting...\n", "TV");
    //return true, run again, return false,not run.
    return false;
}

int CTvin::CSourceConnectDetect::DetectSources()
{
    int i = 0, cur_source = 0;

    for (i = 0; i < SOURCE_MAX; i++) {
        cur_source = mSourceDetectTable[i];
        if (cur_source == SOURCE_AV1 || cur_source == SOURCE_AV2) {
            mSourceDetectCurStatusBuf[i] = DetectAVSource(cur_source);
        } else if (cur_source == SOURCE_HDMI1 || cur_source == SOURCE_HDMI2 || cur_source == SOURCE_HDMI3) {
            mSourceDetectCurStatusBuf[i] = DetectHDMISource(cur_source);
        }
    }

    for (i = 0; i < SOURCE_MAX; i++) {
        if (mSourceDetectCurStatusBuf[i] != mSourceDetectPreStatusBuf[i]) {
            mSourceDetectPreStatusBuf[i] = mSourceDetectCurStatusBuf[i];
            LOGD("%s, Source id = %d, Source plug status = %d\n", "TV", mSourceDetectTable[i], mSourceDetectCurStatusBuf[i]);

#if CC_DEBUG_AV_SAMPLE_BUF == 1
            int *cur_sample_buf = NULL;

            if (mSourceDetectTable[i] == SOURCE_AV1) {
                cur_sample_buf = mAV1SampleDataBuf;
            } else if (mSourceDetectTable[i] == SOURCE_AV2) {
                cur_sample_buf = mAV2SampleDataBuf;
            }

            if (cur_sample_buf != NULL) {
                for (int j = 0; j < mAVDetectSampleSize; j++) {
                    LOGD("%s, cur_sample_buf[%d] = %d\n", "TV", j, cur_sample_buf[j]);
                }
                LOGD("%s, \n\n\n", "TV", i, cur_sample_buf[i]);
            }
#endif

            mpObserver->onSourceConnect(mSourceDetectTable[i], mSourceDetectCurStatusBuf[i]);
        }
    }

    return 0;
}

int CTvin::CSourceConnectDetect::DetectAVSource(int source_input)
{
    int i = 0, plug_in_cnt = 0, plug_out_cnt = 0;
    int detect_adc_chan = -1, cur_adc_val = 0;
    int *cur_sample_ind = 0;
    int *cur_sample_buf = NULL;

    if (source_input == SOURCE_AV1) {
        detect_adc_chan = mAV1DetectADCChan;
        cur_sample_ind = &mAV1CurSampleInd;
        cur_sample_buf = mAV1SampleDataBuf;
    } else if (source_input == SOURCE_AV2) {
        detect_adc_chan = mAV2DetectADCChan;
        cur_sample_ind = &mAV2CurSampleInd;
        cur_sample_buf = mAV2SampleDataBuf;
    } else {
        return CC_SOURCE_PLUG_OUT;
    }

    cur_adc_val = ReadADCSpecialChannelValue(detect_adc_chan);
    *cur_sample_ind = *cur_sample_ind % mAVDetectSampleSize;
    cur_sample_buf[*cur_sample_ind] = cur_adc_val;
    *cur_sample_ind = (*cur_sample_ind + 1 ) % mAVDetectSampleSize;

    plug_in_cnt = 0;
    plug_out_cnt = 0;
    for (i = 0; i < mAVDetectSampleSize; i++) {
        if (cur_sample_buf[i] >= mAVDetectPlugInADCThreshold) {
            plug_in_cnt += 1;
        } else if (cur_sample_buf[i] <= mAVDetectPlugOutADCThreshold) {
            plug_out_cnt += 1;
        }
    }

    if (mAVDetectMethod == CC_AV_DETECT_METHOD_ADC_PLUG_IN) {
        if ((plug_in_cnt * 100 / mAVDetectSampleSize) >= mAVDetectPlugInDutyCycle) {
            return CC_SOURCE_PLUG_IN;
        }

        return CC_SOURCE_PLUG_OUT;
    } else if (mAVDetectMethod == CC_AV_DETECT_METHOD_ADC_PLUG_OUT) {
        if ((plug_out_cnt * 100 / mAVDetectSampleSize) >= mAVDetectPlugOutDutyCycle) {
            return CC_SOURCE_PLUG_OUT;
        }

        return CC_SOURCE_PLUG_IN;
    }

    return CC_SOURCE_PLUG_OUT;
}

int CTvin::CSourceConnectDetect::DetectHDMISource(int source_input)
{
    int source_port = CTvin::Tvin_GetSourcePortBySourceInput((tv_source_input_t)source_input);

    if (mpTvin->VDIN_GetPortConnect(source_port) == true) {
        return CC_SOURCE_PLUG_IN;
    }

    return CC_SOURCE_PLUG_OUT;
}
