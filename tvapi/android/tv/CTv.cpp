#include <utils/Log.h>
#include "CTv.h"
#include "../include/tvcmd.h"

CTv::CTv() {
	mpObserver = NULL;
	tv = Tv::connect();
	tv->setListener(this);
}

CTv::~CTv() {
	tv.clear();
}

int CTv::setTvObserver ( TvIObserver *ob )
{
	mpObserver = ob;
	return 0;
}

void CTv::notify(int32_t msgType, const Parcel &p)
{
	ALOGD("CTv-------notify-------");
	//if(mpObserver)
	//mpObserver->onTvEvent(msgType, p);
	if (mpObserver != NULL)
		mpObserver->onTvEvent(msgType, p);
}

int CTv::StartTv()
{
	Parcel p, r;
	p.writeInt32(START_TV);
	tv->processCmd(p, &r);
	int ret = r.readInt32();
	return ret;
}

int CTv::StopTv()
{
	Parcel p, r;
	p.writeInt32(STOP_TV);
	tv->processCmd(p, &r);
	int ret = r.readInt32();
	return ret;
}

int CTv::SwitchSourceInput(tv_source_input_t source_input)
{
	Parcel p, r;
	p.writeInt32(SET_SOURCE_INPUT);
	p.writeInt32(source_input);
	tv->processCmd(p, &r);
	int ret = r.readInt32();
	return ret;
}

int CTv::DoSuspend(int type)
{
	Parcel p, r;
	p.writeInt32(DO_SUSPEND);
	p.writeInt32(type);
	tv->processCmd(p, &r);
	int ret = r.readInt32();
	return ret;
}

int  CTv::DoResume(int type)
{
	Parcel p, r;
	p.writeInt32(DO_RESUME);
	p.writeInt32(type);
	tv->processCmd(p, &r);
	int ret = r.readInt32();
	return ret;
}

int CTv::GetSourceConnectStatus(tv_source_input_t source_input)
{
	Parcel p, r;
	p.writeInt32(GET_SOURCE_CONNECT_STATUS);
	p.writeInt32(source_input);
	tv->processCmd(p, &r);
	int ret = r.readInt32();
	return ret;
}

int CTv::GetCurrentSourceInput()
{
	Parcel p, r;
	p.writeInt32(SSM_READ_SOURCE_INPUT);
	tv->processCmd(p, &r);
	int ret = r.readInt32();
	return ret;
}

int CTv::GetHdmiAvHotplugDetectOnoff()
{
	Parcel p, r;
	p.writeInt32(HDMIAV_HOTPLUGDETECT_ONOFF);
	tv->processCmd(p, &r);
	int ret = r.readInt32();
	return ret;
}
