#ifndef _ANDROID_TV_PLAY_H_
#define _ANDROID_TV_PLAY_H_

#ifdef __cplusplus
//extern "C" {
#endif

#include <TvClient.h>

typedef enum tv_source_input_e {
    SOURCE_INVALID = -1,
    SOURCE_TV = 0,
    SOURCE_AV1,
    SOURCE_AV2,
    SOURCE_YPBPR1,
    SOURCE_YPBPR2,
    SOURCE_HDMI1,
    SOURCE_HDMI2,
    SOURCE_HDMI3,
    SOURCE_HDMI4,
    SOURCE_VGA,
    SOURCE_MPEG,
    SOURCE_DTV,
    SOURCE_SVIDEO,
    SOURCE_IPTV,
    SOURCE_DUMMY,
    SOURCE_SPDIF,
    SOURCE_ADTV,
    SOURCE_MAX,
} tv_source_input_t;

typedef enum tv_source_type_e {
    TYPE_REAL,
    TYPE_VIRTUAL,
} tv_source_type_t;

class TvPlayObserver {
public:
    TvPlayObserver() {};
    virtual ~TvPlayObserver() {};
    virtual void onTvEvent (int32_t msgType, const Parcel &p) = 0;
};

class TvPlay : public TvListener {
public:
    TvPlay();
    ~TvPlay();
    int StartTv();
    int StopTv();
    int SwitchSourceInput(tv_source_input_t source_input);
    //type   ,   1  is instaboot suspend
    int DoSuspend(int type);
    int DoResume(int type);
    int GetSourceConnectStatus(tv_source_input_t source_input);
    int GetCurrentSourceInput();
    int GetHdmiAvHotplugDetectOnoff();
    int setTvObserver (TvPlayObserver *ob);
    int getAllTvDevices(int *devices, int *count);
    int getHdmiPorts();
    int getHdmiPort(tv_source_input_t source_input);
    virtual void notify(int32_t msgType, const Parcel &p);

    sp<TvClient> tvSession;

private:
    TvPlayObserver *mpObserver;
    int mHdmiPorts;
};

#ifdef __cplusplus
//}
#endif

#endif/*_ANDROID_TV_PLAY_H_*/
