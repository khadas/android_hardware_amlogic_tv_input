#define LOG_TAG "TvPlay"

#include <utils/Log.h>
#include <string.h>
#include "TvPlay.h"
#include "tvcmd.h"

TvPlay::TvPlay()
{
    mpObserver = NULL;
    tvSession = TvClient::connect();
    tvSession->setListener(this);
    mHdmiPorts = getHdmiPorts();
}

TvPlay::~TvPlay()
{
    tvSession.clear();
}

int TvPlay::setTvObserver ( TvPlayObserver *ob )
{
    mpObserver = ob;
    return 0;
}

void TvPlay::notify(int32_t msgType, const Parcel &p)
{
    ALOGD("TvPlay-------notify-------");
    if (mpObserver != NULL)
        mpObserver->onTvEvent(msgType, p);
}

int TvPlay::StartTv()
{
    Parcel p, r;
    p.writeInt32(START_TV);
    tvSession->processCmd(p, &r);
    return r.readInt32();
}

int TvPlay::StopTv()
{
    Parcel p, r;
    p.writeInt32(STOP_TV);
    tvSession->processCmd(p, &r);
    return r.readInt32();
}

int TvPlay::SwitchSourceInput(tv_source_input_t source_input)
{
    Parcel p, r;
    p.writeInt32(SET_SOURCE_INPUT);
    p.writeInt32(source_input);
    tvSession->processCmd(p, &r);
    return r.readInt32();
}

int TvPlay::DoSuspend(int type)
{
    Parcel p, r;
    p.writeInt32(DO_SUSPEND);
    p.writeInt32(type);
    tvSession->processCmd(p, &r);
    return r.readInt32();
}

int TvPlay::DoResume(int type)
{
    Parcel p, r;
    p.writeInt32(DO_RESUME);
    p.writeInt32(type);
    tvSession->processCmd(p, &r);
    return r.readInt32();
}

int TvPlay::GetSourceConnectStatus(tv_source_input_t source_input)
{
    Parcel p, r;
    p.writeInt32(GET_SOURCE_CONNECT_STATUS);
    p.writeInt32(source_input);
    tvSession->processCmd(p, &r);
    return r.readInt32();
}

int TvPlay::GetCurrentSourceInput()
{
    Parcel p, r;
    p.writeInt32(GET_CURRENT_SOURCE_INPUT_VIRTUAL);
    tvSession->processCmd(p, &r);
    return r.readInt32();
}

int TvPlay::GetHdmiAvHotplugDetectOnoff()
{
    Parcel p, r;
    p.writeInt32(HDMIAV_HOTPLUGDETECT_ONOFF);
    tvSession->processCmd(p, &r);
    return r.readInt32();
}

int TvPlay::getAllTvDevices(int *devices, int *count)
{
    Parcel p, r;
    p.writeInt32(GET_ALL_TV_DEVICES);
    tvSession->processCmd(p, &r);
    const char *input_list = r.readCString();
    ALOGD("input_list = %s", input_list);

    int len = 0;
    const char *seg = ",";
    char *pT = strtok((char*)input_list, seg);
    while (pT) {
        len ++;
        *devices = atoi(pT);
        ALOGD("devices: %d: %d", len , *devices);
        devices ++;
        pT = strtok(NULL, seg);
    }
    *count = len;
    return 0;
}

int TvPlay::getHdmiPorts()
{
    Parcel p, r;
    p.writeInt32(GET_HDMI_PORTS);
    tvSession->processCmd(p, &r);

    return r.readInt32();
}

int TvPlay::getHdmiPort(tv_source_input_t source_input) {
    int max_port_num = 3;
    if ( (source_input - SOURCE_HDMI1) > (max_port_num-1))
        max_port_num = (max_port_num << 1) + 1;
    return mHdmiPorts == 0 ? 0 : max_port_num & (mHdmiPorts >> (2* (source_input - SOURCE_HDMI1)));
}

