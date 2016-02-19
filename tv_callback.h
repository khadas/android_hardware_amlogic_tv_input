#ifndef TV_CALLBACK
#define TV_CALLBACK
#include "TvPlay.h"
class TvCallback : public TvPlayObserver {
public:
    TvCallback(void *data)
    {
        mPri = data;
    }
    ~TvCallback() {}
    void onTvEvent (int32_t msgType, const Parcel &p);

private:
    void *mPri;
};
#endif

