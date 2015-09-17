#define LOG_TAG "Tv-JNI"
#include <utils/Log.h>

#include "jni.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"
#include <utils/Vector.h>
#include <include/Tv.h>
#include <binder/IMemory.h>
#include <binder/Parcel.h>
#include <binder/MemoryHeapBase.h>
#include <binder/MemoryBase.h>
#include <core/SkBitmap.h>
#include "android_util_Binder.h"
#include "android_os_Parcel.h"
using namespace android;

struct fields_t {
	jfieldID context;
	jmethodID post_event;
};

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "TvJNI"
#endif

static fields_t fields;
static Mutex sLock;
class JNITvContext: public TvListener {
public:
	JNITvContext(JNIEnv *env, jobject weak_this, jclass clazz, const sp<Tv> &tv);
	~JNITvContext()
	{
		release();
	}
	virtual void notify(int32_t msgType, const Parcel &p);
	void addCallbackBuffer(JNIEnv *env, jbyteArray cbb);
	sp<Tv> getTv()
	{
		Mutex::Autolock _l(mLock);
		return mTv;
	}
	void release();
	Parcel *mExtParcel;
private:
	jobject     mTvJObjectWeak;     // weak reference to java object
	jclass      mTvJClass;          // strong reference to java class
	sp<Tv>      mTv;                // strong reference to native object
	Mutex       mLock;

	Vector<jbyteArray> mCallbackBuffers;    // Global reference application managed byte[]
	bool mManualBufferMode;                 // Whether to use application managed buffers.
	bool mManualTvCallbackSet;              // Whether the callback has been set, used to reduce unnecessary calls to set the callback.
};

sp<Tv> get_native_tv(JNIEnv *env, jobject thiz, JNITvContext **pContext)
{
	sp<Tv> tv;
	Mutex::Autolock _l(sLock);
	JNITvContext *context = reinterpret_cast<JNITvContext *>(env->GetIntField(thiz, fields.context));
	if (context != NULL) {
		tv = context->getTv();
	}
	if (tv == 0) {
		jniThrowException(env, "java/lang/RuntimeException", "Method called after release()");
	}

	if (pContext != NULL) *pContext = context;
	return tv;
}

JNITvContext::JNITvContext(JNIEnv *env, jobject weak_this, jclass clazz, const sp<Tv> &tv)
{
	mTvJObjectWeak = env->NewGlobalRef(weak_this);
	mTvJClass = (jclass)env->NewGlobalRef(clazz);
	mTv = tv;
	ALOGD("tvjni----------------------JNITvContext::JNITvContext(");
	mManualBufferMode = false;
	mManualTvCallbackSet = false;
	//mExtParcel = parcelForJavaObject(env, ext_parcel);
}

void JNITvContext::release()
{
	ALOGD("release");
	Mutex::Autolock _l(mLock);
	JNIEnv *env = AndroidRuntime::getJNIEnv();

	if (mTvJObjectWeak != NULL) {
		env->DeleteGlobalRef(mTvJObjectWeak);
		mTvJObjectWeak = NULL;
	}
	if (mTvJClass != NULL) {
		env->DeleteGlobalRef(mTvJClass);
		mTvJClass = NULL;
	}
	mTv.clear();
}

// connect to tv service
static void android_amlogic_Tv_native_setup(JNIEnv *env, jobject thiz, jobject weak_this)
{
	sp<Tv> tv = Tv::connect();

	ALOGD("android_amlogic_Tv_native_setup.");

	if (tv == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "Fail to connect to tv service");
		return;
	}

	// make sure tv amlogic is alive
	if (tv->getStatus() != NO_ERROR) {
		jniThrowException(env, "java/lang/RuntimeException", "Tv initialization failed!");
		return;
	}

	jclass clazz = env->GetObjectClass(thiz);
	if (clazz == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "Can't find android/amlogic/Tv!");
		return;
	}

	sp<JNITvContext> context = new JNITvContext(env, weak_this, clazz, tv);
	context->incStrong(thiz);
	tv->setListener(context);

	env->SetIntField(thiz, fields.context, (int)context.get());
}


static void android_amlogic_Tv_release(JNIEnv *env, jobject thiz)
{
	// TODO: Change to LOGE
	JNITvContext *context = NULL;
	sp<Tv> tv;
	{
		Mutex::Autolock _l(sLock);
		context = reinterpret_cast<JNITvContext *>(env->GetIntField(thiz, fields.context));

		// Make sure we do not attempt to callback on a deleted Java object.
		env->SetIntField(thiz, fields.context, 0);
	}

	ALOGD("release tv");

	// clean up if release has not been called before
	if (context != NULL) {
		tv = context->getTv();
		context->release();
		ALOGD("native_release: context=%p tv=%p", context, tv.get());

		// clear callbacks
		if (tv != NULL) {
			//tv->setPreviewCallbackFlags(FRAME_CALLBACK_FLAG_NOOP);
			tv->disconnect();
		}

		// remove context to prevent further Java access
		context->decStrong(thiz);
	}
}

