#define LOG_TAG "Tv"
#include <utils/Log.h>
#include <utils/threads.h>
#include <signal.h>
#include <binder/IServiceManager.h>
#include <binder/IMemory.h>

#include <include/Tv.h>
#include <include/ITvService.h>

// client singleton for tv service binder interface
Mutex Tv::mLock;
sp<ITvService> Tv::mTvService;
sp<Tv::DeathNotifier> Tv::mDeathNotifier;

// establish binder interface to tv service
const sp<ITvService> &Tv::getTvService()
{
	Mutex::Autolock _l(mLock);
	if (mTvService.get() == 0) {
		sp<IServiceManager> sm = defaultServiceManager();
		sp<IBinder> binder;
		do {
			binder = sm->getService(String16("aml.tvserver"));
			if (binder != 0)
				break;
			ALOGW("TvService not published, waiting...");
			usleep(500000); // 0.5 s
		} while (true);
		if (mDeathNotifier == NULL) {
			mDeathNotifier = new DeathNotifier();
		}
		binder->linkToDeath(mDeathNotifier);
		mTvService = interface_cast<ITvService>(binder);
	}
	ALOGE_IF(mTvService == 0, "no TvService!?");
	return mTvService;
}

Tv::Tv()
{
	init();
}

// construct a tv client from an existing tv remote
sp<Tv> Tv::create(const sp<ITv> &tv)
{
	ALOGD("create");
	if (tv == 0) {
		ALOGE("tv remote is a NULL pointer");
		return 0;
	}

	sp<Tv> c = new Tv();
	if (tv->connect(c) == NO_ERROR) {
		c->mStatus = NO_ERROR;
		c->mTv = tv;
		IInterface::asBinder(tv)->linkToDeath(c);
	}
	return c;
}

void Tv::init()
{
	mStatus = UNKNOWN_ERROR;
}

Tv::~Tv()
{
	disconnect();
}

sp<Tv> Tv::connect()
{
	ALOGD("Tv::connect------------------------------------------");
	sp<Tv> c = new Tv();
	const sp<ITvService> &cs = getTvService();
	if (cs != 0) {
		c->mTv = cs->connect(c);
	}
	if (c->mTv != 0) {
		IInterface::asBinder(c->mTv)->linkToDeath(c);
		c->mStatus = NO_ERROR;
	} else {
		c.clear();
	}
	return c;
}

void Tv::disconnect()
{
	ALOGD("disconnect");
	if (mTv != 0) {
		mTv->disconnect();
		IInterface::asBinder(mTv)->unlinkToDeath(this);
		mTv = 0;
	}
}

status_t Tv::reconnect()
{
	ALOGD("reconnect");
	sp <ITv> c = mTv;
	if (c == 0) return NO_INIT;
	return c->connect(this);
}

sp<ITv> Tv::remote()
{
	return mTv;
}

status_t Tv::lock()
{
	sp <ITv> c = mTv;
	if (c == 0) return NO_INIT;
	return c->lock();
}

status_t Tv::unlock()
{
	sp <ITv> c = mTv;
	if (c == 0) return NO_INIT;
	return c->unlock();
}

status_t Tv::processCmd(const Parcel &p, Parcel *r)
{
	sp <ITv> c = mTv;
	if (c == 0) return NO_INIT;
	return c->processCmd(p, r);
}

//
status_t Tv::createSubtitle(const sp<IMemory> &share_mem)
{
	sp <ITv> c = mTv;
	if (c == 0) return NO_INIT;
	return c->createSubtitle(share_mem);
}

status_t    Tv::createVideoFrame(const sp<IMemory> &share_mem, int iSourceMode, int iCapVideoLayerOnly)
{
	sp <ITv> c = mTv;
	if (c == 0) return NO_INIT;
	return c->createVideoFrame(share_mem, iSourceMode, iCapVideoLayerOnly);
}


void Tv::setListener(const sp<TvListener> &listener)
{
	ALOGD("tv------------Tv::setListener");
	Mutex::Autolock _l(mLock);
	mListener = listener;
}


// callback from tv service
void Tv::notifyCallback(int32_t msgType, const Parcel &p)
{
	int size = p.dataSize();
	int pos = p.dataPosition();
	p.setDataPosition(0);
	sp<TvListener> listener;
	{
		Mutex::Autolock _l(mLock);
		listener = mListener;
	}
	if (listener != NULL) {
		listener->notify(msgType, p);
	}
}

void Tv::binderDied(const wp<IBinder> &who)
{
	ALOGW("ITv died");
	//notifyCallback(1, 2, 0);
}

void Tv::DeathNotifier::binderDied(const wp<IBinder> &who)
{
	ALOGW("-----------------binderDied");
	Mutex::Autolock _l(Tv::mLock);
	Tv::mTvService.clear();
	ALOGW("kill myself");
	kill(getpid(), SIGKILL);
	ALOGW("----------------Tv server died!");
}

