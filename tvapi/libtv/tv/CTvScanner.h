//
//
//  amlogic 2013
//
//  @ Project : tv
//  @ File Name :
//  @ Date : 2013-11
//  @ Author :
//
//
#include <am_scan.h>
#include <am_epg.h>
#include <am_mem.h>
#include "CTvChannel.h"
#include "CTvLog.h"
#include "CTvEv.h"
#include "tvin/CTvin.h"

#if !defined(_CTVSCANNER_H)
#define _CTVSCANNER_H
class CTvScanner {
public:
	/** ATSC Attenna type */
	static const int AM_ATSC_ATTENNA_TYPE_AIR = 1;
	static const int AM_ATSC_ATTENNA_TYPE_CABLE_STD = 2;
	static const int AM_ATSC_ATTENNA_TYPE_CABLE_IRC = 3;
	static const int AM_ATSC_ATTENNA_TYPE_CABLE_HRC = 4;
	CTvScanner(CTvin *pTvin);
	~CTvScanner();
	void setGlobalScanerObject(CTvScanner *);
	int startScan();
	int stopScan();
	int ATVManualScan(int min_freq, int max_freq, int std, int store_Type = 0, int channel_num = 0);
	int autoAtvScan(int min_freq, int max_freq, int std, int search_type);
	int autoDtmbScan();
	int manualDtmbScan(int beginFreq, int endFreq, int modulation = -1);
	int autoAtscScan(int attennaType, int std);
	int manualDtmbScan(int freq);
	int manualAtscScan(int freq, int attennaType, int std);
	int autoTvScan();
	int unsubscribeEvent();

	class ScannerEvent: public CTvEv {
	public:
		static const int EVENT_SCAN_PROGRESS = 0;
		static const int EVENT_STORE_BEGIN   = 1;
		static const int EVENT_STORE_END     = 2;
		static const int EVENT_SCAN_END     = 3;
		static const int EVENT_BLINDSCAN_PROGRESS = 4;
		static const int EVENT_BLINDSCAN_NEWCHANNEL = 5;
		static const int EVENT_BLINDSCAN_END    = 6;
		static const int EVENT_ATV_PROG_DATA = 7;

		ScannerEvent(): CTvEv(CTvEv::TV_EVENT_SCANNER)
		{
		}
		~ScannerEvent()
		{
		}
		int mType;
		int mPercent;
		int mTotalChannelCount;
		int mLockedStatus;
		int mChannelNumber;
		//CTvChannel mChannel;
		int mMode;
		int mFrequency;
		int mSymbolRate;
		int mModulation;
		int mBandwidth;
		int mOfdm_mode;
		int mAudio;
		int mStandard;
		int mSat_polarisation;
		//
		int mStrength;
		int mSnr;
		char mProgramName[1024];
		int mprogramType;
		char mMSG[128];
		//for atv
		int mVideoStd;
		int mAudioStd;
		int mIsAutoStd;//1 is true
		int mAfcData;
		//      ScannerEvent(int type){
		//          this->mType = type;
		//      }
	};

	class IObserver {
	public:
		IObserver() {};
		virtual ~IObserver() {};
		virtual void onEvent(const ScannerEvent &ev) = 0;
	};
	//1 VS n
	//int addObserver(IObserver* ob);
	//int removeObserver(IObserver* ob);

	//1 VS 1
	int setObserver(IObserver *ob)
	{
		mpObserver = ob;
		return 0;
	}
private:
	static AM_Bool_t s_atv_cvbs_lock_check(void *);
	AM_Bool_t atv_cvbs_lock_check(v4l2_std_id  *colorStd);
	static void tv_scan_evt_callback(long dev_no, int event_type, void *param, void *data);
	static void am_scan_atv_store(AM_SCAN_Result_t *result);
	void tv_scan_reconnect_dmx_to_fend(int dmx_no, int fend_no);
	int getAtscChannelPara(int attennaType, Vector<sp<CTvChannel> > &vcp);
	//
	AM_SCAN_Handle_t mScanHandle;
	volatile bool mbScanStart;

	//scan para info
	/** General TV Scan Mode */
	static const int TV_MODE_ATV = 0;   // Only search ATV
	static const int TV_MODE_DTV = 1;   // Only search DTV
	static const int TV_MODE_ADTV = 2;  // A/DTV will share a same frequency list, like ATSC
	/** DTV scan mode */
	static const int DTV_MODE_AUTO   = 1;
	static const int DTV_MODE_MANUAL = 2;
	static const int DTV_MODE_ALLBAND = 3;
	static const int DTV_MODE_BLIND  = 4;

	/** DTV scan options, DONOT channge */
	static const int DTV_OPTION_UNICABLE = 0x10;      //Satellite unicable mode
	static const int DTV_OPTION_FTA      = 0x20;      //Only store free programs
	static const int DTV_OPTION_NO_TV    = 0x40;      //Only store tv programs
	static const int DTV_OPTION_NO_RADIO = 0x80;      //Only store radio programs

	/** ATV scan mode */
	static const int ATV_MODE_AUTO   = 1;
	static const int ATV_MODE_MANUAL = 2;

	//
private:

	//
	IObserver *mpObserver;
	//
	CTvin *mpTvin;
	int mMode;
	int mFendID;
	/** DTV parameters */
	int mTvMode;
	int mTvOptions;
	int mSat_id;
	int mSource;
	//showboz
	//TVSatelliteParams tv_satparams;
	int mTsSourceID;
	CTvChannel mStartChannel;
	Vector<CTvChannel> mvChooseListChannels;
	/** ATV parameters */
	int mAtvMode;
	int mStartFreq;
	int mDirection;
	int mChannelID;



	//extern for scanner
	//int channelID; //can be used for manual scan
	/** Atv set */
	int mMinFreq;
	int mMaxFreq;
	long long mCurScanStartFreq;
	long long mCurScanEndFreq;
	int tunerStd;
	/** Tv set */
	int demuxID;//default 0
	String8 defaultTextLang;
	String8 orderedTextLangs;
	//showboz
	//Vector<CTvChannel> ChannelList;//VS mvChooseListChannels

	/** Dtv-Sx set Unicable settings*/
	int user_band;
	int ub_freq;//!< kHz

	ScannerEvent mCurEv;
	static CTvScanner *m_s_Scanner;
};
#endif //CTVSCANNER_H