void JNITvContext::notify(int32_t msgType, const Parcel &p)
{
	// VM pointer will be NULL if object is released
	Mutex::Autolock _l(mLock);
	if (mTvJObjectWeak == NULL) {
		ALOGW("callback on dead tv object");
		return;
	}

	JNIEnv *env = AndroidRuntime::getJNIEnv();

	jobject jParcel = createJavaParcelObject(env);
	if (jParcel != NULL) {
		Parcel *nativeParcel = parcelForJavaObject(env, jParcel);
		nativeParcel->write(p.data(), p.dataSize());
		env->CallStaticVoidMethod(mTvJClass, fields.post_event, mTvJObjectWeak, msgType, jParcel);
		env->DeleteLocalRef(jParcel);
	}
}


void JNITvContext::addCallbackBuffer(JNIEnv *env, jbyteArray cbb)
{
	if (cbb != NULL) {
		Mutex::Autolock _l(mLock);
		jbyteArray callbackBuffer = (jbyteArray)env->NewGlobalRef(cbb);
		mCallbackBuffers.push(cbb);
		ALOGD("Adding callback buffer to queue, %d total", mCallbackBuffers.size());
	} else {
		ALOGE("Null byte array!");
	}
}

static jint android_amlogic_Tv_processCmd(JNIEnv *env, jobject thiz, jobject pObj, jobject rObj)
{
	sp<Tv> tv = get_native_tv(env, thiz, NULL);
	if (tv == 0) return -1;

	Parcel *p = parcelForJavaObject(env, pObj);
	//jclass clazz;
	//clazz = env->FindClass("android/os/Parcel");
	//LOG_FATAL_IF(clazz == NULL, "Unable to find class android.os.Parcel");


	//jmethodID mConstructor = env->GetMethodID(clazz, "<init>", "(I)V");
	//jobject replayobj = env->NewObject(clazz, mConstructor, 0);
	Parcel *r = parcelForJavaObject(env, rObj);


	return tv->processCmd(*p, r);
	//if ( != NO_ERROR) {
	//    jniThrowException(env, "java/lang/RuntimeException", "StartTv failed");
	//    return -1;
	// }
	//return 0;
}

static void android_amlogic_Tv_addCallbackBuffer(JNIEnv *env, jobject thiz, jbyteArray bytes)
{
	JNITvContext *context = reinterpret_cast<JNITvContext *>(env->GetIntField(thiz, fields.context));

	ALOGD("addCallbackBuffer");
	if (context != NULL) {
		context->addCallbackBuffer(env, bytes);
	}
}

static void android_amlogic_Tv_reconnect(JNIEnv *env, jobject thiz)
{
	sp<Tv> tv = get_native_tv(env, thiz, NULL);
	if (tv == 0) return;

	if (tv->reconnect() != NO_ERROR) {
		jniThrowException(env, "java/io/IOException", "reconnect failed");
		return;
	}
}

static void android_amlogic_Tv_lock(JNIEnv *env, jobject thiz)
{
	sp<Tv> tv = get_native_tv(env, thiz, NULL);
	if (tv == 0) return;

	ALOGD("lock");

	if (tv->lock() != NO_ERROR) {
		jniThrowException(env, "java/lang/RuntimeException", "lock failed");
	}
}

static void android_amlogic_Tv_unlock(JNIEnv *env, jobject thiz)
{
	sp<Tv> tv = get_native_tv(env, thiz, NULL);
	if (tv == 0) return;

	ALOGD("unlock");

	if (tv->unlock() != NO_ERROR) {
		jniThrowException(env, "java/lang/RuntimeException", "unlock failed");
	}
}

static void android_amlogic_Tv_create_subtitle_bitmap(JNIEnv *env, jobject thiz, jobject bmpobj)
{
	ALOGD("create subtitle bmp");
	sp<Tv> tv = get_native_tv(env, thiz, NULL);
	if (tv == 0) return;

	//get skbitmap
	jclass bmp_clazz;
	jfieldID skbmp_fid;
	jint hbmp;
	bmp_clazz = env->FindClass("android/graphics/Bitmap");
	skbmp_fid  = env->GetFieldID(bmp_clazz, "mNativeBitmap", "I");
	hbmp = env->GetIntField(bmpobj, skbmp_fid);
	SkBitmap *pSkBmp = reinterpret_cast<SkBitmap *>(hbmp);
	ALOGD("pSkBmp = %d", hbmp);
	ALOGD("bmp width = %d height = %d", pSkBmp->width(), pSkBmp->height());
	env->DeleteLocalRef(bmp_clazz);

	//alloc share mem
	sp<MemoryHeapBase> MemHeap = new MemoryHeapBase(1920 * 1080 * 4, 0, "subtitle bmp");
	ALOGD("heap id = %d", MemHeap->getHeapID());
	if (MemHeap->getHeapID() < 0) {
		return;
	}
	sp<MemoryBase> MemBase = new MemoryBase(MemHeap, 0, 1920 * 1080 * 4);
	pSkBmp->setPixels(MemBase->pointer());


	//send share mem to server
	tv->createSubtitle(MemBase);
	return;
}

