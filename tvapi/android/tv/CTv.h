#ifndef ANDROID_AMLOGIC_CTV_H
#define ANDROID_AMLOGIC_CTV_H

#include <include/Tv.h>

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
	SOURCE_VGA,
	SOURCE_MPEG,
	SOURCE_DTV,
	SOURCE_SVIDEO,
	SOURCE_IPTV,
	SOURCE_DUMMY,
	SOURCE_MAX,
} tv_source_input_t;

class CTv : public TvListener{
public:
	class TvIObserver
  {
  public:
    TvIObserver() {};
    virtual ~TvIObserver() {};
    virtual void onTvEvent (int32_t msgType, const Parcel &p) = 0;
  };
	CTv();
	~CTv();
	int StartTv();
	int StopTv();
	int SwitchSourceInput(tv_source_input_t source_input);
	//type   ,   1  is instaboot suspend
	int DoSuspend(int type);
	int DoResume(int type);
	int GetSourceConnectStatus(tv_source_input_t source_input);
	int GetCurrentSourceInput();
	int GetHdmiAvHotplugDetectOnoff();
	int setTvObserver (TvIObserver *ob);
	virtual void notify(int32_t msgType, const Parcel &p);

	sp<Tv> tv;

private:
	TvIObserver *mpObserver;
};

#endif
