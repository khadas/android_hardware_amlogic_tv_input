#ifndef __TV_AUDIO_ANDROID_H__
#define __TV_AUDIO_ANDROID_H__

#define CC_FLAG_CREATE_RECORD           (0x0001)
#define CC_FLAG_CREATE_TRACK            (0x0002)
#include "audio_alsa.h"
#include <media/AudioRecord.h>
#include <media/AudioTrack.h>

using namespace android;


class CAndroidAudio
{
public:
    CAndroidAudio(CAudioAlsa *);
    ~CAndroidAudio();
    int amAndroidInit(int tm_sleep, int init_flag, int recordSr, int trackSr,
                      bool enable_noise_gat);
    int amAndroidUninit(int tm_sleep);
    int amAndroidSetRecorderSr(int sr);
    int amAndroidSetTrackerSr(int sr);
    int amAndroidSetDumpDataFlag(int tmp_flag);
    int amAndroidGetDumpDataFlag();
    void amAndroidSetNoiseGateThreshold(int thresh);
    void reset_system_framesize(int input_sample_rate, int output_sample_rate);

private:
    void DoDumpData(void *data_buf, int size);
    void noise_filter_init();
    int noise_filter_left(int);
    int noise_filter_right(int);
    void DeleteAudioRecorder();
    void FreeAudioRecorder();
    void DeleteAudioTracker();
    void FreeAudioTracker();
    int InitTempBuffer();
    void FreeTempBuffer();
    void ResetRecordWritePointer();
    void ResetPlaybackReadPointer();
    void ResetPointer();
    short int clip(int);
    int upsample(short int *, short int *, unsigned int, unsigned int, unsigned int);
    int downsample(short int *, short int *, unsigned int, unsigned int, unsigned int);
    int GetWriteSpace(volatile short int *, volatile short int *);
    int GetReadSpace(volatile short int *, volatile short int *);
    void recorderCallback(int, void *, void *);
    void trackerCallback(int, void *, void *);
    int initAudioTracker(int);
    int initAudioRecorder(int);

    CAudioAlsa *mpAudioAlsa;
    int mDumpDataFlag;
    int mDumpDataFd;
    AudioRecord *mlpRecorder;
    AudioTrack *mlpTracker;

#if ANDROID_PLATFORM_SDK_VERSION >= 19
    sp<AudioTrack> mmpAudioTracker;
    sp<AudioRecord> mmpAudioRecorder;
#endif

    short *temp_buffer;
    short *end_temp_buffer;
    short volatile *record_write_pointer;
    short volatile *playback_read_pointer;

    ///////////////////////////////////////////////////////////////////////////////
    // Helpers for the noise gate.
    bool gEnableNoiseGate;
    bool gUserSetEnableNoiseGate;
    unsigned zero_count_left;
    unsigned zero_count_right;
    unsigned NOISE_HIS;
    signed gNoiseGateThresh;
};


#endif //__TV_AUDIO_ANDROID_H__