static void android_amlogic_Tv_create_video_frame_bitmap(JNIEnv *env, jobject thiz, jobject bmpobj )
{
	ALOGD("create video frame bmp");
	sp<Tv> tv = get_native_tv(env, thiz, NULL);
	if (tv == 0) return;

	//get skbitmap
	jclass bmp_clazz;
	jfieldID skbmp_fid;
	jint hbmp;
	bmp_clazz = env->FindClass("android/graphics/Bitmap");
	skbmp_fid  = env->GetFieldID(bmp_clazz, "mNativeBitmap", "I");
	hbmp = env->GetIntField(bmpobj, skbmp_fid);
	SkBitmap *pSkBmp = reinterpret_cast<SkBitmap *>(hbmp);
	ALOGD("pSkBmp = %d", hbmp);
	ALOGD("bmp width = %d height = %d", pSkBmp->width(), pSkBmp->height());
	env->DeleteLocalRef(bmp_clazz);

	//alloc share mem
	sp<MemoryHeapBase> MemHeap = new MemoryHeapBase(1280 * 720 * 4 + 1, 0, "video frame bmp");
	ALOGD("heap id = %d", MemHeap->getHeapID());
	if (MemHeap->getHeapID() < 0) {
		return;
	}
	sp<MemoryBase> MemBase = new MemoryBase(MemHeap, 0, 1280 * 720 * 4 + 1);
	pSkBmp->setPixels(MemBase->pointer());


	//send share mem to server
	tv->createVideoFrame(MemBase);
	return;
}

//-------------------------------------------------

static JNINativeMethod camMethods[] = {
	{
		"native_setup",
		"(Ljava/lang/Object;)V",
		(void *)android_amlogic_Tv_native_setup
	},
	{
		"native_release",
		"()V",
		(void *)android_amlogic_Tv_release
	},
	{
		"processCmd",
		"(Landroid/os/Parcel;Landroid/os/Parcel;)I",
		(void *)android_amlogic_Tv_processCmd
	},
	{
		"addCallbackBuffer",
		"([B)V",
		(void *)android_amlogic_Tv_addCallbackBuffer
	},
	{
		"reconnect",
		"()V",
		(void *)android_amlogic_Tv_reconnect
	},
	{
		"lock",
		"()V",
		(void *)android_amlogic_Tv_lock
	},
	{
		"unlock",
		"()V",
		(void *)android_amlogic_Tv_unlock
	},
	{
		"native_create_subtitle_bitmap",
		"(Ljava/lang/Object;)V",
		(void *)android_amlogic_Tv_create_subtitle_bitmap
	},
	{
		"native_create_video_frame_bitmap",
		"(Ljava/lang/Object;)V",
		(void *)android_amlogic_Tv_create_video_frame_bitmap
	},

};

struct field {
	const char *class_name;
	const char *field_name;
	const char *field_type;
	jfieldID   *jfield;
};

static int find_fields(JNIEnv *env, field *fields, int count)
{
	for (int i = 0; i < count; i++) {
		field *f = &fields[i];
		jclass clazz = env->FindClass(f->class_name);
		if (clazz == NULL) {
			ALOGE("Can't find %s", f->class_name);
			return -1;
		}

		jfieldID field = env->GetFieldID(clazz, f->field_name, f->field_type);
		if (field == NULL) {
			ALOGE("Can't find %s.%s", f->class_name, f->field_name);
			return -1;
		}

		*(f->jfield) = field;
	}

	return 0;
}

// Get all the required offsets in java class and register native functions
int register_android_amlogic_Tv(JNIEnv *env)
{
	field fields_to_find[] = {
		{ "android/amlogic/Tv", "mNativeContext",   "I", &fields.context }
	};

	ALOGD("register_android_amlogic_Tv.");

	if (find_fields(env, fields_to_find, NELEM(fields_to_find)) < 0)
		return -1;

	jclass clazz = env->FindClass("android/amlogic/Tv");
	fields.post_event = env->GetStaticMethodID(clazz, "postEventFromNative", "(Ljava/lang/Object;ILandroid/os/Parcel;)V");
	if (fields.post_event == NULL) {
		ALOGE("Can't find android/amlogic/Tv.postEventFromNative");
		return -1;
	}

	// Register native functions
	return AndroidRuntime::registerNativeMethods(env, "android/amlogic/Tv", camMethods, NELEM(camMethods));
}


jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
	JNIEnv *env = NULL;
	jint result = -1;

	if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
		ALOGE("ERROR: GetEnv failed\n");
		goto bail;
	}
	assert(env != NULL);

	register_android_amlogic_Tv(env);

	/* success -- return valid version number */
	result = JNI_VERSION_1_4;
bail:
	return result;
}

