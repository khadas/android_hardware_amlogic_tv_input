//
//
//  amlogic 2013
//
//  @ Project : tv
//  @ File Name : CTv.cpp
//  @ Date : 2013-11
//  @ Author :
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <am_epg.h>
#include <am_mem.h>
#include "CTvDatabase.h"
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/types.h>
#include <errno.h>
#include <dlfcn.h>
#include <cutils/properties.h>
#include <cutils/log.h>
#include <cutils/android_reboot.h>
#include <utils/threads.h>
#include <time.h>
#include <sys/prctl.h>
#include <getopt.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#ifdef ANDROID
#include <termios.h>
#endif
#include <string.h>
#include <signal.h>

#include "../version/version.h"
#include "../tvsetting/CTvSetting.h"
#include "../tvutils/tvutils.h"
#include "../tvconfig/tvconfig.h"
#include "../tvutils/CFile.h"
#include <hardware_legacy/power.h>
#include "../tvutils/serial_operate.h"

extern "C" {
#include "am_ver.h"
#include "am_misc.h"
#include "am_debug.h"
#include "am_fend.h"
}

#include <math.h>
#include <sys/ioctl.h>

#include "CTv.h"

#define LOG_TAG "CTv"


using namespace android;

// Called each time a message is logged.
static void sqliteLogCallback(void *data, int iErrCode, const char *zMsg)
{
	LOGD( "showbo sqlite (%d) %s\n", iErrCode, zMsg);
}



CTv::CTv() : mSigDetectThread ( &mTvin ), mSourceConnectDetectThread(&mTvin), mHDMIRxCEC ( &mTvin ), mTvScanner ( &mTvin ), mTvMsgQueue(this), mAutoBackLight( &mVpp, &mTvin )
{
	//mpClient = pClient;
	//copy file to param
	if ( Tv_Utils_IsFileExist ( TV_CONFIG_FILE_SYSTEM_PATH ) == 0 ) {
		if ( Tv_Utils_IsFileExist ( TV_CONFIG_FILE_PARAM_PATH ) != 0 ) {
			CFile file ( TV_CONFIG_FILE_SYSTEM_PATH );

			if ( file.copyTo ( TV_CONFIG_FILE_PARAM_PATH ) != 0 ) {
				LOGE ( "%s, copy file = %s , error", __FUNCTION__, TV_CONFIG_FILE_PARAM_PATH );
			}
		}
	}

	if ( Tv_Utils_IsFileExist ( TV_CHANNEL_LIST_SYSTEM_PATH ) == 0 ) {
		if ( Tv_Utils_IsFileExist ( TV_CHANNEL_LIST_PARAM_PATH ) != 0 ) {
			CFile file ( TV_CHANNEL_LIST_SYSTEM_PATH );

			if ( file.copyTo ( TV_CHANNEL_LIST_PARAM_PATH ) != 0 ) {
				LOGE ( "%s, copy file = %s , error", __FUNCTION__, TV_CHANNEL_LIST_PARAM_PATH );
			}
		}
	}

	//ssm
	if ( Tv_Utils_IsFileExist ( TV_SSM_DATA_SYSTEM_PATH ) == 0 ) {
		if ( Tv_Utils_IsFileExist ( TV_SSM_DATA_PARAM_PATH ) != 0 ) {
			CFile file ( TV_SSM_DATA_SYSTEM_PATH );

			if ( file.copyTo ( TV_SSM_DATA_PARAM_PATH ) != 0 ) {
				LOGE ( "%s, copy file = %s , error", __FUNCTION__, TV_SSM_DATA_PARAM_PATH );
			}
		}
	}

	AM_EVT_Init();
	mTvEpg.setObserver ( &mTvMsgQueue );
	mpObserver = NULL;
	audio_sr = 0;
	mIsFbc = false;
	dtv_auto_3d_flag = 0;
	m_autoset_displayfreq = false;
	m_cur_set_info_fmt = TVIN_SIG_FMT_CVBS_PAL_I;

	tv_config_load ( TV_CONFIG_FILE_PATH );
	sqlite3_config (SQLITE_CONFIG_SERIALIZED);
	//sqlite3_config(SQLITE_CONFIG_LOG, &sqliteLogCallback, (void*)1);
	sqlite3_soft_heap_limit(8 * 1024 * 1024);
	// Initialize SQLite.
	sqlite3_initialize();
	CTvDatabase::GetTvDb()->InitTvDb ( TV_DB_PATH );

	if ( CTvDimension::isDimensionTblExist() == false ) {
		CTvDimension::builtinAtscDimensions();
	}

	CTvSettingLoad();

	mDtvScanRunningStatus = DTV_SCAN_RUNNING_NORMAL;

	m_sig_stable_nums = 0;
	m_is_set_hdmi_edid = false;
	m_cur_playing_prog_id = -1;

	mSigDetectThread.setObserver ( this );
	mSourceConnectDetectThread.setObserver ( this );
	mHDMIRxCEC.setObserver(this);
	mFrontDev.setObserver ( &mTvMsgQueue );
	mpUpgradeFBC = NULL;
	if (GetPlatformHaveFBCFlag() == 1) {
		mIsFbc = true;

		mpUpgradeFBC = new CUpgradeFBC();
		mpUpgradeFBC->setObserver(this);
	}
	mSerialA.setObserver(this);
	mSerialB.setObserver(this);
	mSerialC.setObserver(this);
	mAv.setObserver(&mTvMsgQueue);
	mTvScanner.setGlobalScanerObject(&mTvScanner);
	mTvAction = mTvAction & TV_ACTION_NULL;
	mTvStatus = TV_INIT_ED;
	print_version_info();
};

CTv::~CTv()
{
	int iRet;
	mpObserver = NULL;
	CTvSettingunLoad();
	CTvDatabase::deleteTvDb();
	tv_config_unload();
	mAv.Close();
	mTvStatus = TV_INIT_ED;
	mFrontDev.Close();

	if (mpUpgradeFBC != NULL) {
		delete mpUpgradeFBC;
		mpUpgradeFBC = NULL;
	}
}

void CTv::onEvent ( const CTvScanner::ScannerEvent &ev )
{
	LOGD ( "%s, CTv::onEvent lockStatus = %d type = %d\n", __FUNCTION__,  ev.mLockedStatus, ev.mType );

	if ( mDtvScanRunningStatus == DTV_SCAN_RUNNING_ANALYZE_CHANNEL ) {
		if ( ev.mType == CTvScanner::ScannerEvent::EVENT_SCAN_END ) {
			CMessage msg;
			msg.mType = CTvMsgQueue::TV_MSG_STOP_ANALYZE_TS;
			msg.mpData = this;
			mTvMsgQueue.sendMsg ( msg );
		} else if ( ev.mType == CTvScanner::ScannerEvent::EVENT_STORE_END ) {
			CTvEpg::EpgEvent epgev;
			epgev.type = CTvEpg::EpgEvent::EVENT_CHANNEL_UPDATE_END;
			mDtvScanRunningStatus = DTV_SCAN_RUNNING_NORMAL;
			sendTvEvent ( epgev );
		}
	} else {
		sendTvEvent ( ev );
	}
}

void CTv::onEvent ( const CFrontEnd::FEEvent &ev )
{
	LOGD ( "%s, FE   event  type = %d tvaction=%x", __FUNCTION__, ev.mCurSigStaus, mTvAction);
	if(mTvAction & TV_ACTION_SCANNING) return;

	//前端事件响应处理
	if ( ev.mCurSigStaus  ==  CFrontEnd::FEEvent::EVENT_FE_HAS_SIG ) { //作为信号稳定
		if(/*m_source_input == SOURCE_TV || */m_source_input == SOURCE_DTV && (mTvAction & TV_ACTION_PLAYING)) { //atv and other tvin source    not to use it, and if not playing, not use have sig
			Vpp_SetDisplayMode ( mVpp.Tv_GetDisplayMode ( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) ), CTvin::Tvin_SourceInputToSourceInputType(m_source_input), mSigDetectThread.getCurSigInfo().fmt);
			if ( m_win_mode == PREVIEW_WONDOW ) {
				mAv.setVideoAxis(m_win_pos.x1, m_win_pos.y1, m_win_pos.x2, m_win_pos.y2);
				mAv.setVideoScreenMode ( CAv::VIDEO_WIDEOPTION_FULL_STRETCH );
			}

			if ( m_autoset_displayfreq) {
				mTvin.VDIN_SetDisplayVFreq ( 50, mAv.getVideoDisplayResolution() , mIsFbc);
			}
			TvEvent::SignalInfoEvent ev;
			ev.mStatus = TVIN_SIG_STATUS_STABLE;
			ev.mTrans_fmt = TVIN_TFMT_2D;
			ev.mFmt = TVIN_SIG_FMT_NULL;
			ev.mReserved = 0;
			sendTvEvent ( ev );
			mAv.EnableVideoAuto();
			mTvAudio.SetAudioMuteForTv ( CC_AUDIO_UNMUTE );
		}
	} else if ( ev.mCurSigStaus == CFrontEnd::FEEvent::EVENT_FE_NO_SIG ) { //作为信号消失
		if(/*m_source_input == SOURCE_TV || */m_source_input == SOURCE_DTV && (mTvAction & TV_ACTION_PLAYING)) { //just playing
			mAv.DisableVideoWithBlueColor();
			//
			TvEvent::SignalInfoEvent ev;
			ev.mStatus = TVIN_SIG_STATUS_NOSIG;
			ev.mTrans_fmt = TVIN_TFMT_2D;
			ev.mFmt = TVIN_SIG_FMT_NULL;
			ev.mReserved = 0;
			sendTvEvent ( ev );
		}
	}

}

void CTv::onEvent ( const CTvEpg::EpgEvent &ev )
{
	switch ( ev.type ) {
	case CTvEpg::EpgEvent::EVENT_TDT_END:
		LOGD ( "%s, CTv::onEvent epg time = %ld", __FUNCTION__, ev.time );
		mTvTime.setTime ( ev.time );
		break;

	case CTvEpg::EpgEvent::EVENT_CHANNEL_UPDATE: {
		LOGD ( "%s, CTv:: onEvent channel update", __FUNCTION__ );
		CMessage msg;
		msg.mType = CTvMsgQueue::TV_MSG_START_ANALYZE_TS;
		msg.mpData = this;
		mCurAnalyzeTsChannelID = ev.channelID;
		mTvMsgQueue.sendMsg ( msg );
		break;
	}

	default:
		break;
	}

	sendTvEvent ( ev );
}


void CTv::onEvent(const CAv::AVEvent &ev)
{
	LOGD("AVEvent = %d", ev.type);
	switch ( ev.type ) {
	case CAv::AVEvent::EVENT_AV_STOP: {
		break;
	}
	case CAv::AVEvent::EVENT_AV_RESUEM: {
		mAv.EnableVideoAuto();
		TvEvent::AVPlaybackEvent AvPlayBackEvt;
		AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_PLAYBACK_RESUME;
		AvPlayBackEvt.mProgramId = ( int ) ev.param;
		sendTvEvent(AvPlayBackEvt );
		break;
	}
	case CAv::AVEvent::EVENT_AV_SCAMBLED: {
		mAv.DisableVideoWithBlueColor();
		TvEvent::AVPlaybackEvent AvPlayBackEvt;
		AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_SCAMBLED;
		AvPlayBackEvt.mProgramId = ( int ) ev.param;
		sendTvEvent(AvPlayBackEvt );
		break;
	}
	case CAv::AVEvent::EVENT_AV_UNSUPPORT: {
		LOGD("To AVS or AVS+ format");//just avs format,  not unsupport, and avs avs+
		break;
	}
	default:
		break;
	}
}

CTv::CTvMsgQueue::CTvMsgQueue(CTv *tv)
{
	mpTv = tv;
}

CTv::CTvMsgQueue::~CTvMsgQueue()
{
}

void CTv::CTvMsgQueue::handleMessage ( CMessage &msg )
{
	LOGD ( "%s, CTv::CTvMsgQueue::handleMessage type = %d", __FUNCTION__,  msg.mType );

	switch ( msg.mType ) {
	case TV_MSG_COMMON:
		break;

	case TV_MSG_STOP_ANALYZE_TS: {
		//mpTv->Tv_Stop_Analyze_Ts();
		break;
	}

	case TV_MSG_START_ANALYZE_TS: {
		//mpTv->Tv_Start_Analyze_Ts ( pTv->mCurAnalyzeTsChannelID );
		break;
	}

	case TV_MSG_CHECK_FE_DELAY: {
		mpTv->mFrontDev.checkStatusOnce();
		break;
	}
	case TV_MSG_AV_EVENT: {
		mpTv->onEvent(*((CAv::AVEvent *)(msg.mpPara)));
		break;
	}
	case TV_MSG_FE_EVENT: {
		mpTv->onEvent(*((CFrontEnd::FEEvent *)(msg.mpPara)));
		break;
	}
	case TV_MSG_SCAN_EVENT: {
		mpTv->onEvent(*((CTvScanner::ScannerEvent *)(msg.mpPara)));
		break;
	}
	case TV_MSG_EPG_EVENT: {
		mpTv->onEvent(*((CTvEpg::EpgEvent *)(msg.mpPara)));
		break;
	}

	default:
		break;
	}
}


void CTv::CTvMsgQueue::onEvent ( const CTvScanner::ScannerEvent &ev )
{
	CMessage msg;
	msg.mDelayMs = 0;
	msg.mType = CTvMsgQueue::TV_MSG_SCAN_EVENT;
	memcpy(msg.mpPara, &ev, sizeof(ev));
	this->sendMsg ( msg );
}

void CTv::CTvMsgQueue::onEvent ( const CFrontEnd::FEEvent &ev )
{
	CMessage msg;
	msg.mDelayMs = 0;
	msg.mType = CTvMsgQueue::TV_MSG_FE_EVENT;
	memcpy(msg.mpPara, &ev, sizeof(ev));
	this->sendMsg ( msg );
}

void CTv::CTvMsgQueue::onEvent ( const CTvEpg::EpgEvent &ev )
{
	CMessage msg;
	msg.mDelayMs = 0;
	msg.mType = CTvMsgQueue::TV_MSG_EPG_EVENT;
	memcpy(msg.mpPara, &ev, sizeof(ev));
	this->sendMsg ( msg );
}


void CTv::CTvMsgQueue::onEvent(const CAv::AVEvent &ev)
{
	CMessage msg;
	msg.mDelayMs = 0;
	msg.mType = CTvMsgQueue::TV_MSG_AV_EVENT;
	memcpy(msg.mpPara, &ev, sizeof(ev));
	this->sendMsg ( msg );
}

int CTv::setTvObserver ( TvIObserver *ob )
{
	mpObserver = ob;
	return 0;
}

void CTv::sendTvEvent ( const CTvEv &ev )
{
	//AutoMutex lock(mLock);
	/* send sigstate to AutoBackLight */
	if (ev.getEvType() == CTvEv::TV_EVENT_SIGLE_DETECT) {
		TvEvent::SignalInfoEvent *pEv = (TvEvent::SignalInfoEvent *)(&ev);
		if (pEv->mStatus == TVIN_SIG_STATUS_STABLE) {
			mAutoBackLight.updateSigState(mAutoBackLight.SIG_STATE_STABLE);
		} else {
			mAutoBackLight.updateSigState(mAutoBackLight.SIG_STATE_NOSIG);
		}
	}

	LOGD ( "%s, tvaction=%x", __FUNCTION__, mTvAction);
	if((mTvAction & TV_ACTION_SCANNING) && (ev.getEvType() == CTvEv::TV_EVENT_SIGLE_DETECT)) {
		LOGD("%s, when scanning, not send sig detect event", __FUNCTION__);
		return;
	}
	if ( mpObserver != NULL ) {
		mpObserver->onTvEvent ( ev );
	}

	return;
}

int CTv::ClearAnalogFrontEnd()
{
	return mFrontDev.ClearAnalogFrontEnd ();
}

int CTv::dtvAutoScan()
{
	AutoMutex lock ( mLock );
	mTvAction = mTvAction | TV_ACTION_SCANNING;
	mTvEpg.leaveChannel();
	mAv.StopTS ();
	mAv.DisableVideoWithBlueColor();
	CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_DTV );
	CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_RADIO );
	CTvEvent::CleanAllEvent();
	mTvScanner.setObserver ( &mTvMsgQueue );
	mDtvScanRunningStatus = DTV_SCAN_RUNNING_NORMAL;
	mFrontDev.Open(FE_DTMB);
	mTvScanner.autoDtmbScan();//dtmb
	return 0;
}

int CTv::dtvCleanProgramByFreq ( int freq )
{
	int iOutRet = 0;
	CTvChannel channel;
	iOutRet = CTvChannel::SelectByFreq ( freq, channel );
	if ( -1 == iOutRet ) {
		LOGD ( "%s, Warnning: Current Freq not have program info in the ts_table\n", __FUNCTION__ );
	}

	iOutRet == CTvProgram::deleteChannelsProgram ( channel );
	return iOutRet;
}

int CTv::dtvManualScan (int beginFreq, int endFreq, int modulation)
{
	AutoMutex lock ( mLock );
	mTvAction = mTvAction | TV_ACTION_SCANNING;
	mTvEpg.leaveChannel();
	mAv.StopTS ();
	mAv.DisableVideoWithBlueColor();
	CTvChannel::DeleteBetweenFreq(beginFreq, endFreq);
	mTvScanner.setObserver ( &mTvMsgQueue );
	mDtvScanRunningStatus = DTV_SCAN_RUNNING_NORMAL;
	mFrontDev.Open(FE_DTMB);
	int iOutRet = mTvScanner.manualDtmbScan (beginFreq, endFreq, modulation); //dtmb
	return iOutRet;
}

int CTv::dtvAutoScanAtscLock(int attenna, int videoStd, int audioStd)
{
	//showboz
	/*  int minScanFreq, maxScanFreq, vStd, aStd;
	  vStd = CC_ATV_VIDEO_STD_PAL;
	  aStd = CC_ATV_AUDIO_STD_DK;
	  v4l2_std_id stdAndColor = mFrontDev.enumToStdAndColor(vStd, aStd);

	  AutoMutex lock ( mLock );
	  mTvAction = mTvAction | TV_ACTION_SCANNING;
	  mTvEpg.leaveChannel();
	  mAv.StopTS ();
	  mAv.DisableVideoWithBlueColor();
	  CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_DTV );
	  CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_RADIO );
	  CTvEvent::CleanAllEvent();
	  mTvScanner.setObserver ( &mTvMsgQueue );
	  mDtvScanRunningStatus = DTV_SCAN_RUNNING_NORMAL;
	  mFrontDev.Open(FE_ATSC);
	  mTvScanner.autoAtscScan(CTvScanner::AM_ATSC_ATTENNA_TYPE_AIR,  stdAndColor);//dtmb*/
	return 0;
}

//searchType 0:not 256 1:is 256 Program
int CTv::atvAutoScan(int videoStd, int audioStd, int searchType)
{
	int minScanFreq, maxScanFreq, vStd, aStd;
	AutoMutex lock ( mLock );
	mAv.DisableVideoWithBlueColor();
	mTvAction = mTvAction | TV_ACTION_SCANNING;
	mTvScanner.setObserver ( &mTvMsgQueue );
	mTvAudio.SetAudioMuteForTv ( CC_AUDIO_MUTE );
	getATVMinMaxFreq (&minScanFreq, &maxScanFreq );
	if ( minScanFreq == 0 || maxScanFreq == 0 || minScanFreq > maxScanFreq ) {
		LOGE ( "%s, auto scan  freq set is error min=%d, max=%d", __FUNCTION__, minScanFreq, maxScanFreq );
		return -1;
	}
	//if  set std null AUTO, use default PAL/DK
	//if(videoStd == CC_ATV_VIDEO_STD_AUTO) {
	//   vStd = CC_ATV_VIDEO_STD_PAL;
	//   aStd = CC_ATV_AUDIO_STD_DK;
	//} else {
	vStd = videoStd;
	aStd = audioStd;
	//}
	tvin_port_t source_port = mTvin.Tvin_GetSourcePortBySourceInput(SOURCE_TV);
	mTvin.VDIN_OpenPort ( source_port );
	LOGD("%s, atv auto scan vstd=%d, astd=%d ", __FUNCTION__,  vStd, aStd);
	v4l2_std_id stdAndColor = mFrontDev.enumToStdAndColor(vStd, aStd);

	int fmt = CFrontEnd::stdEnumToCvbsFmt (vStd, aStd);
	mTvin.AFE_SetCVBSStd ( ( tvin_sig_fmt_t ) fmt );
	m_cur_set_info_fmt = ( tvin_sig_fmt_t )fmt;

	if (searchType == 0) {
		CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_ATV );
	} else if (searchType == 1) { //type for skyworth, and insert 256 prog, and just update ts table
	}
	minScanFreq = mFrontDev.formatATVFreq ( minScanFreq );
	maxScanFreq = mFrontDev.formatATVFreq ( maxScanFreq );
	LOGD("%s, atv auto scan vstd=%d, astd=%d stdandcolor=%lld", __FUNCTION__, vStd, aStd, stdAndColor);
	mSigDetectThread.setVdinNoSigCheckKeepTimes(1000, false);
	mFrontDev.Open(FE_ANALOG);
	mTvScanner.autoAtvScan ( minScanFreq, maxScanFreq, stdAndColor, searchType );
	return 0;
}

int CTv::clearAllProgram(int arg0)
{
	CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_ATV );
	CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_TV );
	CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_DTV );
	CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_RADIO);
	return 0;
}

int CTv::atvMunualScan ( int startFreq, int endFreq, int videoStd, int audioStd, int store_Type , int channel_num )
{
	int minScanFreq, maxScanFreq,  vStd, aStd;

	minScanFreq = mFrontDev.formatATVFreq ( startFreq );
	maxScanFreq = mFrontDev.formatATVFreq ( endFreq );
	if ( minScanFreq == 0 || maxScanFreq == 0 ) {
		LOGE ( "%s, munual scan  freq set is error min=%d, max=%d", __FUNCTION__, minScanFreq, maxScanFreq );
		return -1;
	}

	//if  set std null AUTO, use default PAL/DK
	//if(videoStd == CC_ATV_VIDEO_STD_AUTO) {
	//    vStd = CC_ATV_VIDEO_STD_PAL;
	//    aStd = CC_ATV_AUDIO_STD_DK;
	// } else {
	vStd = videoStd;
	aStd = audioStd;
	// }

	AutoMutex lock ( mLock );
	mAv.DisableVideoWithBlueColor();
	mTvAction = mTvAction | TV_ACTION_SCANNING;
	mTvScanner.setObserver ( &mTvMsgQueue );

	mTvAudio.SetAudioMuteForTv ( CC_AUDIO_MUTE );
	v4l2_std_id stdAndColor = mFrontDev.enumToStdAndColor(vStd, aStd);


	tvin_port_t source_port = mTvin.Tvin_GetSourcePortBySourceInput(SOURCE_TV);
	mTvin.VDIN_OpenPort ( source_port );

	int fmt = CFrontEnd::stdEnumToCvbsFmt (vStd, aStd);
	mTvin.AFE_SetCVBSStd ( ( tvin_sig_fmt_t ) fmt );
	m_cur_set_info_fmt = ( tvin_sig_fmt_t )fmt;
	LOGD("%s, atv manual scan vstd=%d, astd=%d stdandcolor=%lld", __FUNCTION__, videoStd, audioStd, stdAndColor);
	mFrontDev.Open(FE_ANALOG);
	return mTvScanner.ATVManualScan ( minScanFreq, maxScanFreq, stdAndColor, store_Type, channel_num);
}

int CTv::getVideoFormatInfo ( int *pWidth, int *pHeight, int *pFPS, int *pInterlace )
{
	int  iOutRet = -1;
	AM_AV_VideoStatus_t video_status;

	do {
		if ( NULL == pWidth || NULL == pHeight || NULL == pFPS || NULL == pInterlace ) {
			break;
		}

		iOutRet = mAv.GetVideoStatus (&video_status );

		if ( AM_SUCCESS != iOutRet ) {
			LOGD ( "%s, ERROR: Cann't Get video format info\n", __FUNCTION__);
			break;
		}

		*pWidth = video_status.src_w;
		*pHeight = video_status.src_h;
		*pFPS = video_status.fps;
		*pInterlace = video_status.interlaced;
		//LOGD("%s, w : %d h : %d fps : %d  interlaced: %d\n", __FUNCTION__, video_status.src_w,video_status.src_h,video_status.fps,video_status.interlaced);
	} while ( false );

	return iOutRet;

}

int CTv::stopScanLock()
{
	AutoMutex lock ( mLock );
	return stopScan();
}

int CTv::stopScan()
{
	if(!(mTvAction & TV_ACTION_SCANNING)) {
		LOGD("%s, tv not scanning ,return\n", __FUNCTION__);
		return 0;
	}
	mSigDetectThread.requestAndWaitPauseDetect();
	mTvEpg.leaveChannel();
	mTvScanner.stopScan();
	mFrontDev.Close();
	mTvAction = mTvAction & ~TV_ACTION_SCANNING;
	return 0;
}

int CTv::playProgramLock ( int progId )
{
	/*   AutoMutex lock ( mLock );
	   CTvProgram prog;
	   int ret = CTvProgram::selectByID ( progId, prog );

	   if ( ret != 0 ) {
	       return -1;
	   }
	   int type = prog.getProgType();
	   LOGD ( "%s, blackout = %dprog type = %d id = %d name = %s\n", __FUNCTION__,   m_blackout_enable, type, progId, prog.getName().string() );
	   if(m_blackout_enable == 1){
	       mAv.EnableVideoBlackout();
	   }else if(m_blackout_enable == 0){
	       mAv.DisableVideoBlackout();
	   }

	   mTvAction = mTvAction | TV_ACTION_PLAYING;
	   m_cur_playing_prog_id = progId;
	   mFrontDev.Open(FE_ANALOG);
	   if ( m_source_input == SOURCE_TV && type == CTvProgram::TYPE_ATV ) {
	       playAtvProgram ( progId );
	   } else if ( m_source_input == SOURCE_DTV && ( type == CTvProgram::TYPE_DTV || type == CTvProgram::TYPE_RADIO ) ) {
	       playDtmbProgram ( progId );
	   } else {
	       LOGD ( "%s, source(%d) != type(%d),not play\n", __FUNCTION__,  m_source_input, type );
	   }
	   //just for ui,left/right/stereo
	   setAudioChannel((int)AM_AOUT_OUTPUT_STEREO);*/
	return 0;
}

int CTv::playDvbcProgram ( int progId )
{
	LOGD ( "%s, progId = %d\n", __FUNCTION__,  progId );

	CTvProgram prog;
	int ret = CTvProgram::selectByID ( progId, prog );

	if ( ret != 0 ) {
		return -1;
	}

	LOGD ( "%s, prog name = %s\n", __FUNCTION__,  prog.getName().string() );
	CTvChannel channel;
	prog.getChannel ( channel );

	mFrontDev.setPara ( channel.getMode(), channel.getFrequency(), channel.getSymbolRate(), channel.getModulation() );

	int vpid = 0x1fff, apid = 0x1fff, vfmt = -1, afmt = -1;

	CTvProgram::Video *pV = prog.getVideo();
	CTvProgram::Audio *pA;

	if ( pV != NULL ) {
		vpid = pV->getPID();
		vfmt = pV->getFormat();
	}

	int aindex = prog.getCurrentAudio ( String8 ( "eng" ) );

	if ( aindex >= 0 ) {
		pA = prog.getAudio ( aindex );

		if ( pA != NULL ) {
			apid = pA->getPID();
			afmt = pA->getFormat();
		}
	}
	mTvEpg.leaveProgram();
	mTvEpg.leaveChannel();
	startPlayTv ( SOURCE_DTV, vpid, apid, vfmt, afmt );

	mTvEpg.enterChannel ( channel.getID() );
	mTvEpg.enterProgram ( prog.getID() );
	return 0;
}

int CTv::getAudioTrackNum ( int progId )
{
	int iRet, iOutCnt = 0;
	CTvProgram prog;

	do {
		if ( 0 > progId ) {
			break;
		}

		iRet = CTvProgram::selectByID ( progId, prog );

		if ( 0 != iRet ) {
			break;
		}

		iOutCnt = prog.getAudioTrackSize();

	} while ( false );

	return iOutCnt;
}

int CTv::getAudioInfoByIndex ( int progId, int idx, int *pAFmt, String8 &lang )
{
	int iRet, iOutRet = -1;
	CTvProgram prog;
	CTvProgram::Audio *pA = NULL;

	do {
		if ( NULL == pAFmt || idx < 0 ) {
			break;
		}

		iRet = CTvProgram::selectByID ( progId, prog );

		if ( 0 != iRet ) {
			break;
		}

		if ( idx >= prog.getAudioTrackSize() ) {
			break;
		}

		pA = prog.getAudio ( idx );

		if ( NULL == pA ) {
			break;
		}

		*pAFmt = pA->getFormat();
		lang = pA->getLang();
	} while ( false );

	return iOutRet;
}

int CTv::switchAudioTrack ( int progId, int idx )
{
	int iOutRet = 0;
	CTvProgram prog;
	CTvProgram::Audio *pAudio = NULL;
	int aPid = -1;
	int aFmt = -1;

	do {
		if ( idx < 0 ) {
			break;
		}

		iOutRet = CTvProgram::selectByID ( progId, prog );

		if ( 0 != iOutRet ) {
			break;
		}
		if(prog.getAudioTrackSize() <= 1) {
			LOGD("%s, just one audio track, not to switch", __FUNCTION__);
			break;
		}
		if ( idx >= prog.getAudioTrackSize() ) {
			break;
		}

		pAudio = prog.getAudio ( idx );

		if ( NULL == pAudio ) {
			break;
		}

		aPid =  pAudio->getPID();
		aFmt = pAudio->getFormat();

		if ( aPid < 0 || aFmt < 0 ) {
			break;
		}

		iOutRet = mAv.SwitchTSAudio (( uint16_t ) aPid, ( AM_AV_AFormat_t ) aFmt );

		LOGD ( "%s, iOutRet = %d AM_AV_SwitchTSAudio\n", __FUNCTION__,  iOutRet );
	} while ( false );

	return iOutRet;
}

int CTv::ResetAudioDecoderForPCMOutput()
{
	int iOutRet = 0;

	iOutRet = mAv.ResetAudioDecoder ();
	LOGD ( "%s, iOutRet = %d AM_AV_ResetAudioDecoder\n", __FUNCTION__,  iOutRet );
	return iOutRet;
}

int CTv::playDtvProgram ( int mode, int freq, int para1, int para2, int vpid, int vfmt, int apid, int afmt, int pcr)
{

	mFrontDev.setPara ( mode, freq, para1, para2);

	startPlayTv ( SOURCE_DTV, vpid, apid, vfmt, afmt );

	return 0;
}


int CTv::playDtmbProgram ( int progId )
{
	CTvProgram prog;
	CTvChannel channel;
	int vpid = 0x1fff, apid = 0x1fff, vfmt = -1, afmt = -1;
	int aindex;
	CTvProgram::Audio *pA;
	CTvProgram::Video *pV;
	int ret = CTvProgram::selectByID ( progId, prog );

	saveDTVProgramID ( progId );
	prog.getChannel ( channel );
	//音量补偿
	int chanVolCompValue = 0;
	chanVolCompValue = GetAudioVolumeCompensationVal(progId);
	SetCurProgramAudioVolumeCompensationVal ( chanVolCompValue );

	mFrontDev.setPara ( channel.getMode(), channel.getFrequency(), channel.getBandwidth(), 0 );

	pV = prog.getVideo();

	if ( pV != NULL ) {
		vpid = pV->getPID();
		vfmt = pV->getFormat();
	}

	aindex = prog.getCurrAudioTrackIndex();

	if ( -1 == aindex ) { //db is default
		aindex = prog.getCurrentAudio ( String8 ( "eng" ) );

		if ( aindex >= 0 ) {
			prog.setCurrAudioTrackIndex ( progId, aindex );
		}
	}

	if ( aindex >= 0 ) {
		pA = prog.getAudio ( aindex );

		if ( pA != NULL ) {
			apid = pA->getPID();
			afmt = pA->getFormat();
		}
	}

	mTvEpg.leaveProgram();
	mTvEpg.leaveChannel();
	startPlayTv ( SOURCE_DTV, vpid, apid, vfmt, afmt );
	mTvEpg.enterChannel ( channel.getID() );
	mTvEpg.enterProgram ( prog.getID() );
	return 0;
}

int CTv::playAtvProgram (int  freq, int videoStd, int audioStd, int fineTune)
{
	//image selecting channel
	mSigDetectThread.requestAndWaitPauseDetect();
	mTvin.Tvin_StopDecoder();

	//set CVBS
	int fmt = CFrontEnd::stdEnumToCvbsFmt (videoStd, audioStd);
	mTvin.AFE_SetCVBSStd ( ( tvin_sig_fmt_t ) fmt );
	m_cur_set_info_fmt = ( tvin_sig_fmt_t )fmt;

	v4l2_std_id stdAndColor = mFrontDev.enumToStdAndColor (videoStd, audioStd);
	//set TUNER
	mFrontDev.setPara (FE_ANALOG, freq, stdAndColor, fineTune);

	if ( fineTune != 0 ) {
		mFrontDev.fineTune ( fineTune / 1000 );
	}

	mSigDetectThread.initSigState();
	mSigDetectThread.resumeDetect();
	return 0;
}

int CTv::resetFrontEndPara ( frontend_para_set_t feParms )
{
	if ( feParms.mode == FE_ANALOG ) {
		int progID = -1;
		int tmpFreq = feParms.freq;
		int tmpfineFreq = feParms.para2;
		int mode = feParms.mode;

		//get tunerStd from videoStd and audioStd
		v4l2_std_id stdAndColor = mFrontDev.enumToStdAndColor (feParms.videoStd, feParms.audioStd);

		//get program dbID saved by last time,if dbID is -1, it shows program hasn't played
		progID = getATVProgramID();

		LOGD("%s, resetFrontEndPara-　vstd=%d astd=%d stdandcolor=%lld", __FUNCTION__,  feParms.videoStd, feParms.audioStd, stdAndColor);

		//judge progID matching db's or not
		CTvProgram prog;

		if ( CTvProgram::selectByID ( progID, prog ) != 0 ) {
			LOGE ( "%s, atv progID is not matching the db's\n", __FUNCTION__);
			return 0;
		}

		//channel fine tune and save freq, std, fineFreq into DB
		CTvChannel channel;
		prog.upDateChannel ( channel, stdAndColor, tmpFreq, tmpfineFreq );
		mFrontDev.fineTune ( tmpfineFreq / 1000 ); //driver fine tune freq khz


		//set frontend parameters to tuner dev
		mSigDetectThread.requestAndWaitPauseDetect();
		mTvin.Tvin_StopDecoder();

		//set CVBS
		int fmt = CFrontEnd::stdEnumToCvbsFmt (feParms.videoStd, feParms.audioStd );
		mTvin.AFE_SetCVBSStd ( ( tvin_sig_fmt_t ) fmt );
		m_cur_set_info_fmt = ( tvin_sig_fmt_t )fmt;

		//set TUNER
		usleep(400 * 1000);
		mFrontDev.setPara ( FE_ANALOG, tmpFreq, stdAndColor, tmpfineFreq );
		usleep(400 * 1000);
		if ( tmpfineFreq != 0 ) {
			mFrontDev.fineTune ( tmpfineFreq / 1000 );
		}

		mSigDetectThread.initSigState();
		mSigDetectThread.resumeDetect();
	} else if ( feParms.mode == FE_DTMB ) {
		mFrontDev.setPara ( FE_DTMB, feParms.freq, feParms.para1, feParms.para2 );
	}

	return 0;
}

int CTv::resetDmxAndAvSource()
{
	AM_DMX_Source_t curdmxSource;
	mFrontDev.GetTSSource ( &curdmxSource );
	LOGD ( "%s, AM_FEND_GetTSSource %d", __FUNCTION__, curdmxSource );
	mTvDmx.Close();
	AM_DMX_OpenPara_t para;
	memset ( &para, 0, sizeof ( para ) );
	mTvDmx.Open (para);
	mTvDmx.SetSource(curdmxSource );
	AM_AV_TSSource_t ts_source = ( AM_AV_TSSource_t ) curdmxSource;
	mAv.SetTSSource (ts_source );
	return 0;
}

int CTv::SetCurProgramAudioVolumeCompensationVal ( int tmpVal )
{
	mTvAudio.SetAudioVolumeCompensationVal ( tmpVal );
	mTvAudio.SetAudioMasterVolume (mTvAudio.GetAudioMasterVolume() );

	LOGD ( "%s, VolumeCompensationVal = %d, id = %d\n", __FUNCTION__,  tmpVal, m_cur_playing_prog_id );
	CTvProgram prog;
	if ( CTvProgram::selectByID ( m_cur_playing_prog_id, prog ) != 0 ) {
		LOGE ( "%s, atv progID is not matching the db's  ret = 0\n", __FUNCTION__ );
		return -1;
	}

	if (prog.updateVolComp ( m_cur_playing_prog_id, tmpVal ) != 0 ) {
		LOGE ( "%s, atv progID is not matching the db's\n", __FUNCTION__);
		return -1;
	}
	return 0;
}

int CTv::GetAudioVolumeCompensationVal(int progxxId)
{
	int tmpVolValue = 0;
	CTvProgram prog;
	if ( CTvProgram::selectByID ( m_cur_playing_prog_id, prog ) != 0 ) {
		LOGE ( "%s, atv progID is not matching the db's  ret = 0\n", __FUNCTION__ );
		return tmpVolValue;
	}
	tmpVolValue = prog.getChanVolume ();
	LOGD ( "%s,progid = %d CompensationVal = %d\n", __FUNCTION__,  m_cur_playing_prog_id, tmpVolValue );
	if(tmpVolValue > 20 || tmpVolValue < -20) tmpVolValue = 0;
	return tmpVolValue;
}

int CTv::startPlayTv ( int source, int vid, int aid, int vfat, int afat )
{
	int ret = 0;

	if ( source == SOURCE_DTV ) {
		AM_FileEcho ( DEVICE_CLASS_TSYNC_AV_THRESHOLD_MIN, AV_THRESHOLD_MIN_MS );
		LOGD ( "%s, startPlayTv", __FUNCTION__);
		mAv.StartTS (vid, aid, ( AM_AV_VFormat_t ) vfat, ( AM_AV_AFormat_t ) afat );
	} else {

	}
	return ret;
}

int CTv::stopPlayingLock()
{
	AutoMutex lock ( mLock );
	return stopPlaying();
}

int CTv::stopPlaying()
{
	if(!(mTvAction & TV_ACTION_PLAYING)) {
		LOGD("%s, stopplay  cur  action = %x  not playing , return", __FUNCTION__, mTvAction);
		return 0;
	}
	mAv.EnableVideoBlackout();
	if(m_source_input  ==  SOURCE_TV) {
		//first mute
		mTvAudio.SetAudioMuteForTv(CC_AUDIO_MUTE);
		ClearAnalogFrontEnd();
	} else if(m_source_input  ==  SOURCE_DTV) {
		//mFrontDev.setPara(FE_DTMB, 51000000, 0, 0);
		mAv.StopTS ();
		mTvEpg.leaveChannel();
		mTvEpg.leaveProgram();
	}
	mAv.DisableVideoWithBlueColor();
	mTvAction = mTvAction & ~TV_ACTION_PLAYING;
	return 0;
}

int CTv::getAudioChannel()
{
	int iRet = -1;
	AM_AOUT_OutputMode_t audioChanneleMod;
	do {
		iRet = mAv.AudioGetOutputMode (&audioChanneleMod );
		if ( AM_SUCCESS != iRet ) {
			LOGD ( "%s, jianfei.lan GetOutputMode is FAILED %d\n", __FUNCTION__,  iRet );
			break;
		}
		LOGD ( "%s, jianfei.lan getAudioChannel iRet : %d audioChanneleMod %d\n", __FUNCTION__,  iRet, audioChanneleMod );
	} while ( 0 );
	return audioChanneleMod;
}

int  CTv::setAudioChannel ( int channelIdx )
{
	int iOutRet = 0;
	AM_AOUT_OutputMode_t audioChanneleMod;
	LOGD ( "%s, channelIdx : %d\n", __FUNCTION__,  channelIdx );
	audioChanneleMod = ( AM_AOUT_OutputMode_t ) channelIdx;
	iOutRet = mAv.AudioSetOutputMode (audioChanneleMod );
	if ( AM_SUCCESS != iOutRet) {
		LOGD ( "%s, jianfei.lan %s AM_AOUT_SetOutputMode device is FAILED %d\n", __FUNCTION__,  "TV", iOutRet );
	}
	return 0;
}

int CTv::getFrontendSignalStrength()
{
	int Strength = 0;
	Strength = mFrontDev.getStatus();
	return Strength;
}

int CTv::getFrontendSNR()
{
	int snr = 0;
	snr = mFrontDev.getSNR();
	return snr;
}

int CTv::getFrontendBER()
{
	int ber = 0;
	ber = mFrontDev.getBER();
	return ber;

}

int CTv::getChannelInfoBydbID ( int dbID, channel_info_t &chan_info )
{
	CTvProgram prog;
	CTvChannel channel;
	Vector<sp<CTvProgram> > out;
	memset ( &chan_info, sizeof ( chan_info ), 0 );
	chan_info.freq = 44250000;
	chan_info.uInfo.atvChanInfo.videoStd = CC_ATV_VIDEO_STD_PAL;
	chan_info.uInfo.atvChanInfo.audioStd = CC_ATV_AUDIO_STD_DK;

	int ret = CTvProgram::selectByID ( dbID, prog );
	if ( ret != 0 ) {
		LOGD ( "getChannelinfo , select dbid=%d,is not exist", dbID);
		return -1;
	}
	prog.getChannel ( channel );

	if ( CTvProgram::TYPE_ATV == prog.getProgType() ) {
		chan_info.freq = channel.getFrequency();
		chan_info.uInfo.atvChanInfo.finefreq  =  channel.getAfcData();
		LOGD("%s, get channel std = %lld", __FUNCTION__, channel.getStd());
		chan_info.uInfo.atvChanInfo.videoStd =
			( atv_video_std_t ) CFrontEnd::stdAndColorToVideoEnum ( channel.getStd() );
		chan_info.uInfo.atvChanInfo.audioStd =
			( atv_audio_std_t ) CFrontEnd::stdAndColorToAudioEnum ( channel.getStd() );
		chan_info.uInfo.atvChanInfo.isAutoStd =  ((channel.getStd()  &  V4L2_COLOR_STD_AUTO) ==  V4L2_COLOR_STD_AUTO) ? 1 : 0;
	} else if ( CTvProgram::TYPE_DTV == prog.getProgType()  || CTvProgram::TYPE_RADIO == prog.getProgType()) {
		chan_info.freq = channel.getFrequency();
		chan_info.uInfo.dtvChanInfo.strength = getFrontendSignalStrength();
		chan_info.uInfo.dtvChanInfo.quality = getFrontendSNR();
		chan_info.uInfo.dtvChanInfo.ber = getFrontendBER();
	}

	return  0;
}

bool CTv::Tv_Start_Analyze_Ts ( int channelID )
{
	int freq, ret;
	CTvChannel channel;

	AutoMutex lock ( mLock );
	mAv.StopTS ();
	mAv.DisableVideoWithBlueColor();
	ret = CTvChannel::selectByID ( channelID, channel );

	if ( ret != 0 ) {
		LOGD ( "%s, CTv tv_updatats can not get freq by channel ID", __FUNCTION__ );
		return false;
	}

	mTvAction = mTvAction | TV_ACTION_SCANNING;
	freq = channel.getFrequency();
	LOGD ( "%s, the freq = %d", __FUNCTION__,  freq );
	mDtvScanRunningStatus = DTV_SCAN_RUNNING_ANALYZE_CHANNEL;
	mTvScanner.setObserver ( &mTvMsgQueue );
	mTvScanner.manualDtmbScan ( freq, freq ); //dtmb
	return true;
}

bool CTv::Tv_Stop_Analyze_Ts()
{
	stopScanLock();
	return true;
}


int CTv::saveATVProgramID ( int dbID )
{
	config_set_int ( "TV", "atv.get.program.id", dbID );
	return 0;
}

int CTv::getATVProgramID ( void )
{
	return config_get_int ( "TV", "atv.get.program.id", -1 );
}

int CTv::saveDTVProgramID ( int dbID )
{
	config_set_int ( "TV", "dtv.get.program.id", dbID );
	return 0;;
}

int CTv::getDTVProgramID ( void )
{
	return config_get_int ( "TV", "dtv.get.program.id", -1 );
}

int CTv::getATVMinMaxFreq ( int *scanMinFreq, int *scanMaxFreq )
{
	int tmpVal, i = 0;
	const char *config_value;
	const char *strDelimit = ",";
	char *token = NULL;

	*scanMinFreq = 44250000;
	*scanMaxFreq = 868250000;

	config_value = config_get_str ( "ATV", "atv.get.min.max.freq", "null" );

	if ( strcmp ( config_value, "null" ) == 0 ) {
		LOGD ( "%s, atv.get.min.max.freq not set config\n", __FUNCTION__ );
		return -1;
	}

	char data_str[512];
	memset ( ( void * ) data_str, 0, sizeof ( data_str ) );
	strncpy ( data_str, config_value, sizeof ( data_str ) - 1 );

	token = strtok ( data_str, strDelimit );
	sscanf ( token, "%d", scanMinFreq );
	token = strtok ( NULL, strDelimit );

	if ( token != NULL ) {
		sscanf ( token, "%d", scanMaxFreq );
		return 0;
	}
	return 0;
}

int CTv::IsDVISignal()
{
	return ( ( TVIN_SIG_FMT_NULL != mSigDetectThread.getCurSigInfo().fmt ) && ( mSigDetectThread.getCurSigInfo().reserved & 0x1 ) );
}

int CTv::getHDMIFrameRate()
{
	int ConstRate[5] = {24, 25, 30, 50, 60};
	float ConstRateDiffHz[5] = {0.5, 0.5, 0.5, 2, 2}; //偏差值
	int fps = mSigDetectThread.getCurSigInfo().fps;
	int i = 0;
	for(; i < 5; i++) {
		if(abs(ConstRate[i] - fps) < ConstRateDiffHz[i])
			fps = ConstRate[i];
	}
	return fps;
}

tv_source_input_t CTv::GetLastSourceInput ( void )
{
	return m_last_source_input;
}

int CTv::isVgaFmtInHdmi ( void )
{
	tvin_sig_fmt_t fmt = mSigDetectThread.getCurSigInfo().fmt;

	if ( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) != SOURCE_TYPE_HDMI ) {
		return -1;
	}

	return CTvin::isVgaFmtInHdmi ( fmt );
}

int CTv::SetBypassModulesFor3D ( VIDEO_3D_MODE_E mode )
{
	LOGD ( "%s, ----> Set mode(%d) <----", __FUNCTION__, mode );
	const char *config_value = NULL;

	int DIBypassAllFlag = 0;
	config_value = config_get_str ( "TV", "tvin.di.bypass_all", "null" );//bypass , if
	if (strcasecmp(config_value, "true") == 0 || strcasecmp(config_value, "1") == 0) {
		DIBypassAllFlag = 1;
	} else {
		DIBypassAllFlag = 0;
	}


	switch (mode) {
	case VIDEO_3D_MODE_DISABLE: {
		mTvin.VDIN_SetDIBypassAll ( 0 );
		mTvin.VDIN_SetDIBypassPost ( 0 );
		//        mTvin.VDIN_SetDISip_Top_Bot ( 0 );
	}
	break;
	case VIDEO_3D_MODE_AUTO: {
		mTvin.VDIN_SetDIBypassAll ( 0 );
		mTvin.VDIN_SetDIBypassPost ( 1 );
		//        mTvin.VDIN_SetDISip_Top_Bot ( 0 );


	}
	break;
	case VIDEO_3D_MODE_LR:
	case VIDEO_3D_MODE_TB: {
		mTvin.VDIN_SetDIBypassPost ( 1 );
		usleep ( 20 * 1000 );
		mTvin.VDIN_SetDIBypassAll ( 1 | DIBypassAllFlag );
		//mTvin.VDIN_SetDISip_Top_Bot ( 1 );
	}
	break;
	case VIDEO_3D_MODE_LA:
	case VIDEO_3D_MODE_FA: {
		mTvin.VDIN_SetDIBypassPost ( 1 );
		usleep ( 20 * 1000 );
		mTvin.VDIN_SetDIBypassAll ( 0 );
		//mTvin.VDIN_SetDISip_Top_Bot ( 0 );
	}
	break;
	default:
		break;
	}

	if (CTvin::Tvin_SourceInputToSourceInputType(m_source_input) == SOURCE_TYPE_DTV ) {
		if ( mode == VIDEO_3D_MODE_DISABLE ) {
			mTvin.VDIN_SetVdinFlag ( MEMP_DCDR_WITHOUT_3D );
		} else {
			mTvin.VDIN_SetVdinFlag ( MEMP_DCDR_WITH_3D );
		}
	} else {
		if ( mode == VIDEO_3D_MODE_DISABLE ) {
			if ( m_source_input == SOURCE_TV ) {
				mTvin.VDIN_SetVdinFlag ( MEMP_ATV_WITHOUT_3D );
			} else {
				mTvin.VDIN_SetVdinFlag ( MEMP_VDIN_WITHOUT_3D );
			}
		} else {
			if ( m_source_input == SOURCE_TV ) {
				mTvin.VDIN_SetVdinFlag ( MEMP_ATV_WITH_3D );
			} else {
				mTvin.VDIN_SetVdinFlag ( MEMP_VDIN_WITH_3D );
			}
		}
	}

	return 0;
}

void CTv::print_version_info ( void )
{
	// print tvapi version info
	LOGD ( "libtvservice git branch:%s\n", tvservice_get_git_branch_info() );
	LOGD ( "libtvservice git version:%s\n",  tvservice_get_git_version_info() );
	LOGD ( "libtvservice Last Changed:%s\n", tvservice_get_last_chaned_time_info() );
	LOGD ( "libtvservice Last Build:%s\n",  tvservice_get_build_time_info() );
	LOGD ( "libtvservice Builer Name:%s\n", tvservice_get_build_name_info() );
	LOGD ( "libtvservice board version:%s\n", tvservice_get_board_version_info() );
	LOGD ( "\n\n");
	// print dvb version info
	LOGD ( "libdvb git branch:%s\n", dvb_get_git_branch_info() );
	LOGD ( "libdvb git version:%s\n", dvb_get_git_version_info() );
	LOGD ( "libdvb Last Changed:%s\n", dvb_get_last_chaned_time_info() );
	LOGD ( "libdvb Last Build:%s\n", dvb_get_build_time_info() );
	LOGD ( "libdvb Builer Name:%s\n", dvb_get_build_name_info() );
}

int CTv::Tvin_GetTvinConfig ( void )
{
	const char *config_value;
	int i = 0;

	config_value = config_get_str ( "TV", "tvin.overscan.3d", "null" );

	if ( strcmp ( config_value, "enable" ) == 0 ) {
		gTvinConfig.overscan_3d = 1;
	} else {
		gTvinConfig.overscan_3d = 0;
	}

	config_value = config_get_str ( "TV", "tvin.tv.out.counter", "null" );
	gTvinConfig.tv_out_counter = ( unsigned int ) strtol ( config_value, NULL, 6 );

	if ( gTvinConfig.tv_out_counter <= 0 || gTvinConfig.tv_out_counter > 8 ) {
		gTvinConfig.tv_out_counter = 6;
	}

	config_value = config_get_str ( "TV", "tvin.kernelpet_disable", "null" );

	if ( strcmp ( config_value, "disable" ) == 0 ) {
		gTvinConfig.kernelpet_disable = true;
	} else {
		gTvinConfig.kernelpet_disable = false;
	}

	config_value = config_get_str ( "TV", "tvin.kernelpet.timeout", "null" );
	gTvinConfig.userpet_timeout = ( unsigned int ) strtol ( config_value, NULL, 10 );

	if ( gTvinConfig.kernelpet_timeout <= 0 || gTvinConfig.kernelpet_timeout > 40 ) {
		gTvinConfig.kernelpet_timeout = 10;
	}


	config_value = config_get_str ( "TV", "tvin.userpet", "null" );

	if ( strcmp ( config_value, "enable" ) == 0 ) {
		gTvinConfig.userpet = true;
	} else {
		gTvinConfig.userpet = false;
	}

	config_value = config_get_str ( "TV", "tvin.userpet.timeout", "null" );
	gTvinConfig.userpet_timeout = ( unsigned int ) strtol ( config_value, NULL, 10 );

	if ( gTvinConfig.userpet_timeout <= 0 || gTvinConfig.userpet_timeout > 100 ) {
		gTvinConfig.userpet_timeout = 10;
	}

	config_value = config_get_str ( "TV", "tvin.userpet.reset", "null" );

	if ( strcmp ( config_value, "disable" ) == 0 ) {
		gTvinConfig.userpet_reset = 0;
	} else {
		gTvinConfig.userpet_reset = 1;
	}

	config_value = config_get_str ( "TV", "tvin.title_shake", "null" );

	if ( strcmp ( config_value, "enable" ) == 0 ) {
		gTvinConfig.title_shake = true;
	} else {
		gTvinConfig.title_shake = false;
	}

	return 0;
}

int CTv::SetAudioMainVolLutBufName()
{
	int i = 0;
	const char *tmp_ptr = NULL;
	int get_type_buf[6] = { CC_GET_LUT_TV, CC_GET_LUT_AV, CC_GET_LUT_COMP, CC_GET_LUT_HDMI, CC_GET_LUT_VGA, CC_GET_LUT_MPEG };
	const char *end_str_buf[6] = { ".tv", ".av", ".comp", ".hdmi", ".vga", ".mpeg", };
	project_info_t tmp_info;
	char tmp_buf[128];
	char value_buf[128];
	char key_buf[128];

	int name_en = GetAudioMainVolLutBufNameModifyEnableCFG();

	if (name_en == 1) {
		strcpy(tmp_buf, "null");
		if (GetProjectInfo(&tmp_info) == 0) {
			strncpy(tmp_buf, tmp_info.amp_curve_name, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
		}

		if (strcmp(tmp_buf, "null") != 0) {
			for ( i = 0; i < 6; i++ ) {
				memset(value_buf, '\0', sizeof(value_buf));
				memset(key_buf, '\0', sizeof(key_buf));

				GetAudioMainVolLutBufNameCFG(get_type_buf[i], key_buf);

				strcpy(value_buf, tmp_buf);
				strcat(value_buf, end_str_buf[i]);

				tmp_ptr = config_get_str("TV", key_buf, "null");
				if (strcmp(tmp_ptr, value_buf) != 0) {
					config_set_str("TV", key_buf, value_buf);
					LOGD("%s, config string now set as: %s \n", __FUNCTION__, value_buf);
				} else {
					LOGD("%s, config string has been set as: %s \n", __FUNCTION__, value_buf);
				}
			}
		}
	}

	return 0;
}

int CTv::SetAudioSupperBassVolLutBufName()
{
	int i = 0;
	const char *tmp_ptr = NULL;
	int get_type_buf[6] = { CC_GET_LUT_TV, CC_GET_LUT_AV, CC_GET_LUT_COMP, CC_GET_LUT_HDMI, CC_GET_LUT_VGA, CC_GET_LUT_MPEG };
	const char *end_str_buf[6] = { ".tv", ".av", ".comp", ".hdmi", ".vga", ".mpeg", };
	project_info_t tmp_info;
	char tmp_buf[128];
	char value_buf[128];
	char key_buf[128];

	int name_en = GetAudioSupperBassVolLutBufNameModifyEnableCFG();

	if (name_en == 1) {
		strcpy(tmp_buf, "null");
		if (GetProjectInfo(&tmp_info) == 0) {
			strncpy(tmp_buf, tmp_info.amp_curve_name, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
		}

		if (strcmp(tmp_buf, "null") != 0) {
			for ( i = 0; i < 6; i++ ) {
				memset(value_buf, '\0', sizeof(value_buf));
				memset(key_buf, '\0', sizeof(key_buf));

				GetAudioSupperBassVolLutBufNameCFG(get_type_buf[i], key_buf);

				strcpy(value_buf, tmp_buf);
				strcat(value_buf, ".sup");
				strcat(value_buf, end_str_buf[i]);

				tmp_ptr = config_get_str("TV", key_buf, "null");
				if (strcmp(tmp_ptr, value_buf) != 0) {
					config_set_str("TV", key_buf, value_buf);
					LOGD("%s, config string now set as: %s \n", __FUNCTION__, value_buf);
				} else {
					LOGD("%s, config string has been set as: %s \n", __FUNCTION__, value_buf);
				}
			}
		}
	}

	return 0;
}

TvRunStatus_t CTv::GetTvStatus()
{
	return mTvStatus;
}

int CTv::OpenTv ( void )
{
	int tmp_val = 0;
	const char *value;
	//reboot system by fbc setting.
	value = config_get_str ( "TV", "fbc.get.panelinfo", "null" );
	LOGD("%s fbc-------------------\n", value);
	if ( strcmp ( value, "edid" ) == 0 ) {
		if(reboot_sys_by_fbc_edid_info() == 0) {
		}
	} else if ( strcmp ( value, "uart" ) == 0 ) {
		if(reboot_sys_by_fbc_uart_panel_info() == 0) {
		}
	}

	LOGD ( "%s, tv open\n", __FUNCTION__);
	char prop_value[PROPERTY_VALUE_MAX];
	memset ( prop_value, '\0', PROPERTY_VALUE_MAX );

	mTvin.Tvin_LoadSourceInputToPortMap();

	SSMSave3DLRSwitch ( 0 );
	SSMSave3DTO2D ( 0 );
	//
	SSMHandlePreCopying();
	if ( SSMDeviceMarkCheck() < 0 ) {
		SSMRestoreDeviceMarkValues();
		Tv_SSMRestoreDefaultSetting();
	}
	mTvin.OpenTvin();
	mTvin.init_vdin();
	mTvin.Tv_init_afe();
	mVpp.Vpp_Init();
	SetAudioSupperBassVolLutBufName();
	SetAudioMainVolLutBufName();
	mTvAudio.AudioCtlInitializeLoad();

	SSMSetHDCPKey();
	system ( "/system/bin/dec" );

	//set color filter
	//    SetFileAttrValue ( "/sys/module/amvideo/parameters/filt_mode", "0x2100" );

	if ( gTvinConfig.title_shake) {
		SetFileAttrValue ( "/sys/module/di/parameters/di_new_mode_mask", "0x10000" );
	}

	value = config_get_str ( "TV", "tvin.autoset.displayfreq", "null" );

	if ( strcmp ( value, "enable" ) == 0 ) {
		m_autoset_displayfreq = true;
	} else {
		m_autoset_displayfreq = false;
	}

	if (mTvin.VDIN_GetDisplayVFreq() == 30) {
		m_autoset_displayfreq = false;
	}

	if (GetPlatformHaveDDFlag() == 1) {
		SetFileAttrValue ("/sys/module/tvin_hdmirx/parameters/edid_mode", "0x109");
	}

	value = config_get_str ( "TV", "ssm.handle.hdmi.edid.en", "null" );

	if ( strtoul(value, NULL, 10) == 1 ) {
		LOGD( "%s, get config \"%s\" is \"%s\".\n",
			  __FUNCTION__, "ssm.handle.hdmi.edid.en", value );
		//get hdmi edid use mode
		char prop_value[256];
		memset( prop_value, '\0', 256 );
		property_get( "ubootenv.var.outputmode", prop_value, "null" );
		LOGD( "%s, get property \"%s\" is \"%s\".\n",
			  __FUNCTION__, "ubootenv.var.outputmode", prop_value );
		if ( strcmp ( prop_value, "null" ) != 0 ) {
			config_set_str ( "TV", "ssm.handle.hdmi.edid.use", prop_value );
			int i;
			char edid_path[256];
			char edid_path_cfg[256];
			//set file's path for hdmi edid of each port
			for ( i = 1; i <= SSM_HDMI_PORT_MAX; i++ ) {
				memset( edid_path, '\0', 256 );
				memset( edid_path_cfg, '\0', 256 );
				sprintf ( edid_path, "/system/etc/%s_port%d.bin", prop_value, i );
				sprintf ( edid_path_cfg, "ssm.handle.hdmi.port%d.edid.file.path", i );
				config_set_str ( "TV", edid_path_cfg, edid_path );
			}
		} else {
			//set default hdmi edid
			config_set_str ( "TV", "ssm.handle.hdmi.edid.use", "hdmi_edid" );
			int i;
			char edid_path[256];
			char edid_path_cfg[256];
			//set file's path for hdmi edid of each port
			for ( i = 1; i <= SSM_HDMI_PORT_MAX; i++ ) {
				memset( edid_path, '\0', 256 );
				memset( edid_path_cfg, '\0', 256 );
				sprintf ( edid_path, "/system/etc/%s_port%d.bin", "hdmi_edid", i );
				sprintf ( edid_path_cfg, "ssm.handle.hdmi.port%d.edid.file.path", i );
				config_set_str ( "TV", edid_path_cfg, edid_path );
			}
		}
		m_is_set_hdmi_edid = true;
	}
	//
	unsigned char val = 0;
	Tvin_GetTvinConfig();
	m_last_source_input = SOURCE_INVALID;
	m_source_input = SOURCE_INVALID;
	m_mode_3d = VIDEO_3D_MODE_DISABLE;
	m_is_hdmi_sr_detect_start = false;
	m_hdmi_sampling_rate = 0;
	int8_t blackout_enable;
	SSMReadBlackoutEnable(&blackout_enable);
	m_blackout_enable = blackout_enable;


	//
	//dtv init
	int rt = -1;
	//opendemux();
	rt = mFrontDev.Open(FE_ANALOG);
	mFrontDev.autoLoadFE();
	AM_DMX_OpenPara_t para_dmx;
	memset ( &para_dmx, 0, sizeof ( AM_DMX_OpenPara_t ) );
	mTvDmx.Open(para_dmx );
	mTvDmx.SetSource (AM_DMX_SRC_TS2);
	mAv.Open();
	mTvEpg.Init ( 0, 0, 1, "eng zho chi", "GB2312" );
	resetDmxAndAvSource();
	mSourceConnectDetectThread.startDetect();

	if (SSMReadSerialCMDSwitchValue() == 1) {
		SetSerialSwitch(SERIAL_A, 1);
	}
	mTvin.VDIN_SetVdinFlag ( MEMP_DCDR_WITHOUT_3D );
	mTvin.VDIN_SetMVCViewMode(0);//for not 3d screen
	ClearAnalogFrontEnd();

	mTvStatus = TV_OPEN_ED;

	mHDMIRxCEC.start();

	return 0;
}

int CTv::CloseTv ( void )
{
	LOGD ( "tv close");
	mSigDetectThread.stopDetect();
	if (mpUpgradeFBC != NULL) {
		mpUpgradeFBC->stop();
	}
	mTvin.Tv_uninit_afe();
	mTvin.uninit_vdin();
	mVpp.Vpp_Uninit();
	TvMisc_DisableWDT ( gTvinConfig.userpet );
	mTvStatus = TV_CLOSE_ED;
	return 0;
}


int CTv::StartTvLock ()
{
	LOGD ( "%s, tv start status = %d", __FUNCTION__, mTvStatus);
	if(mTvStatus == TV_START_ED)
		return 0;

	Mutex::Autolock _l ( mLock );
	mTvAction = mTvAction | TV_ACTION_STARTING;
	mTvAudio.SwitchAVOutBypass(0);
	mTvAudio.AudioCtlInit();
	mFrontDev.Open(FE_ANALOG);
	mSigDetectThread.startDetect();
	mSigDetectThread.pauseDetect();
	mSigDetectThread.initSigState();
	mTvMsgQueue.startMsgQueue();
	resetDmxAndAvSource();
	Vpp_SetDisplayMode ( mVpp.Tv_GetDisplayMode ( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) ), CTvin::Tvin_SourceInputToSourceInputType(m_source_input), mSigDetectThread.getCurSigInfo().fmt);
	TvMisc_EnableWDT ( gTvinConfig.kernelpet_disable, gTvinConfig.userpet, gTvinConfig.kernelpet_timeout, gTvinConfig.userpet_timeout, gTvinConfig.userpet_reset );
	am_phase_t am_phase;
	if (mVpp.getPqData()->PQ_GetPhaseArray ( &am_phase ) == 0 ) {
		mTvin.TvinApi_SetCompPhase(am_phase);
	}
	mTvin.TvinApi_SetCompPhaseEnable ( 1 );
	mTvin.VDIN_EnableRDMA ( 1 );
	mTvin.VDIN_SetDIBypasshd ( 0 );

	SetBypassModulesFor3D ( VIDEO_3D_MODE_DISABLE );
	//win pos
	tvin_window_pos_t win_pos;
	Vpp_GetDisplayResolutionInfo(&win_pos);
	SetPreviewWindow ( win_pos );

	mTvin.Tvin_RemovePath ( TV_PATH_TYPE_TVIN );
	if ( mTvin.Tvin_RemovePath ( TV_PATH_TYPE_DEFAULT ) ) {
		mTvin.Tvin_AddPath ( TV_PATH_DECODER_NEW3D_WITHOUTPPMGR_AMVIDEO );
	}
	mTvin.VDIN_SetVdinFlag ( MEMP_DCDR_WITHOUT_3D );

	mAv.SetVideoWindow (0, 0, 0, 0 );

	mTvAction = mTvAction & ~TV_ACTION_STARTING;
	mTvStatus = TV_START_ED;
	return 0;
}
int CTv::StopTvLock ( void )
{
	LOGD("%s, call Tv_Stop status = %d \n", __FUNCTION__, mTvStatus);
	const char *value;
	Mutex::Autolock _l ( mLock );
	//we should stop audio first for audio mute.
	mTvAction = mTvAction | TV_ACTION_STOPING;
	mSigDetectThread.requestAndWaitPauseDetect();
	mAv.EnableVideoBlackout();
	//stop scan  if scanning
	if(mTvAction & TV_ACTION_SCANNING) {
		LOGD("%s, scanning first stop it\n", __FUNCTION__)
		mTvEpg.leaveChannel();
		mTvScanner.stopScan();
		mTvAction = mTvAction & ~TV_ACTION_SCANNING;
	}
	//first stop play(if playing)
	stopPlaying();
	//
	mTvAudio.SwitchAVOutBypass(0);
	tv_audio_channel_e audio_channel = mTvin.Tvin_GetInputSourceAudioChannelIndex (SOURCE_MPEG);
	mTvAudio.AudioLineInSelectChannel( audio_channel );
	mTvAudio.AudioCtlUninit();
	mTvin.Tvin_StopDecoder();
	mTvin.Tvin_RemovePath ( TV_PATH_TYPE_TVIN );
	Tv_Set3DMode(VIDEO_3D_MODE_DISABLE);
	mTvAudio.SetAudioMuteForTv ( CC_AUDIO_UNMUTE );
	tvin_window_pos_t win_pos;
	Vpp_GetDisplayResolutionInfo(&win_pos);
	SetPreviewWindow (win_pos );
	mVpp.Tv_LoadVppSettings ( SOURCE_TYPE_MPEG, TVIN_SIG_FMT_NULL, INDEX_2D, TVIN_TFMT_2D );
	Vpp_SetDisplayMode ( mVpp.Tv_GetDisplayMode ( CTvin::Tvin_SourceInputToSourceInputType(SOURCE_MPEG) ), CTvin::Tvin_SourceInputToSourceInputType(SOURCE_MPEG), mSigDetectThread.getCurSigInfo().fmt);
	RefreshAudioMasterVolume ( SOURCE_MPEG );
	mVpp.VPP_SetVideoCrop ( 0, 0, 0, 0 );
	m_last_source_input = SOURCE_INVALID;
	m_source_input = SOURCE_INVALID;
	mAv.DisableVideoWithBlackColor();
	mAv.ClearVideoBuffer();
	mFrontDev.Close();
	mTvAction = mTvAction & ~TV_ACTION_STOPING;
	mTvStatus = TV_STOP_ED;
	value = config_get_str ( "TV", "tvin.2d4G.headset.en", "null" );
	if ( strcmp ( value, "enable" ) == 0 ) {
		property_set("audio.tv_open.flg", "0");
	}

	Tv_SetDDDRCMode(SOURCE_MPEG);
	return 0;
}

int CTv::Tv_MiscSetBySource ( tv_source_input_t source_input )
{
	int ret = -1;

	switch ( source_input ) {
	case SOURCE_TV:
		ret = SetFileAttrValue ( "/sys/module/am_vecm/parameters/dnlp_adj_level", "4" );
		break;

	case SOURCE_HDMI1:
	case SOURCE_HDMI2:
	case SOURCE_HDMI3:
		//ret = mVpp.Tv_SavePanoramaMode ( VPP_PANORAMA_MODE_FULL, SOURCE_TYPE_HDMI );
		ret |= SetFileAttrValue ( "/sys/module/am_vecm/parameters/dnlp_adj_level", "5" );
		break;

	case SOURCE_DTV:
		if ( m_autoset_displayfreq) {
			ret = mTvin.VDIN_SetDisplayVFreq ( 50, mAv.getVideoDisplayResolution() , mIsFbc);
		}

	case SOURCE_AV1:
	case SOURCE_AV2:
	case SOURCE_VGA:
		ret |= SetFileAttrValue ( "/sys/module/am_vecm/parameters/dnlp_adj_level", "5" );
		break;

	case SOURCE_SVIDEO:
	case SOURCE_IPTV:
	default:
		ret |= SetFileAttrValue ( "/sys/module/am_vecm/parameters/dnlp_adj_level", "5" );
		break;
	}

	return ret;
}

int CTv::SetSourceSwitchInput (tv_source_input_t source_input )
{
	Mutex::Autolock _l ( mLock );
	LOGD ( "%s, source input = %d", __FUNCTION__, source_input );
	tv_source_input_t cur_source_input = m_source_input;
	tvin_port_t cur_port;

	Tv_SetDDDRCMode(source_input);

	if (source_input == cur_source_input ) {
		LOGW ( "%s,same input change display mode", __FUNCTION__ );
		return 0;
	}

	//if HDMI, need to set EDID of each port
	if (m_is_set_hdmi_edid) {
		int tmp_ret = 0;
		switch ( source_input ) {
		case SOURCE_HDMI1:
			tmp_ret = SSMSetHDMIEdid(1);
			break;
		case SOURCE_HDMI2:
			tmp_ret = SSMSetHDMIEdid(2);
			break;
		case SOURCE_HDMI3:
			tmp_ret = SSMSetHDMIEdid(3);
			break;
		default:
			tmp_ret = -1;
			break;
		}
		if (tmp_ret < 0)
			LOGE ( "%s, do not set hdmi port%d edid.ret=%d", __FUNCTION__, source_input - 4, tmp_ret );
	}
	mTvAction = mTvAction | TV_ACTION_SOURCE_SWITCHING;
	m_cur_playing_prog_id = -1;
	//
	CMessage msg;
	msg.mType = CTvMsgQueue::TV_MSG_CHECK_FE_DELAY;
	msg.mpData = this;
	msg.mDelayMs = 3500;//ms
	mTvMsgQueue.removeMsg ( msg );
	//mute first
	mTvAudio.SetAudioMuteForTv(CC_AUDIO_MUTE);
	mSigDetectThread.requestAndWaitPauseDetect();
	mAv.DisableVideoWithBlueColor();
	//enable blackout, when play,disable it
	mAv.EnableVideoBlackout();
	//set front dev mode
	if ( source_input == SOURCE_TV ) {
		mFrontDev.Close();
		mFrontDev.Open(FE_ANALOG);
		//mFrontDev.setMode ( FE_ANALOG );
	} else if ( source_input == SOURCE_DTV ) {
		mFrontDev.Close();
		mFrontDev.Open(FE_ANALOG);
		//mFrontDev.setMode (  FE_DTMB);
	} else {
		mFrontDev.Close();
	}

	//ok
	m_last_source_input = m_source_input;
	m_source_input = source_input;
	SSMSaveSourceInput ( source_input );

	tv_source_input_t pre_source_input = cur_source_input;//change
	cur_source_input = source_input;

	mTvAudio.SetAudioVolumeCompensationVal ( 0 );

	if ( source_input == SOURCE_DTV ) {
		//we should stop audio first for audio mute.
		mTvAudio.SwitchAVOutBypass(0);
		tv_audio_channel_e audio_channel = mTvin.Tvin_GetInputSourceAudioChannelIndex (SOURCE_MPEG);
		mTvAudio.AudioLineInSelectChannel( audio_channel );
		mTvAudio.AudioCtlUninit();
		//
		mTvin.Tvin_StopDecoder();
		mTvin.Tvin_RemovePath ( TV_PATH_TYPE_TVIN );
		Tv_Set3DMode(VIDEO_3D_MODE_DISABLE);
		//double confirm we set the main volume lut buffer to mpeg
		RefreshAudioMasterVolume ( SOURCE_MPEG );
		mTvAudio.AudioDataInitForDtv();
		mTvin.setMpeg2Vdin(1);
		mVpp.Tv_LoadVppSettings ( SOURCE_TYPE_DTV, TVIN_SIG_FMT_NULL, INDEX_2D, TVIN_TFMT_2D );
	} else {
		mTvin.setMpeg2Vdin(0);
		mTvin.Tvin_AddPath ( TV_PATH_VDIN_NEW3D_WITHOUTPPMGR_AMVIDEO );
		mTvin.VDIN_SetVdinFlag ( MEMP_VDIN_WITHOUT_3D );
	}
	cur_port = mTvin.Tvin_GetSourcePortBySourceInput ( cur_source_input );
	Tv_MiscSetBySource ( source_input );

	//only DTV->TVIN or TVIN->DTV need start resource detect thread
	if ( ( pre_source_input == SOURCE_DTV || cur_source_input == SOURCE_DTV ) && ( cur_source_input != pre_source_input ) ) {
		LOGD ( "%s, dtv  need  resource  detect\n", __FUNCTION__ );
		int isNeedCheckD2D3 = ( gTvinConfig.memory512m) ? 1 : 0;
		static int sigchkcnt = 0;
		while ( true ) {
			if ( ( mTvin.Tvin_CheckPathActive ( TV_PATH_TYPE_DEFAULT, isNeedCheckD2D3 ) == TV_PATH_STATUS_INACTIVE )
					&& ( mTvin.Tvin_CheckPathActive ( TV_PATH_TYPE_TVIN, isNeedCheckD2D3 ) == TV_PATH_STATUS_INACTIVE ) ) {
				LOGD("%s, path check  is  ok, and break\n", __FUNCTION__)
				break;
			} else {
				if(sigchkcnt++ > 20) {
					sigchkcnt = 0;
					break;
				}

				LOGW ( "%s,resource is busy.", __FUNCTION__);
			}
		}//while
	}//if    need check,and check return

	if ( source_input != SOURCE_DTV ) {
		// Uninit data
		mTvAudio.AudioDataUnInit();
		if (source_input == SOURCE_HDMI1 || source_input == SOURCE_HDMI2 || source_input == SOURCE_HDMI3 ||
				source_input == SOURCE_MPEG || source_input == SOURCE_DTV ) {
			mTvAudio.SwitchAVOutBypass(0);
		} else {
			mTvAudio.SwitchAVOutBypass(1);
		}

		tv_audio_channel_e audio_channel = mTvin.Tvin_GetInputSourceAudioChannelIndex (source_input);
		mTvAudio.AudioLineInSelectChannel( audio_channel );

		Tv_SetAudioInSource ( source_input );
		if ( source_input == SOURCE_HDMI1 || source_input == SOURCE_HDMI2 || source_input == SOURCE_HDMI3 ) {
			m_is_hdmi_sr_detect_start = true;
			m_hdmi_sampling_rate = 0;
		} else {
			mTvAudio.AudioDataInit ( 48000, true );
			audio_sr = 48000;
		}
		//===========================================
		if ( mTvin.SwitchPort ( cur_port )  == 0 ) { //ok
			//==========================================
			SSMSave3DDepth ( 0 );
			mTvin.Tvin_SetDepthOf2Dto3D ( 0 ); // set default depth
			Tv_Set3DMode(VIDEO_3D_MODE_DISABLE);
			SSMSave3DLRSwitch ( 0 );
			SSMSave3DTO2D (0);

			//for HDMI source connect detect
			mTvin.VDIN_OpenHDMIPinMuxOn(true);
			mTvAudio.AudioSetRecordDataZero ( false ); //make AudioRecord data avaliable
			mVpp.Vpp_ResetLastVppSettingsSourceType();
			//==========================================
			m_sig_stable_nums = 0;
			mSigDetectThread.initSigState();
			if(source_input == SOURCE_HDMI1 || source_input == SOURCE_HDMI2 || source_input == SOURCE_HDMI3) {
				mSigDetectThread.setVdinNoSigCheckKeepTimes(5000, true);
			} else {
				mSigDetectThread.setVdinNoSigCheckKeepTimes(3000, true);
			}
			mSigDetectThread.resumeDetect();
		} else {
		}
	}

	Tv_SetAudioSourceType(source_input);
	RefreshAudioMasterVolume(source_input);
	Tv_SetAudioOutputSwap_Type(source_input);

	mTvAction = mTvAction & ~ TV_ACTION_SOURCE_SWITCHING;
	return 0;
}

void CTv::onSigToStable()
{
	if ( ( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) == SOURCE_TYPE_TV ) ) {
		mSigDetectThread.getCurSigInfo().fmt = m_cur_set_info_fmt;
	}
	if ( m_autoset_displayfreq) {
		if ( CTvin::Tvin_is50HzFrameRateFmt ( mSigDetectThread.getCurSigInfo().fmt ) ) {
			mTvin.VDIN_SetDisplayVFreq ( 50, mAv.getVideoDisplayResolution(), mIsFbc );
			LOGD ( "%s, SetDisplayVFreq 50HZ.", __FUNCTION__);
		} else {
			mTvin.VDIN_SetDisplayVFreq ( 60, mAv.getVideoDisplayResolution(), mIsFbc );
			LOGD ( "%s, SetDisplayVFreq 60HZ.", __FUNCTION__);
		}
	}
	//showbo mark  hdmi auto 3d, tran fmt  is 3d, so switch to 3d
	LOGD("hdmi trans_fmt = %d", mSigDetectThread.getCurSigInfo().trans_fmt);
	if (mSigDetectThread.getCurSigInfo().trans_fmt != TVIN_TFMT_2D) {
		Tv_Set3DMode(VIDEO_3D_MODE_AUTO);
	}

	LOGD("onSigToStable --- trans_fmt = %d", mSigDetectThread.getCurSigInfo().trans_fmt);
	//load pq parameters
	mVpp.Tv_LoadVppSettings (CTvin::Tvin_SourceInputToSourceInputType(m_source_input), mSigDetectThread.getCurSigInfo().fmt, Check2Dor3D(m_mode_3d, mSigDetectThread.getCurSigInfo().trans_fmt ), mSigDetectThread.getCurSigInfo().trans_fmt );

	if ( m_win_mode == PREVIEW_WONDOW ) {
		mAv.setVideoAxis(m_win_pos.x1, m_win_pos.y1, m_win_pos.x2, m_win_pos.y2);
		mAv.setVideoScreenMode ( CAv::VIDEO_WIDEOPTION_FULL_STRETCH );
	} else {
		Vpp_SetDisplayMode ( mVpp.Tv_GetDisplayMode ( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) ), CTvin::Tvin_SourceInputToSourceInputType(m_source_input), mSigDetectThread.getCurSigInfo().fmt);
	}
	m_sig_stable_nums = 0;

}
void CTv::onSigStableToUnstable()
{
	LOGD ( "%s, stable to unstable\n", __FUNCTION__);
	mTvAudio.SetAudioMuteForTv(CC_AUDIO_MUTE);
	mAv.DisableVideoWithBlackColor();
	mTvin.Tvin_StopDecoder();
}
void CTv::onSigStableToUnSupport()
{
	mTvAudio.SetAudioMuteForTv(CC_AUDIO_MUTE);
	mAv.DisableVideoWithBlackColor();
	mTvin.Tvin_StopDecoder();

	tvin_info_t info = mSigDetectThread.getCurSigInfo();
	TvEvent::SignalInfoEvent ev;
	ev.mTrans_fmt = info.trans_fmt;
	ev.mFmt = info.fmt;
	ev.mStatus = info.status;
	ev.mReserved = info.reserved;
	sendTvEvent ( ev );
	LOGD ( "%s, Enable blackscreen for signal change in StableToUnSupport!", __FUNCTION__ );
}
void CTv::onSigStableToNoSig()
{
	mTvAudio.SetAudioMuteForTv(CC_AUDIO_MUTE);
	mAv.DisableVideoWithBlueColor();
	//SetAudioMuteForTv(CC_AUDIO_MUTE);
	mTvin.Tvin_StopDecoder();

	tvin_info_t info = mSigDetectThread.getCurSigInfo();
	TvEvent::SignalInfoEvent ev;
	ev.mTrans_fmt = info.trans_fmt;
	ev.mFmt = info.fmt;
	ev.mStatus = info.status;
	ev.mReserved = info.reserved;
	sendTvEvent ( ev );
	LOGD ( "%s, Enable bluescreen for signal change in StableToNoSig!", __FUNCTION__);
}
void CTv::onSigUnStableToUnSupport()
{
	mAv.DisableVideoWithBlackColor();
	mTvin.Tvin_StopDecoder();
	tvin_info_t info = mSigDetectThread.getCurSigInfo();


	TvEvent::SignalInfoEvent ev;
	ev.mTrans_fmt = info.trans_fmt;
	ev.mFmt = info.fmt;
	ev.mStatus = info.status;
	ev.mReserved = info.reserved;
	sendTvEvent ( ev );
	LOGD ( "%s, Enable blackscreen for signal change in UnStableToUnSupport!", __FUNCTION__);
}
void CTv::onSigUnStableToNoSig()
{
	mTvAudio.SetAudioMuteForTv(CC_AUDIO_MUTE);
	mAv.DisableVideoWithBlueColor();
	//SetAudioMuteForTv(CC_AUDIO_MUTE);
	mTvin.Tvin_StopDecoder();

	tvin_info_t info = mSigDetectThread.getCurSigInfo();
	TvEvent::SignalInfoEvent ev;
	ev.mTrans_fmt = info.trans_fmt;
	ev.mFmt = info.fmt;
	ev.mStatus = info.status;
	ev.mReserved = info.reserved;
	sendTvEvent ( ev );
	LOGD ( "%s, Enable bluescreen for signal change in UnStableToNoSig! status = %d", __FUNCTION__, ev.mStatus );
}
void CTv::onSigNullToNoSig()
{
	mTvAudio.SetAudioMuteForTv(CC_AUDIO_MUTE);
	mAv.DisableVideoWithBlueColor();
	//SetAudioMuteForTv(CC_AUDIO_MUTE);
	mTvin.Tvin_StopDecoder();

	tvin_info_t info = mSigDetectThread.getCurSigInfo();


	TvEvent::SignalInfoEvent ev;
	ev.mTrans_fmt = info.trans_fmt;
	ev.mFmt = info.fmt;
	ev.mStatus = info.status;
	ev.mReserved = info.reserved;
	sendTvEvent ( ev );
	LOGD ( "%s, Enable bluescreen for signal change in NullToNoSig!", __FUNCTION__);
}

void CTv::onSigNoSigToUnstable()
{
	mAv.DisableVideoWithBlueColor();
	LOGD("Enable bluescreen for signal change in NoSigToUnstable\n");
}

void CTv::onSigStillStable()
{
	if ( m_sig_stable_nums == 20) {
		tvin_info_t info = mSigDetectThread.getCurSigInfo();
		TvEvent::SignalInfoEvent ev;
		ev.mTrans_fmt = info.trans_fmt;
		ev.mFmt = info.fmt;
		ev.mStatus = info.status;
		ev.mReserved = info.reserved;
		sendTvEvent ( ev );
	}
	if(m_sig_stable_nums ==  2) {
		int startdec_status = mTvin.Tvin_StartDecoder ( mSigDetectThread.getCurSigInfo() );
		if ( startdec_status == 0 ) { //showboz  codes from  start decode fun
			const char *value;
			value = config_get_str ( "TV", "tvin.db.reg.en", "null" );
			if ( strcmp ( value, "enable" ) == 0 ) {
				usleep ( 20 * 1000 );
				Tvin_SetPLLValues ();
				usleep ( 20 * 1000 );
				SetCVD2Values ();
			}
		}
	}
	if ( m_sig_stable_nums == 25 ) {
		if(!(mTvAction & TV_ACTION_SCANNING))
			mTvAudio.SetAudioMuteForTv ( CC_AUDIO_UNMUTE );
		mAv.EnableVideoAuto();
	}

	m_sig_stable_nums++;
}
void CTv::onSigStillUnstable()
{
}
void CTv::onSigStillNosig()
{

}
void CTv::onSigStillNoSupport()
{

	tvin_info_t info = mSigDetectThread.getCurSigInfo();
	TvEvent::SignalInfoEvent SigEv;
	SigEv.mTrans_fmt = info.trans_fmt;
	SigEv.mFmt = info.fmt;
	SigEv.mStatus = info.status;
	SigEv.mReserved = info.reserved;
	sendTvEvent ( SigEv );
	LOGD ( "%s, Unsupport signal", __FUNCTION__);
}
void CTv::onSigStillNull()
{
}
void CTv::onStableSigFmtChange()
{
}
void CTv::onStableTransFmtChange()
{
}
void CTv::onSigDetectEnter()
{
	struct timeval time;
	gettimeofday ( &time, NULL );
}

void CTv::onLoop()
{
	if ( ( m_is_hdmi_sr_detect_start == true )
			&& ( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) == SOURCE_TYPE_HDMI ) ) {
		int sr = mTvin.get_hdmi_sampling_rate();

		if ( ( sr > 0 ) && ( sr != m_hdmi_sampling_rate ) ) {
			if ( 0 == m_hdmi_sampling_rate ) {
				LOGD ( "%s, Init HDMI audio, sampling rate:%d", __FUNCTION__,  sr );
				mTvAudio.AudioDataInit ( sr, false );
				audio_sr = sr;
			} else {
				LOGD ( "%s, Reset HDMI sampling rate to %d", __FUNCTION__,  sr );
				mTvAudio.AudioChangeSampleRate ( sr );
			}

			m_hdmi_sampling_rate = sr;
		}
	}
}

void CTv::onSourceConnect(int source_type, int connect_status)
{
	TvEvent::SourceConnectEvent ev;
	ev.mSourceInput = source_type;
	ev.connectionState = connect_status;
	sendTvEvent(ev);
}

int CTv::GetSourceConnectStatus(int source_input)
{
	return mSourceConnectDetectThread.GetSourceConnectStatus(source_input);
}

tv_source_input_t CTv::GetCurrentSourceInputLock ( void )
{
	Mutex::Autolock _l ( mLock );
	return m_source_input;
}

//dtv and tvin
tvin_info_t CTv::GetCurrentSignalInfo ( void )
{
	tvin_trans_fmt det_fmt = TVIN_TFMT_2D;
	tvin_sig_status_t signalState = TVIN_SIG_STATUS_NULL;
	tvin_info_t signal_info = mSigDetectThread.getCurSigInfo();

	if ( (CTvin::Tvin_SourceInputToSourceInputType(m_source_input) == SOURCE_TYPE_DTV ) ) {
		for ( int i = 0; i < 10; i++ ) {
			det_fmt = mTvin.TvinApi_Get3DDectMode();
			signal_info.status = TVIN_SIG_STATUS_STABLE;

			if ( det_fmt != TVIN_TFMT_2D ) {
				signal_info.trans_fmt = det_fmt;
				break;
			}
		}
	}
	return signal_info;
}

int CTv::Tv_Set3DMode ( VIDEO_3D_MODE_T mode )
{
	if (mode == VIDEO_3D_MODE_AUTO) {
		mTvin.VDIN_SetDI3DDetc (1);
	} else {
		mTvin.VDIN_SetDI3DDetc (0);
	}

	mAv.set3DMode(mode, 0, 0);
	m_mode_3d = mode;
	SSMSave3DMode ( ( unsigned char ) mode );
	SetBypassModulesFor3D ( mode );
	return 0;
}

VIDEO_3D_MODE_T CTv::Tv_Get3DMode ( void )
{
	return m_mode_3d;
}

int CTv::Tv_Set3DLRSwith ( int on_off)
{
	LOGW ( "%s,Set3D LRSwith on_off %d ,status %d !!! ",  __FUNCTION__, on_off);
	mAv.set3DMode(m_mode_3d, on_off, 0);
	SSMSave3DLRSwitch(on_off);
	return 0;
}

int CTv::Tv_Get3DLRSwith ( void )
{
	unsigned char val = 0;
	SSMRead3DLRSwitch ( &val );
	return ( int ) val;
}

int CTv::Tv_Set3DTo2DMode ( int mode)
{
	LOGW ( "%s,Set3D to 2D mode %d ,status %d !!! ",  __FUNCTION__  , mode);
	mAv.set3DMode(m_mode_3d, 0, mode);
	SSMSave3DTO2D ( mode );
	return 0;
}

int CTv::Tv_Get3DTo2DMode ( void )
{
	unsigned char val = 0;
	SSMRead3DTO2D ( &val );
	return val;
}

int CTv::Tv_Set3DDepth ( int value )
{
	mTvin.Tvin_SetDepthOf2Dto3D( value );
	SSMSave3DDepth ( value );
	return 0;
}

int CTv::GetSave3DDepth ( void )
{
	unsigned char val = 0;
	SSMRead3DDepth ( &val );
	return val;
}

is_3d_type_t CTv::Check2Dor3D ( VIDEO_3D_MODE_T mode3d, tvin_trans_fmt_t trans_fmt )
{
	if ( mode3d == VIDEO_3D_MODE_DISABLE ) {
		return INDEX_2D;
	} else if ( mode3d == VIDEO_3D_MODE_AUTO ) {
		if ( trans_fmt == TVIN_TFMT_2D ) {
			return INDEX_2D;
		} else {
			return INDEX_3D;
		}
	} else {
		return INDEX_3D;
	}
}


int CTv::Tvin_SetPLLValues ()
{
	tvin_sig_fmt_t  sig_fmt = mSigDetectThread.getCurSigInfo().fmt;
	tvin_port_t source_port =  CTvin::Tvin_GetSourcePortBySourceInput(m_source_input);
	am_regs_t regs;

	if ( mVpp.getPqData()->PQ_GetPLLParams ( source_port, sig_fmt, &regs ) == 0 ) {
		LOGD ("%s,PQ_GetPLLParams(source_port[%d], sig_fmt[%d],&regs).\n", __FUNCTION__, source_port, sig_fmt );

		if ( mTvin.TvinApi_LoadPLLValues ( regs ) < 0 ) {
			LOGE ( "%s, TvinApi_LoadPLLValues failed!\n", __FUNCTION__ );
			return -1;
		}
	} else {
		LOGE ( "%s, PQ_GetPLLParams failed!\n", __FUNCTION__ );
		return -1;
	}

	return 0;
}
int CTv::SetCVD2Values ()
{
	tvin_sig_fmt_t  sig_fmt = mSigDetectThread.getCurSigInfo().fmt;
	tvin_port_t source_port = CTvin::Tvin_GetSourcePortBySourceInput(m_source_input);
	am_regs_t regs;

	if (mVpp.getPqData()->PQ_GetCVD2Params ( source_port, sig_fmt, &regs ) == 0) {
		LOGD ( "%s, PQ_GetCVD2Params(source_port[%d], sig_fmt[%d],&regs).\n", __FUNCTION__,
			   source_port, sig_fmt );

		if ( mTvin.TvinApi_LoadCVD2Values ( regs ) < 0 ) {
			LOGE ( "%s, TvinApi_LoadCVD2Values failed!\n", __FUNCTION__);
			return -1;
		}
	} else {
		LOGE ( "%s, PQ_GetCVD2Params failed!\n", __FUNCTION__);
		return -1;
	}

	return 0;
}

int CTv::SetPreviewWindow ( tvin_window_pos_t pos )
{
	m_win_pos.x1 = pos.x1;
	m_win_pos.y1 = pos.y1;
	m_win_pos.x2 = pos.x2;
	m_win_pos.y2 = pos.y2;
	LOGD ( "%s, SetPreviewWindow x = %d y=%d", __FUNCTION__, pos.x2, pos.y2 );

	tvin_window_pos_t def_pos;
	Vpp_GetDisplayResolutionInfo(&def_pos);

	if (pos.x1 != 0 || pos.y1 != 0 || pos.x2 != def_pos.x2 || pos.y2 != def_pos.y2) {
		m_win_mode = PREVIEW_WONDOW;
	} else {
		m_win_mode = NORMAL_WONDOW;
	}

	return mAv.setVideoAxis(m_win_pos.x1, m_win_pos.y1, m_win_pos.x2, m_win_pos.y2);;
}

/*********************** Audio start **********************/
int CTv::SetAudioVolDigitLUTBuf ( tv_source_input_t source_input )
{
	if (source_input == SOURCE_TV) {
		mTvAudio.AudioSetVolumeDigitLUTBuf ( CC_LUT_SEL_TV, 1, 1, -1 );
	} else if (source_input == SOURCE_AV1 || source_input == SOURCE_AV2) {
		mTvAudio.AudioSetVolumeDigitLUTBuf ( CC_LUT_SEL_AV, 1, 1, -1 );
	} else if (source_input == SOURCE_YPBPR1 || source_input == SOURCE_YPBPR2) {
		mTvAudio.AudioSetVolumeDigitLUTBuf ( CC_LUT_SEL_COMP, 1, 1, -1 );
	} else if (source_input == SOURCE_VGA) {
		mTvAudio.AudioSetVolumeDigitLUTBuf ( CC_LUT_SEL_VGA, 1, 1, -1 );
	} else if (source_input == SOURCE_HDMI1 || source_input == SOURCE_HDMI2 || source_input == SOURCE_HDMI3) {
		mTvAudio.AudioSetVolumeDigitLUTBuf ( CC_LUT_SEL_HDMI, 1, 1, -1 );
	} else if ( source_input == SOURCE_MPEG ) {
		mTvAudio.AudioSetVolumeDigitLUTBuf ( CC_LUT_SEL_MPEG, 1, 1, -1 );
	} else if ( source_input == SOURCE_DTV ) {
		mTvAudio.AudioSetVolumeDigitLUTBuf ( CC_LUT_SEL_MPEG, 1, 1, -1 );
	}
	return 0;
}

void CTv::RefreshAudioMasterVolume ( tv_source_input_t source_input )
{
	if (source_input == SOURCE_HDMI1 || source_input == SOURCE_HDMI2 || source_input == SOURCE_HDMI3) {
		if ( GetAudioDVISupportEnable() == 1 ) {
			if ( IsDVISignal() ) {
				SetAudioVolDigitLUTBuf ( SOURCE_MPEG );
				mTvAudio.SetAudioMasterVolume ( mTvAudio.GetAudioMasterVolume() );
				return;
			}
		}
	}

	SetAudioVolDigitLUTBuf ( source_input );
	mTvAudio.SetAudioMasterVolume ( mTvAudio.GetAudioMasterVolume() );
}

int CTv::Tv_SetAudioInSource (tv_source_input_t source_input)
{
	if (source_input == SOURCE_TV) {
		if (mTvin.Tvin_GetAudioInSourceType(source_input) == TV_AUDIO_IN_SOURCE_TYPE_ATV) {
			return mTvAudio.AudioSetAudioInSource(CC_AUDIO_IN_SOURCE_ATV);
		} else {
			return mTvAudio.AudioSetAudioInSource(CC_AUDIO_IN_SOURCE_LINEIN);
		}
	} else if (source_input == SOURCE_AV1 || source_input == SOURCE_AV2) {
		return mTvAudio.AudioSetAudioInSource(CC_AUDIO_IN_SOURCE_LINEIN);
	} else if (source_input == SOURCE_HDMI1 || source_input == SOURCE_HDMI2 || source_input == SOURCE_HDMI3) {
		return mTvAudio.AudioSetAudioInSource(CC_AUDIO_IN_SOURCE_HDMI);
	} else if (source_input == SOURCE_YPBPR1 || source_input == SOURCE_YPBPR2 ||
			   source_input == SOURCE_TYPE_VGA) {
		return  mTvAudio.AudioSetAudioInSource(CC_AUDIO_IN_SOURCE_LINEIN);
	}

	return 0;
}

int CTv::Tv_SetAudioSourceType (tv_source_input_t source_input)
{
	int audio_source = -1;

	if (source_input == SOURCE_TV) {
		audio_source = AUDIO_ATV_SOURCE;
	} else if (source_input == SOURCE_AV1 || source_input == SOURCE_AV2) {
		audio_source = AUDIO_AV_SOURCE;
	} else if (source_input == SOURCE_HDMI1 || source_input == SOURCE_HDMI2 || source_input == SOURCE_HDMI3) {
		audio_source = AUDIO_HDMI_SOURCE;
	} else if (source_input == SOURCE_YPBPR1 || source_input == SOURCE_YPBPR2 ||
			   source_input == SOURCE_TYPE_VGA) {
		audio_source = AUDIO_AV_SOURCE;
	} else if (source_input == SOURCE_DTV) {
		audio_source = AUDIO_MPEG_SOURCE;
	} else {
		audio_source = AUDIO_MPEG_SOURCE;
	}

	return mTvAudio.AudioSetAudioSourceType(audio_source);
}
void CTv::Tv_SetAudioOutputSwap_Type (tv_source_input_t source_input)
{
	const char *config_value;
	config_value = config_get_str("TV", "audio.output.swap.enable", "null");
	if ( strcmp ( config_value, "enable" ) == 0 ) {
		int sw_status = 0;

		LOGD("%s, audio.output.swap.enable\n", __FUNCTION__);

		switch(source_input) {
		case SOURCE_AV1:
			config_value = config_get_str("TV", "audio.output.swap.av1", "null");
			sw_status = atoi ( config_value );
			mTvAudio.SetOutput_Swap(sw_status);
			LOGD("%s, av1 %d\n", __FUNCTION__, sw_status);
			break;
		case SOURCE_AV2:
			config_value = config_get_str("TV", "audio.output.swap.av2", "null");
			sw_status = atoi ( config_value );
			mTvAudio.SetOutput_Swap(sw_status);
			LOGD("%s, av2 %d\n", __FUNCTION__, sw_status);
			break;
		case SOURCE_HDMI1:
			config_value = config_get_str("TV", "audio.output.swap.hdmi1", "null");
			sw_status = atoi ( config_value );
			mTvAudio.SetOutput_Swap(sw_status);
			LOGD("%s, hdmi1 %d\n", __FUNCTION__, sw_status);
			break;
		case SOURCE_HDMI2:
			config_value = config_get_str("TV", "audio.output.swap.hdmi2", "null");
			sw_status = atoi ( config_value );
			mTvAudio.SetOutput_Swap(sw_status);
			LOGD("%s, hdmi2 %d\n", __FUNCTION__, sw_status);
			break;
		case SOURCE_HDMI3:
			config_value = config_get_str("TV", "audio.output.swap.hdmi3", "null");
			sw_status = atoi ( config_value );
			mTvAudio.SetOutput_Swap(sw_status);
			LOGD("%s, hdmi3 %d\n", __FUNCTION__, sw_status);
			break;
		case SOURCE_TV:
			config_value = config_get_str("TV", "audio.output.swap.tv", "null");
			sw_status = atoi ( config_value );
			mTvAudio.SetOutput_Swap(sw_status);
			LOGD("%s, tv %d\n", __FUNCTION__, sw_status);
			break;
		case SOURCE_DTV:
			config_value = config_get_str("TV", "audio.output.swap.dtv", "null");
			sw_status = atoi ( config_value );
			mTvAudio.SetOutput_Swap(sw_status);
			LOGD("%s, dtv %d\n", __FUNCTION__, sw_status);
			break;
		case SOURCE_MPEG:
			config_value = config_get_str("TV", "audio.output.swap.mpeg", "null");
			sw_status = atoi ( config_value );
			mTvAudio.SetOutput_Swap(sw_status);
			LOGD("%s, mpeg %d\n", __FUNCTION__, sw_status);
			break;
		default:
			mTvAudio.SetOutput_Swap(sw_status);
			LOGD("%s, default %d\n", __FUNCTION__, sw_status);
			break;
		}
	}

}

/*********************** Audio end **********************/

unsigned int CTv::Vpp_GetDisplayResolutionInfo(tvin_window_pos_t *win_pos)
{
	int display_resolution = mAv.getVideoDisplayResolution();
	unsigned int tmp_ret = 0;

	switch(display_resolution) {
	case VPP_DISPLAY_RESOLUTION_1366X768:
		tmp_ret |= (CC_RESOLUTION_1366X768_W << 16);
		tmp_ret |= (CC_RESOLUTION_1366X768_H << 0);
		break;
	case VPP_DISPLAY_RESOLUTION_1920X1080:
		tmp_ret |= (CC_RESOLUTION_1920X1080_W << 16);
		tmp_ret |= (CC_RESOLUTION_1920X1080_H << 0);
		break;
	case VPP_DISPLAY_RESOLUTION_3840X2160:
		tmp_ret |= (CC_RESOLUTION_3840X2160_W << 16);
		tmp_ret |= (CC_RESOLUTION_3840X2160_H << 0);
		break;
	default:
		tmp_ret |= (CC_RESOLUTION_3840X2160_W << 16);
		tmp_ret |= (CC_RESOLUTION_3840X2160_H << 0);
		break;
	}

	if (win_pos != NULL) {
		win_pos->x1 = 0;
		win_pos->y1 = 0;
		win_pos->x2 = ((tmp_ret >> 16) & 0xFFFF) - 1;
		win_pos->y2 = ((tmp_ret >> 0) & 0xFFFF) - 1;
	}

	return 0;
}


int CTv::setBlackoutEnable(int enable)
{
	m_blackout_enable = enable;
	return SSMSaveBlackoutEnable(enable);
}

int CTv::getSaveBlackoutEnable()
{
	int8_t enable;
	SSMReadBlackoutEnable(&enable);
	return enable;
}
void CTv::startAutoBackLight()
{
	mAutoBackLight.startAutoBacklight(CTvin::Tvin_SourceInputToSourceInputType(m_source_input));
}

void CTv::stopAutoBackLight()
{
	mAutoBackLight.stopAutoBacklight();
}

int CTv::getAutoBackLight_on_off()
{
	return mAutoBackLight.isAutoBacklightOn() ? 1 : 0;
}
/*********************** SSM  start **********************/
int CTv::Tv_SSMRestoreDefaultSetting()
{
	SSMRestoreDeviceMarkValues();
	mTvAudio.AudioSSMRestoreDefaultSetting();
	mVpp.VPPSSMRestoreDefault();
	MiscSSMRestoreDefault();
	ReservedSSMRestoreDefault();
	SSMSaveCVBSStd ( 0 );
	SSMSaveLastSelectSourceInput ( SOURCE_TV );
	SSMSavePanelType ( 0 );
	//tvconfig default
	saveDTVProgramID ( -1 );
	saveATVProgramID ( -1 );
	return 0;
}

int CTv::clearDbAllProgramInfoTable()
{
	return CTvDatabase::GetTvDb()->clearDbAllProgramInfoTable();
}

int CTv::Tv_SSMFacRestoreDefaultSetting()
{
	mVpp.VPPSSMFacRestoreDefault();
	mTvAudio.AudioSSMRestoreDefaultSetting();
	MiscSSMFacRestoreDefault();

	return 0;
}
/*********************** SSM  End **********************/
//not in CTv, not use lock
void CTv::setSourceSwitchAndPlay()
{
	int progID = 0;
	LOGD ( "%s\n", __FUNCTION__  );
	static const int  POWERON_SOURCE_TYPE_NONE = 0;//not play source
	static const int  POWERON_SOURCE_TYPE_LAST = 1;//play last save source
	static const int  POWERON_SOURCE_TYPE_SETTING = 2;//play ui set source
	int to_play_source = -1;
	int powerup_type = SSMReadPowerOnOffChannel();
	LOGD("read  power on  source type = %d", powerup_type);
	if(powerup_type == POWERON_SOURCE_TYPE_NONE) {
		return ;
	} else if(powerup_type == POWERON_SOURCE_TYPE_LAST) {
		to_play_source = SSMReadSourceInput();
	} else if(powerup_type == POWERON_SOURCE_TYPE_SETTING) {
		to_play_source =  SSMReadLastSelectSourceInput();
	}
	SetSourceSwitchInput (( tv_source_input_t ) to_play_source );
	if ( to_play_source == SOURCE_TV ) {
		progID = getATVProgramID();
	} else if ( to_play_source == SOURCE_DTV ) {
		progID = getDTVProgramID();
	}
	playProgramLock(progID);
	return;
}

int CTv::startCC(int country, int src, int channel, int service)
{
	//turn_on_cc = true;
	return mTvSub.sub_start_atsc_cc((enum cc_param_country)country, (enum cc_param_source_type)src, channel, (enum cc_param_caption_type)service);
}

int CTv::stopCC()
{
	//because cc,vchip data both come from vbi thread , here judge cc, vchip is whether  both turn off
	/*turn_on_cc = false;
	if(config_get_int("TV","tv.vchip.enable", 0))
	{
	    return 0;  //at ATV if vchip is on, turn off CC, just set flag not display CC, but vchip still running
	}*/
	return mTvSub.sub_stop_atsc_cc();
}

void CTv::printDebugInfo()
{
	print_version_info();
	LOGD("%s, TvAction = %x",  __FUNCTION__, mTvAction);
	LOGD("%s, TvRunStatus = %d",  __FUNCTION__, mTvStatus);
	LOGD("%s, TvCurSourceInput = %d",  __FUNCTION__, m_source_input);
	LOGD("%s, TvLastSourceInput = %d",  __FUNCTION__, m_last_source_input);
}
//==============vchip end================================
//----------------DVR API============================
void CTv::SetRecordFileName ( char *name )
{
	char tmp[256];
	strcpy ( tmp, name );
	mTvRec.SetRecordFileName ( tmp );
}
void CTv::StartToRecord()
{
	int progID = getDTVProgramID();
	mTvRec.StartRecord ( progID );
}
void CTv::StopRecording()
{
	mTvRec.StopRecord();
}
void CTv::SetRecCurTsOrCurProgram ( int sel )
{
	mTvRec.SetRecCurTsOrCurProgram ( sel );
}

int CTv::GetDisplayResolutionConfig()
{
	return mAv.getVideoDisplayResolution();
}

int CTv::GetDisplayResolutionInfo()
{
	return Vpp_GetDisplayResolutionInfo(NULL);
}

void CTv::onHDMIRxCECMessage(int msg_len, unsigned char msg_buf[])
{
	int i = 0;
	TvEvent::HDMIRxCECEvent ev;
	ev.mDataCount = msg_len;
	for(i = 0; i < msg_len; i++) {
		ev.mDataBuf[i] = msg_buf[i];
	}
	sendTvEvent(ev);
}

int CTv::SendHDMIRxCECCustomMessage(unsigned char data_buf[])
{
	tv_source_input_t source_input =  m_source_input;

	return mHDMIRxCEC.SendCustomMessage(source_input, data_buf);
}

int CTv::SendHDMIRxCECCustomMessageAndWaitReply(unsigned char data_buf[], unsigned char reply_buf[], int WaitCmd, int timeout)
{
	tv_source_input_t source_input =  m_source_input;

	return mHDMIRxCEC.SendCustomMessageAndWaitReply(source_input, data_buf, reply_buf, WaitCmd, timeout);
}

int CTv::SendHDMIRxCECBoradcastStandbyMessage(void)
{
	tv_source_input_t source_input =  m_source_input;

	return mHDMIRxCEC.SendBoradcastStandbyMessage(source_input);
}

int CTv::SendHDMIRxCECGiveCECVersionMessage(tv_source_input_t source_input, unsigned char data_buf[])
{
	if (mHDMIRxCEC.processRefreshSrcDevice(source_input) == 0) {
		return mHDMIRxCEC.SendGiveCECVersionMessage(source_input, data_buf);
	}

	return -1;
}

int CTv::SendHDMIRxCECGiveDeviceVendorIDMessage(tv_source_input_t source_input, unsigned char data_buf[])
{
	if (mHDMIRxCEC.processRefreshSrcDevice(source_input) == 0) {
		return mHDMIRxCEC.SendGiveDeviceVendorIDMessage(source_input, data_buf);
	}

	return -1;
}

int CTv::SendHDMIRxCECGiveOSDNameMessage(tv_source_input_t source_input, unsigned char data_buf[])
{
	if (mHDMIRxCEC.processRefreshSrcDevice(source_input) == 0) {
		return mHDMIRxCEC.SendGiveOSDNameMessage(source_input, data_buf);
	}

	return -1;
}

int CTv::GetHdmiHdcpKeyKsvInfo(int data_buf[])
{
	return mTvin.get_hdmi_ksv_info(m_source_input, data_buf);
}

void CTv::onUpgradeStatus(int state, int param)
{
	TvEvent::UpgradeFBCEvent ev;
	ev.mState = state;
	ev.param = param;
	sendTvEvent(ev);
}

int CTv::StartUpgradeFBC(char *file_name, int mode, int upgrade_blk_size)
{
	if (mpUpgradeFBC != NULL) {
		mpUpgradeFBC->SetUpgradeFileName(file_name);
		mpUpgradeFBC->SetUpgradeMode(mode);
		mpUpgradeFBC->SetUpgradeBlockSize(upgrade_blk_size);
		mpUpgradeFBC->start();
		return 0;
	}

	return -1;
}

void CTv::onSerialCommunication(int dev_id, int rd_len, unsigned char data_buf[])
{
	int i = 0;
	TvEvent::SerialCommunicationEvent ev;
	ev.mDevId = dev_id;
	ev.mDataCount = rd_len;
	for(i = 0; i < rd_len; i++) {
		ev.mDataBuf[i] = data_buf[i];
	}
	sendTvEvent(ev);
}

void CTv::onThermalDetect(int state)
{
	const char *value;
	const char *value_normal;
	const char *value_cold;
	int threshold = 0, val = 0;
	static int pre_val = -1;

	value = config_get_str ( "TV", "tvin.thermal.threshold.enable", "null" );

	if ( strcmp ( value, "enable" ) == 0 ) {

		value = config_get_str ( "TV", "tvin.thermal.threshold.value", "null" );

		threshold = atoi(value);
		LOGD ( "%s, threshold value:   %d\n", __FUNCTION__, threshold);

		if(state > threshold) {
			value_normal = config_get_str ( "TV", "tvin.thermal.fbc.normal.value", "null" );
			val = atoi(value_normal);
			if(val == 0) {
				val = 0x4210000;    //normal default
			}
			LOGD ( "%s, current temp:   %d set 1\n", __FUNCTION__, state);
		} else {
			value_cold = config_get_str ( "TV", "tvin.thermal.fbc.cold.value", "null" );
			val = atoi(value_cold);
			if(val == 0) {
				val = 0x8210000;    //cold default
			}
			LOGD ( "%s, current temp:   0x%x set 0\n", __FUNCTION__, state);
		}

		if(pre_val == val) {
			LOGD ( "%s, pre_val == val : 0x%x,bypass\n", __FUNCTION__, val);
		} else {
			pre_val = val;
			Tv_FactorySet_FBC_Thermal_State(val);
			LOGD ( "%s, pre_val :0x%x,bypass\n", __FUNCTION__, pre_val);
		}
	} else {
		LOGD ( "%s, tvin.thermal.threshold.enable == disable\n", __FUNCTION__);
	}
}

int CTv::SetSerialSwitch(int dev_id, int switch_val)
{
	int tmp_ret = 0;

	if (dev_id == SERIAL_A) {
		if (switch_val == 0) {
			tmp_ret |= mSerialA.stop();
			tmp_ret |= mSerialA.CloseModule();

			tmp_ret |= system("start console");
		} else {
			tmp_ret |= system("stop console");

			mSerialA.OpenModule(dev_id);
			tmp_ret |= mSerialA.start();
		}
	} else if (dev_id == SERIAL_B) {
		if (switch_val == 0) {
			tmp_ret |= mSerialB.stop();
			tmp_ret |= mSerialB.CloseModule();
		} else {
			mSerialB.OpenModule(dev_id);
			tmp_ret = mSerialB.start();
		}
	} else if (dev_id == SERIAL_C) {
		if (switch_val == 0) {
			tmp_ret |= mSerialC.stop();
			tmp_ret |= mSerialC.CloseModule();
		} else {
			mSerialC.OpenModule(dev_id);
			tmp_ret = mSerialC.start();
		}
	}

	return tmp_ret;
}

int CTv::SendSerialData(int dev_id, int data_len, unsigned char data_buf[])
{
	int tmp_ret = 0;

	if (dev_id == SERIAL_A) {
		tmp_ret = mSerialA.sendData(data_len, data_buf);
	} else if (dev_id == SERIAL_B) {
		tmp_ret = mSerialB.sendData(data_len, data_buf);
	} else if (dev_id == SERIAL_C) {
		tmp_ret = mSerialC.sendData(data_len, data_buf);
	}

	return tmp_ret;
}

int CTv::ChannelExport(const char *destPath)
{
	//DIR *dirptr = NULL;
	//dirptr = opendir("/storage/external_storage/sda1/");
	//if(NULL == dirptr) {
	//    LOGD("%s, please insert the udisk !",__FUNCTION__);
	//   return -2;
	// } else {
	char tmp[256];
	FILE *fp = NULL;
	if(destPath == NULL) {
		destPath = "/storage/external_storage/sda1/";
	}

	//LOGD("%s,  udisk exist !",__FUNCTION__);
	sprintf(tmp, "cp /param/dtv.db %s", destPath);
	if(system(tmp) >= 0) {
		LOGD("%s, copy dtv.db from /param to udisk success !", __FUNCTION__);
		system("sync");
		fp = fopen(destPath, "r");
		if (fp == NULL) {
			return -1;
		} else {
			fclose(fp);
			fp = NULL;
			return 0;
		}
	} else {
		return -1;
	}
	//}
}

int CTv::ChannelImport(const char *srcPath)
{
	if(srcPath == NULL) {
		srcPath = "/storage/external_storage/sda1/dvb.db";
	}

	if(Tv_Utils_IsFileExist(srcPath) == 0) {
		char tmp[256];
		LOGD("%s,   file exist !" , srcPath);
		CTvDatabase::GetTvDb()->UnInitTvDb();
		sprintf(tmp, "rm /param/dtv.db");
		if(system(tmp) >= 0) {
			LOGD("%s, rm /param/dtv.db success !", __FUNCTION__);
			memset(tmp, 0, sizeof(tmp));
			sprintf(tmp, "cp %s /param/dtv.db", srcPath);
			if(system(tmp) >= 0) {
				LOGD("%s,  copy  to /param/dtv.db success !", srcPath);
				system("sync");
				CTvDatabase::GetTvDb()->InitTvDb(TV_DB_PATH);
				return 0;
			} else {
				LOGD("%s, copy dtv.db from udisk to /param failed !", __FUNCTION__);
				return -1;
			}
		} else {
			LOGD("%s, rm /param/dtv.db failed !", __FUNCTION__);
			return -2;
		}
	} else {
		LOGD("%s, dtv.db file does not exist in the udisk!" , srcPath);
		return -2;
	}

}

int CTv::Tv_GetProjectInfo(project_info_t *ptrInfo)
{
	return GetProjectInfo(ptrInfo);
}

int CTv::Tv_GetPlatformType()
{
	return GetPlatformHaveFBCFlag();
}

int CTv::Tv_SetDDDRCMode(tv_source_input_t source_input)
{
	if (source_input == SOURCE_DTV) {
		if (GetPlatformHaveDDFlag() == 1) {
			Tv_Utils_SetFileAttrStr("/sys/class/audiodsp/ac3_drc_control", "drcmode 3");
		}
	} else {
		if (GetPlatformHaveDDFlag() == 1) {
			Tv_Utils_SetFileAttrStr("/sys/class/audiodsp/ac3_drc_control", "drcmode 2");
		}
	}

	return 0;
}

//PQ
int CTv::Tv_SetBrightness ( int brightness, tv_source_input_type_t source_type, int is_save )
{
	return mVpp.Tv_SetBrightness(brightness, (tv_source_input_type_t)source_type, mSigDetectThread.getCurSigInfo().fmt, mSigDetectThread.getCurSigInfo().trans_fmt, Check2Dor3D(m_mode_3d, mSigDetectThread.getCurSigInfo().trans_fmt ), is_save);
}

int CTv::Tv_GetBrightness ( tv_source_input_type_t source_type )
{
	return mVpp.Tv_GetBrightness((tv_source_input_type_t)source_type);
}

int CTv::Tv_SetContrast ( int contrast, tv_source_input_type_t source_type,  int is_save )
{
	return mVpp.Tv_SetContrast(contrast, (tv_source_input_type_t)source_type, mSigDetectThread.getCurSigInfo().fmt, mSigDetectThread.getCurSigInfo().trans_fmt, Check2Dor3D(m_mode_3d, mSigDetectThread.getCurSigInfo().trans_fmt ), is_save);
}

int CTv::Tv_GetContrast ( tv_source_input_type_t source_type )
{
	return mVpp.Tv_GetContrast((tv_source_input_type_t)source_type);
}

int CTv::Tv_SetSaturation ( int satuation, tv_source_input_type_t source_type, tvin_sig_fmt_t fmt, int is_save )
{
	return mVpp.Tv_SetSaturation(satuation, (tv_source_input_type_t)source_type, (tvin_sig_fmt_t)fmt, mSigDetectThread.getCurSigInfo().trans_fmt, Check2Dor3D(m_mode_3d, mSigDetectThread.getCurSigInfo().trans_fmt ), is_save);
}

int CTv::Tv_GetSaturation ( tv_source_input_type_t source_type )
{
	return mVpp.Tv_GetSaturation((tv_source_input_type_t)source_type);
}

int CTv::Tv_SetHue ( int hue, tv_source_input_type_t source_type, tvin_sig_fmt_t fmt, int is_save )
{
	return mVpp.Tv_SetHue(hue, (tv_source_input_type_t)source_type, (tvin_sig_fmt_t)fmt, mSigDetectThread.getCurSigInfo().trans_fmt, Check2Dor3D(m_mode_3d, mSigDetectThread.getCurSigInfo().trans_fmt ), is_save);
}

int CTv::Tv_GetHue ( tv_source_input_type_t source_type )
{
	return mVpp.Tv_GetHue((tv_source_input_type_t)source_type);
}

int CTv::Tv_SetPQMode ( vpp_picture_mode_t mode, tv_source_input_type_t source_type, int is_save )
{
	return mVpp.Tv_SetPQMode((vpp_picture_mode_t)mode, (tv_source_input_type_t)source_type, mSigDetectThread.getCurSigInfo().fmt, mSigDetectThread.getCurSigInfo().trans_fmt, Check2Dor3D(m_mode_3d, mSigDetectThread.getCurSigInfo().trans_fmt ), is_save);
}

vpp_picture_mode_t CTv::Tv_GetPQMode ( tv_source_input_type_t source_type )
{
	return mVpp.Tv_GetPQMode((tv_source_input_type_t)source_type);
}

int CTv::Tv_SetSharpness ( int value, tv_source_input_type_t source_type, int en, int is_save )
{
	return mVpp.Tv_SetSharpness(value, (tv_source_input_type_t)source_type, en, Check2Dor3D(m_mode_3d, mSigDetectThread.getCurSigInfo().trans_fmt ), mSigDetectThread.getCurSigInfo().fmt, mSigDetectThread.getCurSigInfo().trans_fmt, is_save);
}

int CTv::Tv_GetSharpness ( tv_source_input_type_t source_type )
{
	return mVpp.Tv_GetSharpness((tv_source_input_type_t)source_type);
}

int CTv::Tv_SaveSharpness ( int value, tv_source_input_type_t source_type, int en )
{
	if ( en == 1 ) {
		return SSMSaveSharpness( (tv_source_input_type_t)source_type, value );
	} else {
		return 0;
	}
}

int CTv::Tv_SetBacklight ( int value, tv_source_input_type_t source_type, int is_save )
{
	return mVpp.Tv_SetBacklight(value, (tv_source_input_type_t)source_type, is_save);
}

int CTv::Tv_GetBacklight ( tv_source_input_type_t source_type )
{
	return mVpp.Tv_GetBacklight((tv_source_input_type_t)source_type);
}

int CTv::Tv_SetColorTemperature ( vpp_color_temperature_mode_t mode, tv_source_input_type_t source_type, int is_save )
{
	return mVpp.Tv_SetColorTemperature((vpp_color_temperature_mode_t)mode, (tv_source_input_type_t)source_type, is_save);
}

vpp_color_temperature_mode_t CTv::Tv_GetColorTemperature ( tv_source_input_type_t source_type )
{
	return mVpp.Tv_GetColorTemperature((tv_source_input_type_t)source_type);
}

int CTv::Tv_SaveColorTemp ( vpp_color_temperature_mode_t mode, tv_source_input_type_t source_type )
{
	return mVpp.Tv_SaveColorTemp((vpp_color_temperature_mode_t)mode, (tv_source_input_type_t)source_type);
}

int CTv::Tv_SetDisplayMode ( vpp_display_mode_t mode, tv_source_input_type_t source_type, tvin_sig_fmt_t fmt, int is_save )
{
	int ret = Vpp_SetDisplayMode((vpp_display_mode_t)mode, (tv_source_input_type_t)source_type, (tvin_sig_fmt_t)fmt);
	if(ret == 0) {
		if(is_save == 1) {
			ret =  ret | SSMSaveDisplayMode ( source_type, (int)mode );
		}
	}
	return ret;
}

int CTv::Vpp_SetDisplayMode ( vpp_display_mode_t display_mode, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt )
{
	LOGD("%s, display_mode = %d, source_type = %d. \n", __FUNCTION__,  display_mode, source_type);

	tvin_cutwin_t  cutwin = mVpp.Tv_GetOverscan ( source_type, sig_fmt, Check2Dor3D(m_mode_3d, mSigDetectThread.getCurSigInfo().trans_fmt ), mSigDetectThread.getCurSigInfo().trans_fmt);
	int  video_screen_mode = CAv::VIDEO_WIDEOPTION_16_9;
	tvin_window_pos_t win_pos;
	int display_resolution = Vpp_GetDisplayResolutionInfo(&win_pos);


	if ( source_type == SOURCE_TYPE_HDMI ) {
		if ( sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_60HZ
				|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080I_60HZ
				|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_50HZ
				|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_A
				|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_B
				|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_24HZ
				|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_25HZ
				|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_30HZ
				|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080I_100HZ
				|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080I_120HZ
				|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_100HZ
				|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_120HZ
				|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_FRAME_PACKING
				|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_24HZ_FRAME_PACKING
				|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_30HZ_FRAME_PACKING
				|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080I_60HZ_ALTERNATIVE
				|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_ALTERNATIVE
				|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_24HZ_ALTERNATIVE
				|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_30HZ_ALTERNATIVE
				|| sig_fmt == TVIN_SIG_FMT_HDMI_3840_2160_00HZ
				|| sig_fmt == TVIN_SIG_FMT_HDMI_4096_2160_00HZ

		   ) {
			if ( display_mode == VPP_DISPLAY_MODE_NOSCALEUP ) {
				cutwin.vs = 0;
				cutwin.hs = 1;
				cutwin.ve = 0;
				cutwin.he = 1;
			}
		}
	}

	switch ( display_mode ) {
	case VPP_DISPLAY_MODE_169:
		video_screen_mode = CAv::VIDEO_WIDEOPTION_16_9;
		break;
	case VPP_DISPLAY_MODE_MODE43:
		video_screen_mode = CAv::VIDEO_WIDEOPTION_4_3;
		break;
	case VPP_DISPLAY_MODE_NORMAL:
		video_screen_mode = CAv::VIDEO_WIDEOPTION_NORMAL;
		break;
	case VPP_DISPLAY_MODE_FULL:
		break;
	case VPP_DISPLAY_MODE_NOSCALEUP: {
		if ( source_type == SOURCE_TYPE_HDMI ) {
			int retValue = GetFileAttrIntValue ( "/sys/module/tvin_hdmirx/parameters/cur_colorspace" );
			switch ( retValue ) {
			case RGB_FORMAT:
				mVpp.Tv_SetColorSpaceMode ( VPP_COLOR_SPACE_RGB );
				cutwin.vs = 0;
				cutwin.hs = 0;
				cutwin.ve = 0;
				cutwin.he = 0;
				break;

			default:
				mVpp.Tv_SetColorSpaceMode ( VPP_COLOR_SPACE_YUV );
				break;
			}
		} else {
			video_screen_mode  = CAv::VIDEO_WIDEOPTION_NORMAL_NOSCALEUP;
		}
		break;
	}
	case VPP_DISPLAY_MODE_FULL_REAL:
		video_screen_mode = CAv::VIDEO_WIDEOPTION_16_9;    //added for N360 by haifeng.liu
		break;
	case VPP_DISPLAY_MODE_PERSON:
		if ( display_resolution == VPP_DISPLAY_RESOLUTION_1366X768 ) {
			win_pos.y1 = -45;
			win_pos.y2 = win_pos.y2 - win_pos.y1;
		} else {
			win_pos.y1 = -65;
			win_pos.y2 = win_pos.y2 - win_pos.y1;
		}
		video_screen_mode = CAv::VIDEO_WIDEOPTION_FULL_STRETCH;
		break;
	case VPP_DISPLAY_MODE_MOVIE:
		if ( display_resolution == VPP_DISPLAY_RESOLUTION_1366X768 ) {
			win_pos.y1 = -61;
			win_pos.y2 = win_pos.y2 - win_pos.y1;
		} else {
			win_pos.y1 = -86;
			win_pos.y2 = win_pos.y2 - win_pos.y1;
		}
		video_screen_mode = CAv::VIDEO_WIDEOPTION_FULL_STRETCH;
		break;
	case VPP_DISPLAY_MODE_CAPTION:
		if (display_resolution == VPP_DISPLAY_RESOLUTION_1366X768 ) {
			win_pos.y1 = -61;
			win_pos.y2 = win_pos.y2 + 2;
		} else {
			win_pos.y1 = -86;
			win_pos.y2 = win_pos.y2 + 4;
		}
		video_screen_mode = CAv::VIDEO_WIDEOPTION_FULL_STRETCH;
		break;
	case VPP_DISPLAY_MODE_ZOOM:
		if (display_resolution == VPP_DISPLAY_RESOLUTION_1366X768 ) {
			win_pos.y1 = -128;
			win_pos.y2 = win_pos.y2 - win_pos.y1;
		} else {
			win_pos.y1 = -180;
			win_pos.y2 = win_pos.y2 - win_pos.y1;
		}
		video_screen_mode = CAv::VIDEO_WIDEOPTION_FULL_STRETCH;
		break;
	default:
		break;
	}

	if ( display_mode == VPP_DISPLAY_MODE_FULL ) {
		if ( source_type == SOURCE_TYPE_HDMI ) {
			if ( sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_60HZ
					//|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080I_60HZ
					|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_50HZ
					//|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_A
					//|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_B
					|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_24HZ
					|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_25HZ
					|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_30HZ
					//|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080I_100HZ
					//|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080I_120HZ
					|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_100HZ
					|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_120HZ
					//|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_FRAME_PACKING
					|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_24HZ_FRAME_PACKING
					|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_30HZ_FRAME_PACKING
					//|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080I_60HZ_ALTERNATIVE
					//|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_ALTERNATIVE
					|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_24HZ_ALTERNATIVE
					|| sig_fmt == TVIN_SIG_FMT_HDMI_1920X1080P_30HZ_ALTERNATIVE
					|| sig_fmt == TVIN_SIG_FMT_HDMI_3840_2160_00HZ
					|| sig_fmt == TVIN_SIG_FMT_HDMI_4096_2160_00HZ ) {
				const char *config_value = config_get_str ( "TV", "vpp.hdmi.nonlinear.scaling", "null" );
				if ( strcmp ( config_value, "enable" ) == 0 ) {
					video_screen_mode = CAv::VIDEO_WIDEOPTION_NONLINEAR;
					mVpp.VPP_SetNonLinearFactor ( 20 );
				}
			} else {
				video_screen_mode = CAv::VIDEO_WIDEOPTION_NONLINEAR;
				mVpp.VPP_SetNonLinearFactor ( 20 );
			}
			cutwin.vs = 0;
			cutwin.hs =  1;
			cutwin.ve = 0;
			cutwin.he =  1;
		}
	}

	if ( m_mode_3d == VIDEO_3D_MODE_DISABLE ) {
		if(display_mode == VPP_DISPLAY_MODE_FULL_REAL && source_type == SOURCE_TYPE_HDMI) {
			//added for N360 by haifeng.liu
			cutwin.vs = 0;
			cutwin.hs = 0;
			cutwin.ve = 0;
			cutwin.he = 0;
		}
	} else {
		cutwin.vs = 0;
		cutwin.hs = 0;
		cutwin.ve = 0;
		cutwin.he = 0;
		video_screen_mode = CAv::VIDEO_WIDEOPTION_FULL_STRETCH;
	}

	mAv.setVideoAxis ( win_pos.x1, win_pos.y1, win_pos.x2, win_pos.y2 );
	mAv.setVideoScreenMode(video_screen_mode);
	mVpp.VPP_SetVideoCrop(cutwin.vs, cutwin.hs, cutwin.ve, cutwin.he);
	return 0;
}

vpp_display_mode_t CTv::Tv_GetDisplayMode ( tv_source_input_type_t source_type )
{
	return mVpp.Tv_GetDisplayMode((tv_source_input_type_t)source_type);
}

int CTv::Tv_SaveDisplayMode ( vpp_display_mode_t mode, tv_source_input_type_t source_type )
{
	return SSMSaveDisplayMode ( source_type, (int)mode );
}

int CTv::Tv_SetNoiseReductionMode ( vpp_noise_reduction_mode_t mode, tv_source_input_type_t source_type, int is_save )
{
	return mVpp.Tv_SetNoiseReductionMode((vpp_noise_reduction_mode_t)mode, (tv_source_input_type_t)source_type, mSigDetectThread.getCurSigInfo().fmt, Check2Dor3D(m_mode_3d, mSigDetectThread.getCurSigInfo().trans_fmt ), mSigDetectThread.getCurSigInfo().trans_fmt, is_save);
}

vpp_noise_reduction_mode_t CTv::Tv_GetNoiseReductionMode ( tv_source_input_type_t source_type )
{
	return mVpp.Tv_GetNoiseReductionMode((tv_source_input_type_t)source_type);
}

int CTv::Tv_SaveNoiseReductionMode ( vpp_noise_reduction_mode_t mode, tv_source_input_type_t source_type )
{
	return mVpp.Tv_SaveNoiseReductionMode((vpp_noise_reduction_mode_t)mode, (tv_source_input_type_t)source_type);
}

int CTv::Tv_SetRGBOGO_PostOffset(int type, int offset)
{
	return mVpp.Tv_SetRGBOGO_PostOffset(type, offset);
}

int CTv::Tv_SetRGBOGO_Gain(int type, int mode)
{
	return mVpp.Tv_SetRGBOGO_Gain(type, mode);
}

int CTv::Tv_GetRGBOGO_PostOffset ( int type )
{
	return mVpp.Tv_GetRGBOGO_PostOffset(type);
}

int CTv::Tv_GetRGBOGO_Gain ( int type )
{
	return mVpp.Tv_GetRGBOGO_Gain(type);
}

int CTv::Tv_FactorySetPQMode_Brightness ( int source_type, int pq_mode, int brightness )
{
	return mVpp.Tv_FactorySetPQMode_Brightness(source_type, pq_mode, brightness);
}

int CTv::Tv_FactoryGetPQMode_Brightness ( int source_type, int pq_mode )
{
	return mVpp.Tv_FactoryGetPQMode_Brightness(source_type, pq_mode);
}

int CTv::Tv_FactorySetPQMode_Contrast ( int source_type, int pq_mode, int contrast )
{
	return mVpp.Tv_FactorySetPQMode_Contrast(source_type, pq_mode, contrast);
}

int CTv::Tv_FactoryGetPQMode_Contrast ( int source_type, int pq_mode )
{
	return mVpp.Tv_FactoryGetPQMode_Contrast(source_type, pq_mode);
}

int CTv::Tv_FactorySetPQMode_Saturation ( int source_type, int pq_mode, int saturation )
{
	return mVpp.Tv_FactorySetPQMode_Saturation(source_type, pq_mode, saturation);
}

int CTv::Tv_FactoryGetPQMode_Saturation ( int source_type, int pq_mode )
{
	return mVpp.Tv_FactoryGetPQMode_Saturation(source_type, pq_mode);
}

int CTv::Tv_FactorySetPQMode_Hue ( int source_type, int pq_mode, int hue )
{
	return mVpp.Tv_FactorySetPQMode_Hue(source_type, pq_mode, hue);
}

int CTv::Tv_FactoryGetPQMode_Hue ( int source_type, int pq_mode )
{
	return mVpp.Tv_FactoryGetPQMode_Hue(source_type, pq_mode);
}

int CTv::Tv_FactorySetPQMode_Sharpness ( int source_type, int pq_mode, int sharpness )
{
	return mVpp.Tv_FactorySetPQMode_Sharpness(source_type, pq_mode, sharpness);
}

int CTv::Tv_FactoryGetPQMode_Sharpness ( int source_type, int pq_mode )
{
	return mVpp.Tv_FactoryGetPQMode_Sharpness(source_type, pq_mode);
}

int CTv::Tv_FactorySetColorTemp_Rgain ( int source_type, int colortemp_mode, int rgain )
{
	return mVpp.Tv_FactorySetColorTemp_Rgain(source_type, colortemp_mode, rgain);
}

int CTv::Tv_FactorySaveColorTemp_Rgain ( int source_type, int colortemp_mode, int rgain )
{
	return mVpp.Tv_FactorySaveColorTemp_Rgain(source_type, colortemp_mode, rgain);
}

int CTv::Tv_FactoryGetColorTemp_Rgain ( int source_type, int colortemp_mode )
{
	return mVpp.Tv_FactoryGetColorTemp_Rgain(source_type, colortemp_mode);
}

int CTv::Tv_FactorySetColorTemp_Ggain ( int source_type, int colortemp_mode, int ggain )
{
	return mVpp.Tv_FactorySetColorTemp_Ggain(source_type, colortemp_mode, ggain);
}

int CTv::Tv_FactorySaveColorTemp_Ggain ( int source_type, int colortemp_mode, int ggain )
{
	return mVpp.Tv_FactorySaveColorTemp_Ggain(source_type, colortemp_mode, ggain);
}

int CTv::Tv_FactoryGetColorTemp_Ggain ( int source_type, int colortemp_mode )
{
	return mVpp.Tv_FactoryGetColorTemp_Ggain(source_type, colortemp_mode);
}

int CTv::Tv_FactorySetColorTemp_Bgain ( int source_type, int colortemp_mode, int bgain )
{
	return mVpp.Tv_FactorySetColorTemp_Bgain(source_type, colortemp_mode, bgain);
}

int CTv::Tv_FactorySaveColorTemp_Bgain ( int source_type, int colortemp_mode, int bgain )
{
	return mVpp.Tv_FactorySaveColorTemp_Bgain(source_type, colortemp_mode, bgain);
}

int CTv::Tv_FactoryGetColorTemp_Bgain ( int source_type, int colortemp_mode )
{
	return mVpp.Tv_FactoryGetColorTemp_Bgain(source_type, colortemp_mode);
}

int CTv::Tv_FactorySetColorTemp_Roffset ( int source_type, int colortemp_mode, int roffset )
{
	return mVpp.Tv_FactorySetColorTemp_Roffset(source_type, colortemp_mode, roffset);
}

int CTv::Tv_FactorySaveColorTemp_Roffset ( int source_type, int colortemp_mode, int roffset )
{
	return mVpp.Tv_FactorySaveColorTemp_Roffset(source_type, colortemp_mode, roffset);
}

int CTv::Tv_FactoryGetColorTemp_Roffset ( int source_type, int colortemp_mode )
{
	return mVpp.Tv_FactoryGetColorTemp_Roffset(source_type, colortemp_mode);
}

int CTv::Tv_FactorySetColorTemp_Goffset ( int source_type, int colortemp_mode, int goffset )
{
	return mVpp.Tv_FactorySetColorTemp_Goffset(source_type, colortemp_mode, goffset);
}

int CTv::Tv_FactorySaveColorTemp_Goffset ( int source_type, int colortemp_mode, int goffset )
{
	return mVpp.Tv_FactorySaveColorTemp_Goffset(source_type, colortemp_mode, goffset);
}

int CTv::Tv_FactoryGetColorTemp_Goffset ( int source_type, int colortemp_mode )
{
	return mVpp.Tv_FactoryGetColorTemp_Goffset(source_type, colortemp_mode);
}

int CTv::Tv_FactorySetColorTemp_Boffset ( int source_type, int colortemp_mode, int boffset )
{
	return mVpp.Tv_FactorySetColorTemp_Boffset(source_type, colortemp_mode, boffset);
}

int CTv::Tv_FactorySaveColorTemp_Boffset ( int source_type, int colortemp_mode, int boffset )
{
	return mVpp.Tv_FactorySaveColorTemp_Boffset(source_type, colortemp_mode, boffset);
}

int CTv::Tv_FactoryGetColorTemp_Boffset ( int source_type, int colortemp_mode )
{
	return mVpp.Tv_FactoryGetColorTemp_Boffset(source_type, colortemp_mode);
}

int CTv::GetColorTemperatureParams ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params )
{
	return mVpp.GetColorTemperatureParams(Tempmode, params);
}

int CTv::Tv_FactorySetTestPattern ( int pattern )
{
	switch ( pattern ) {
	case VPP_TEST_PATTERN_NONE:
		mAv.SetVideoScreenColor ( 3, 16, 128, 128 );
		break;

	case VPP_TEST_PATTERN_RED:
		mAv.SetVideoScreenColor ( 0, 81, 90, 240 );
		break;

	case VPP_TEST_PATTERN_GREEN:
		mAv.SetVideoScreenColor ( 0, 145, 54, 34 );
		break;

	case VPP_TEST_PATTERN_BLUE:
		mAv.SetVideoScreenColor ( 0, 41, 240, 110 );
		break;

	case VPP_TEST_PATTERN_WHITE:
		mAv.SetVideoScreenColor ( 0, 235, 128, 128 );
		break;

	case VPP_TEST_PATTERN_BLACK:
		mAv.SetVideoScreenColor ( 0, 16, 128, 128 );
		break;

	default:
		return -1;
	}
	return SSMSaveTestPattern ( pattern );
}

int CTv::Tv_FactoryGetTestPattern ( void )
{
	return mVpp.Tv_FactoryGetTestPattern();
}

int CTv::Tv_FactoryResetPQMode ( void )
{
	return mVpp.Tv_FactoryResetPQMode();
}

int CTv::Tv_FactoryResetColorTemp ( void )
{
	return mVpp.Tv_FactoryResetColorTemp();
}

int CTv::Tv_FactorySetParamsDefault ( void )
{
	return mVpp.Tv_FactorySetParamsDefault();
}

int CTv::Tv_FactorySetDDRSSC ( int step )
{
	return mVpp.Tv_FactorySetDDRSSC(step);
}

int CTv::Tv_FactoryGetDDRSSC ( void )
{
	return mVpp.Tv_FactoryGetDDRSSC();
}

int CTv::Tv_FactorySetLVDSSSC ( int step )
{
	return mVpp.Tv_FactorySetLVDSSSC(step);
}

int CTv::Tv_FactoryGetLVDSSSC ( void )
{
	return mVpp.Tv_FactoryGetLVDSSSC();
}

int CTv::Tv_FactorySetNolineParams ( int noline_params_type, int source_type, noline_params_t noline_params )
{
	return mVpp.Tv_FactorySetNolineParams(noline_params_type, source_type, noline_params);
}

noline_params_t CTv::Tv_FactoryGetNolineParams ( int noline_params_type, int source_type )
{
	return mVpp.Tv_FactoryGetNolineParams(noline_params_type, source_type);
}

int CTv::Tv_FactorySetOverscan ( int source_type, int fmt, int status_3d, int trans_fmt, tvin_cutwin_t cutwin_t )
{
	//tvin_cutwin_t cutwin_t = mVpp.Tv_FactoryGetOverscan(source_type, fmt, status_3d, trans_fmt);
	return mVpp.VPP_SetVideoCrop ( ( int ) cutwin_t.vs, ( int ) cutwin_t.hs, ( int ) cutwin_t.ve, ( int ) cutwin_t.he );
}

tvin_cutwin_t CTv::Tv_FactoryGetOverscan ( int source_type, int fmt, int status_3d, int trans_fmt )
{
	return mVpp.Tv_FactoryGetOverscan(source_type, fmt, Check2Dor3D(m_mode_3d, (tvin_trans_fmt_t)trans_fmt), trans_fmt);
}

int CTv::Tv_FactorySet_FBC_Brightness ( int value )
{
	//int temp_value = (255*value)/100;
	int temp_value = value;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_Brightness(COMM_DEV_SERIAL, temp_value);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_Brightness  ( void )
{
	int temp_value = 0;
	int data = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_Brightness(COMM_DEV_SERIAL, &temp_value);
		//data = (temp_value*100)/255;
		data = temp_value;

		return data;
	}

	return 0;
}

int CTv::Tv_FactorySet_FBC_Contrast ( int value )
{
	//int temp_value = (255*value)/100;
	int temp_value = value;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_Contrast(COMM_DEV_SERIAL, temp_value);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_Contrast  ( void )
{
	int temp_value = 0;
	int data = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_Contrast(COMM_DEV_SERIAL, &temp_value);
		//data = (temp_value*100)/255;
		data = temp_value;

		return data;
	}

	return 0;
}

int CTv::Tv_FactorySet_FBC_Saturation ( int value )
{
	//int temp_value = (255*value)/100;
	int temp_value = value;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_Saturation(COMM_DEV_SERIAL, temp_value);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_Saturation  ( void )
{
	int temp_value = 0;
	int data = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_Saturation(COMM_DEV_SERIAL, &temp_value);
		//data = (temp_value*100)/255;
		data = temp_value;

		return data;
	}

	return 0;
}

int CTv::Tv_FactorySet_FBC_HueColorTint ( int value )
{
	//int temp_value = (255*value)/100;
	int temp_value = value;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_HueColorTint(COMM_DEV_SERIAL, temp_value);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_HueColorTint ( void )
{
	int temp_value = 0;
	int data = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_HueColorTint(COMM_DEV_SERIAL, &temp_value);
		//data = (temp_value*100)/255;
		data = temp_value;

		return data;
	}

	return 0;
}

int CTv::Tv_FactorySet_FBC_Backlight ( int value )
{
	int temp_value = value;
	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_Backlight(COMM_DEV_SERIAL, temp_value);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_Backlight ( void )
{
	int temp_value = 0;
	int data = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_Backlight(COMM_DEV_SERIAL, &temp_value);
		data = temp_value;

		return data;
	}

	return 0;
}

int CTv::Tv_FactorySet_FBC_Backlight_N360 ( int value )
{
	int val = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_AUTO_ELEC_MODE(COMM_DEV_SERIAL, value);
		val = value;
		SSMSaveFBCN360BackLightVal(val);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_Backlight_N360 ( void )
{
	int temp_value = 0;
	int val = 0;

	SSMReadFBCN360BackLightVal(&val);

	return val;

}


int CTv::Tv_FactorySet_FBC_ELEC_MODE( int value )
{
	int val = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_AUTO_ELEC_MODE(COMM_DEV_SERIAL, value);
		val = value;
		SSMSaveFBCELECmodeVal(val);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_ELEC_MODE( void )
{
	int val = 0;

	SSMReadFBCELECmodeVal(&val);

	return val;
}

int CTv::Tv_FactorySet_FBC_Thermal_State( int value )
{
	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_Thermal_state(COMM_DEV_SERIAL, value);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactorySet_FBC_Picture_Mode ( int mode )
{
	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_Picture_Mode(COMM_DEV_SERIAL, mode);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_Picture_Mode ( void )
{
	int mode = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_Picture_Mode(COMM_DEV_SERIAL, &mode);
		return mode;
	}

	return 0;
}

int CTv::Tv_FactorySet_FBC_Set_Test_Pattern ( int mode )
{
	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_Test_Pattern(COMM_DEV_SERIAL, mode);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_Get_Test_Pattern ( void )
{
	int mode = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_Test_Pattern(COMM_DEV_SERIAL, &mode);
		return mode;
	}

	return 0;
}


int CTv::Tv_FactorySet_FBC_Gain_Red( int value )
{
	int temp_value = 0;

	//temp_value = (value*255)/2047;
	//value 0 ~ 2047
	temp_value = value;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_Gain_Red(COMM_DEV_SERIAL, temp_value);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_Gain_Red ( void )
{
	int temp_value = 0, value = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_Gain_Red(COMM_DEV_SERIAL, &temp_value);
		//value 0 ~ 2047
		//value = (temp_value*2047)/255;
		value = temp_value;

		return value;
	}

	return 0;
}

int CTv::Tv_FactorySet_FBC_Gain_Green( int value )
{
	int temp_value = 0;

	//temp_value = (value*255)/2047;
	//value 0 ~ 2047
	temp_value = value;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_Gain_Green(COMM_DEV_SERIAL, temp_value);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_Gain_Green ( void )
{
	int temp_value = 0, value = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_Gain_Green(COMM_DEV_SERIAL, &temp_value);
		//value 0 ~ 2047
		//value = (temp_value*2047)/255;
		value = temp_value;

		return value;
	}

	return 0;
}

int CTv::Tv_FactorySet_FBC_Gain_Blue( int value )
{
	int temp_value = 0;

	//temp_value = (value*255)/2047;
	//value 0 ~ 2047
	temp_value = value;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_Gain_Blue(COMM_DEV_SERIAL, temp_value);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_Gain_Blue ( void )
{
	int temp_value = 0, value = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_Gain_Blue(COMM_DEV_SERIAL, &temp_value);
		//value 0 ~ 2047
		//value = (temp_value*2047)/255;
		value = temp_value;

		return value;
	}

	return 0;
}

int CTv::Tv_FactorySet_FBC_Offset_Red( int value )
{
	//value -1024~+1023
	int temp_value = 0;

	//temp_value = (value+1024)*255/2047;
	temp_value = value;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_Offset_Red(COMM_DEV_SERIAL, temp_value);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_Offset_Red ( void )
{
	int temp_value = 0, value = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_Offset_Red(COMM_DEV_SERIAL, &temp_value);
		//value -1024~+1023
		//value = (temp_value*2047)/255 - 1024;
		value = temp_value;

		return value;
	}

	return 0;
}

int CTv::Tv_FactorySet_FBC_Offset_Green( int value )
{
	//value -1024~+1023
	int temp_value = 0;

	//temp_value = (value+1024)*255/2047;
	temp_value = value;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_Offset_Green(COMM_DEV_SERIAL, temp_value);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_Offset_Green ( void )
{
	int temp_value = 0, value = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_Offset_Green(COMM_DEV_SERIAL, &temp_value);
		//value -1024~+1023
		//value = (temp_value*2047)/255 - 1024;
		value = temp_value;

		return value;
	}

	return 0;
}

int CTv::Tv_FactorySet_FBC_Offset_Blue( int value )
{
	//value -1024~+1023
	int temp_value = 0;

	//temp_value = (value+1024)*255/2047;
	temp_value = value;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_Offset_Blue(COMM_DEV_SERIAL, value);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_Offset_Blue ( void )
{
	int temp_value = 0, value = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_Offset_Blue(COMM_DEV_SERIAL, &temp_value);
		//value -1024~+1023
		//value = (temp_value*2047)/255 - 1024;
		value = temp_value;

		return value;
	}

	return 0;
}

int CTv::Tv_FactoryGetWhiteBalanceRedGain(int source_type, int colortemp_mode)
{
	int useFbc = 0;
	int ret = -1;
	useFbc = GetPlatformHaveFBCFlag();
	if (useFbc == 0) { // not use fbc store the white balance params
		LOGD("--------- call none fbc method ---------");
		ret = mVpp.Tv_FactoryGetColorTemp_Rgain(source_type, colortemp_mode);
	} else { //use fbc store the white balance params
		LOGD("--------- call fbc method ---------");
		colortemp_mode = Tv_FactoryWhiteBalanceColorTempMappingG92Fbc(colortemp_mode);
		ret = Tv_FactoryGetItemFromBatch((vpp_color_temperature_mode_t)colortemp_mode, 0);
	}
	return ret;
}

int CTv::Tv_FactoryGetWhiteBalanceGreenGain(int source_type, int colortemp_mode)
{
	int useFbc = 0;
	int ret = -1;
	useFbc = GetPlatformHaveFBCFlag();
	if (useFbc == 0) { // not use fbc store the white balance params
		ret = mVpp.Tv_FactoryGetColorTemp_Ggain(source_type, colortemp_mode);
	} else { //use fbc store the white balance params
		colortemp_mode = Tv_FactoryWhiteBalanceColorTempMappingG92Fbc(colortemp_mode);
		ret = Tv_FactoryGetItemFromBatch((vpp_color_temperature_mode_t)colortemp_mode, 1);
	}
	return ret;
}

int CTv::Tv_FactoryGetWhiteBalanceBlueGain(int source_type, int colortemp_mode)
{
	int useFbc = 0;
	int ret = -1;
	useFbc = GetPlatformHaveFBCFlag();
	if (useFbc == 0) { // not use fbc store the white balance params
		ret = mVpp.Tv_FactoryGetColorTemp_Bgain(source_type, colortemp_mode);
	} else { //use fbc store the white balance params
		colortemp_mode = Tv_FactoryWhiteBalanceColorTempMappingG92Fbc(colortemp_mode);
		ret = Tv_FactoryGetItemFromBatch((vpp_color_temperature_mode_t)colortemp_mode, 2);
	}
	return ret;
}

int CTv::Tv_FactoryGetWhiteBalanceRedOffset(int source_type, int colortemp_mode)
{
	int useFbc = 0;
	int ret = -1;
	useFbc = GetPlatformHaveFBCFlag();
	if (useFbc == 0) { // not use fbc store the white balance params
		ret = mVpp.Tv_FactoryGetColorTemp_Roffset(source_type, colortemp_mode);
	} else { //use fbc store the white balance params
		colortemp_mode = Tv_FactoryWhiteBalanceColorTempMappingG92Fbc(colortemp_mode);
		ret = Tv_FactoryGetItemFromBatch((vpp_color_temperature_mode_t)colortemp_mode, 3);
	}
	return ret;
}

int CTv::Tv_FactoryGetWhiteBalanceGreenOffset(int source_type, int colortemp_mode)
{
	int useFbc = 0;
	int ret = -1;
	useFbc = GetPlatformHaveFBCFlag();
	if (useFbc == 0) { // not use fbc store the white balance params
		ret = mVpp.Tv_FactoryGetColorTemp_Goffset(source_type, colortemp_mode);
	} else { //use fbc store the white balance params
		colortemp_mode = Tv_FactoryWhiteBalanceColorTempMappingG92Fbc(colortemp_mode);
		ret = Tv_FactoryGetItemFromBatch((vpp_color_temperature_mode_t)colortemp_mode, 4);
	}
	return ret;
}

int CTv::Tv_FactoryGetWhiteBalanceBlueOffset(int source_type, int colortemp_mode)
{
	int useFbc = 0;
	int ret = -1;
	useFbc = GetPlatformHaveFBCFlag();
	if (useFbc == 0) { // not use fbc store the white balance params
		ret = mVpp.Tv_FactoryGetColorTemp_Boffset(source_type, colortemp_mode);
	} else { //use fbc store the white balance params
		colortemp_mode = Tv_FactoryWhiteBalanceColorTempMappingG92Fbc(colortemp_mode);
		ret = Tv_FactoryGetItemFromBatch((vpp_color_temperature_mode_t)colortemp_mode, 5);
	}
	return ret;
}

int CTv::Tv_FactorySetWhiteBalanceRedGain(int source_type, int colortemp_mode, int value)
{
	int useFbc = 0;
	int ret = -1;
	useFbc = GetPlatformHaveFBCFlag();
	if (value < 0) {
		value = 0;
	} else if (value > 2047) {
		value = 2047;
	}
	if (useFbc == 0) { // not use fbc store the white balance params
		ret = mVpp.Tv_FactorySetColorTemp_Rgain(source_type, colortemp_mode, value);
		if (ret != -1) {
			LOGD("save the red gain to flash")
			ret = mVpp.Tv_FactorySaveColorTemp_Rgain(source_type, colortemp_mode, value);
		}
	} else { //use fbc store the white balance params
		value = Tv_FactoryWhiteBalanceFormatInputFbcGainParams(value);
		ret = Tv_FactorySet_FBC_Gain_Red(value);
	}
	return ret;
}

int CTv::Tv_FactorySetWhiteBalanceGreenGain(int source_type, int colortemp_mode, int value)
{
	int useFbc = 0;
	int ret = -1;
	useFbc = GetPlatformHaveFBCFlag();
	if (value < 0) {
		value = 0;
	} else if (value > 2047) {
		value = 2047;
	}
	if (useFbc == 0) { // not use fbc store the white balance params
		ret = mVpp.Tv_FactorySetColorTemp_Ggain(source_type, colortemp_mode, value);
		if (ret != -1) {
			LOGD("save the green gain to flash")
			ret = mVpp.Tv_FactorySaveColorTemp_Ggain(source_type, colortemp_mode, value);
		}
	} else { //use fbc store the white balance params
		value = Tv_FactoryWhiteBalanceFormatInputFbcGainParams(value);
		ret = Tv_FactorySet_FBC_Gain_Green(value);
	}
	return ret;
}

int CTv::Tv_FactorySetWhiteBalanceBlueGain(int source_type, int colortemp_mode, int value)
{
	int useFbc = 0;
	int ret = -1;
	useFbc = GetPlatformHaveFBCFlag();
	if (value < 0) {
		value = 0;
	} else if (value > 2047) {
		value = 2047;
	}
	if (useFbc == 0) { // not use fbc store the white balance params
		ret = mVpp.Tv_FactorySetColorTemp_Bgain(source_type, colortemp_mode, value);
		if (ret != -1) {
			LOGD("save the blue gain to flash")
			ret = mVpp.Tv_FactorySaveColorTemp_Bgain(source_type, colortemp_mode, value);
		}
	} else { //use fbc store the white balance params
		value = Tv_FactoryWhiteBalanceFormatInputFbcGainParams(value);
		ret = Tv_FactorySet_FBC_Gain_Blue(value);
	}
	return ret;
}

int CTv::Tv_FactorySetWhiteBalanceRedOffset(int source_type, int colortemp_mode, int value)
{
	int useFbc = 0;
	int ret = -1;
	if (value < -1024) {
		value = -1024;
	} else if (value > 1023) {
		value = 1023;
	}
	useFbc = GetPlatformHaveFBCFlag();
	if (useFbc == 0) { // not use fbc store the white balance params
		ret = mVpp.Tv_FactorySetColorTemp_Roffset(source_type, colortemp_mode, value);
		if (ret != -1) {
			LOGD("save the red offset to flash")
			ret = mVpp.Tv_FactorySaveColorTemp_Roffset(source_type, colortemp_mode, value);
		}
	} else { //use fbc store the white balance params
		value = Tv_FactoryWhiteBalanceFormatInputFbcOffsetParams(value);
		ret = Tv_FactorySet_FBC_Offset_Red(value);
	}
	return ret;
}

int CTv::Tv_FactorySetWhiteBalanceGreenOffset(int source_type, int colortemp_mode, int value)
{
	int useFbc = 0;
	int ret = -1;
	if (value < -1024) {
		value = -1024;
	} else if (value > 1023) {
		value = 1023;
	}
	useFbc = GetPlatformHaveFBCFlag();
	if (useFbc == 0) { // not use fbc store the white balance params
		ret = mVpp.Tv_FactorySetColorTemp_Goffset(source_type, colortemp_mode, value);
		if (ret != -1) {
			LOGD("save the green offset to flash")
			ret = mVpp.Tv_FactorySaveColorTemp_Goffset(source_type, colortemp_mode, value);
		}
	} else { //use fbc store the white balance params
		value = Tv_FactoryWhiteBalanceFormatInputFbcOffsetParams(value);
		ret = Tv_FactorySet_FBC_Offset_Green(value);
	}
	return ret;
}

int CTv::Tv_FactorySetWhiteBalanceBlueOffset(int source_type, int colortemp_mode, int value)
{
	int useFbc = 0;
	int ret = -1;
	useFbc = GetPlatformHaveFBCFlag();
	if (value < -1024) {
		value = -1024;
	} else if (value > 1023) {
		value = 1023;
	}
	if (useFbc == 0) { // not use fbc store the white balance params
		ret = mVpp.Tv_FactorySetColorTemp_Boffset(source_type, colortemp_mode, value);
		if (ret != -1) {
			LOGD("save the blue offset to flash")
			ret = mVpp.Tv_FactorySaveColorTemp_Boffset(source_type, colortemp_mode, value);
		}
	} else { //use fbc store the white balance params
		value = Tv_FactoryWhiteBalanceFormatInputFbcOffsetParams(value);
		ret = Tv_FactorySet_FBC_Offset_Blue(value);
	}
	return ret;
}


int CTv::Tv_FactorySetWhiteBalanceColorTempMode(int source_type, int colortemp_mode, int is_save)
{
	int useFbc = 0;
	int ret = -1;
	int colorTemp = 0;
	useFbc = GetPlatformHaveFBCFlag();
	if (useFbc == 0) { // not use fbc store the white balance params
		ret = mVpp.Tv_SetColorTemperature((vpp_color_temperature_mode_t)colortemp_mode, (tv_source_input_type_t)source_type, is_save);
	} else { //use fbc store the white balance params
		colorTemp = Tv_FactoryWhiteBalanceColorTempMappingG92Fbc(colortemp_mode);
		ret = Tv_FactorySet_FBC_ColorTemp_Mode(colorTemp);
	}
	return ret;
}

int CTv::Tv_FactoryGetWhiteBalanceColorTempMode(int source_type )
{
	int useFbc = 0;
	int ret = -1;
	useFbc = GetPlatformHaveFBCFlag();
	if (useFbc == 0) { // not use fbc store the white balance params
		ret = mVpp.Tv_GetColorTemperature((tv_source_input_type_t)source_type);
	} else { //use fbc store the white balance params
		ret = Tv_FactoryGet_FBC_ColorTemp_Mode();
		ret = Tv_FactoryWhiteBalanceColorTempMappingFbc2G9(ret);
	}
	return ret;
}

int CTv::Tv_FactoryWhiteBalanceFormatInputFbcGainParams(int value)
{
	int ret = 1024;
	if (value < 0) {
		ret = 0;
	} else if (value > 2047) {
		ret = 2047;
	} else {
		ret = value;
	}
	ret = ret >> 3;
	return ret;
}

int CTv::Tv_FactoryWhiteBalanceFormatInputFbcOffsetParams(int value)
{
	int ret = 0;
	if (value < -1024) {
		ret = -1024;
	} else if (value > 1023) {
		ret = 1023;
	} else {
		ret = value;
	}
	ret += 1024;
	ret = ret >> 3;
	return ret;
}

int CTv::Tv_FactoryWhiteBalanceFormatOutputFbcOffsetParams(int value)
{
	if (value == 255) {
		value = 1023;
	} else {
		value = value << 3;
		value -= 1024;
	}
	return value;
}

int CTv::Tv_FactoryWhiteBalanceFormatOutputFbcGainParams(int value)
{
	value = value << 3;
	if (value < 0) {
		value = 0;
	} else if (value > 2047) {
		value = 2047;
	}
	return value;
}


int CTv::Tv_FactorySaveWhiteBalancePramas(int source_type, int tempmode, int r_gain, int g_gain, int b_gain, int r_offset, int g_offset, int b_offset)
{
	int useFbc = 0;
	int ret = 0;
	useFbc = GetPlatformHaveFBCFlag();
	if (useFbc == 0) { // not use fbc store the white balance params
		mVpp.Tv_SaveColorTemp((vpp_color_temperature_mode_t) tempmode, (tv_source_input_type_t) source_type);
		mVpp.Tv_FactorySaveColorTemp_Rgain(source_type, tempmode, r_gain);
		mVpp.Tv_FactorySaveColorTemp_Ggain(source_type, tempmode, g_gain);
		mVpp.Tv_FactorySaveColorTemp_Bgain(source_type, tempmode, b_gain);
		mVpp.Tv_FactorySaveColorTemp_Roffset(source_type, tempmode, r_offset);
		mVpp.Tv_FactorySaveColorTemp_Goffset(source_type, tempmode, g_offset);
		mVpp.Tv_FactorySaveColorTemp_Boffset(source_type, tempmode, b_offset);
	} else { //use fbc store the white balance params
		tcon_rgb_ogo_t params;

		params.r_gain = Tv_FactoryWhiteBalanceFormatInputFbcGainParams(r_gain);
		params.g_gain = Tv_FactoryWhiteBalanceFormatInputFbcGainParams(g_gain);
		params.b_gain = Tv_FactoryWhiteBalanceFormatInputFbcGainParams(b_gain);
		params.r_post_offset = Tv_FactoryWhiteBalanceFormatInputFbcOffsetParams(r_offset);
		params.g_post_offset = Tv_FactoryWhiteBalanceFormatInputFbcOffsetParams(g_offset);
		params.b_post_offset = Tv_FactoryWhiteBalanceFormatInputFbcOffsetParams(b_offset);
		tempmode = Tv_FactoryWhiteBalanceColorTempMappingG92Fbc(tempmode);
		ret = Tv_FactorySet_FBC_ColorTemp_Batch((vpp_color_temperature_mode_t)tempmode, params);
	}
	return ret;
}


int CTv::Tv_FactoryCloseWhiteBalanceGrayPattern()
{
	int useFbc = 0;
	int ret = -1;
	useFbc = GetPlatformHaveFBCFlag();
	if (useFbc == 0) { // not use fbc store the white balance params
		ret = mVpp.VPP_SetGrayPattern(0);
	} else { //use fbc store the white balance params
		ret = Tv_FactoryClose_FBC_GrayPattern();
	}
	return ret;
}

int CTv::Tv_FactoryOpenWhiteBalanceGrayPattern()
{
	int useFbc = 0;
	int ret = -1;
	useFbc = GetPlatformHaveFBCFlag();
	if (useFbc == 0) { // not use fbc store the white balance params
		ret = 0;
	} else { //use fbc store the white balance params
		ret = Tv_FactoryOpen_FBC_GrayPattern();
	}
	return ret;
}

int CTv::Tv_FactorySetWhiteBalanceGrayPattern(int value)
{
	int useFbc = 0;
	int ret = -1;
	useFbc = GetPlatformHaveFBCFlag();
	if (useFbc == 0) {
		ret = mVpp.VPP_SetGrayPattern(value);
	} else {
		ret = Tv_FactorySet_FBC_GrayPattern(value);
	}
	return ret;
}

int CTv:: Tv_FactoryGetWhiteBalanceGrayPattern()
{
	int useFbc = 0;
	int ret = -1;
	useFbc = GetPlatformHaveFBCFlag();
	if (useFbc == 0) {
		ret = mVpp.VPP_GetGrayPattern();
	} else {

	}
	return ret;
}


/**
* The color temperature enum order is diffrent bettewn G9 and Fbc, so we have to make a mapping
**/
int CTv::Tv_FactoryWhiteBalanceColorTempMappingG92Fbc(int Tempmode)
{
	int ret = Tempmode;
	switch (Tempmode) {
	case 0:     //standard
		ret = 1;
		break;
	case 1:     //warm
		ret = 2;
		break;
	case 2:    //cold
		ret = 0;
		break;
	default:
		break;
	}
	return ret;
}

/**
* The color temperature enum order is diffrent bettewn G9 and Fbc, so we have to make a mapping
**/
int CTv::Tv_FactoryWhiteBalanceColorTempMappingFbc2G9(int Tempmode)
{
	int ret = Tempmode;
	switch (Tempmode) {
	case 0:     //cold
		ret = 2;
		break;
	case 1:     //standard
		ret = 0;
		break;
	case 2:    //warm
		ret = 1;
		break;
	default:
		break;
	}
	return ret;
}

int CTv::Tv_FactoryWhiteBalanceGetAllParams(int Tempmode, tcon_rgb_ogo_t *params)
{
	int ret = -1;
	Tempmode = Tv_FactoryWhiteBalanceColorTempMappingG92Fbc(Tempmode);
	ret = Tv_FactoryGet_FBC_ColorTemp_Batch((vpp_color_temperature_mode_t)Tempmode, params);
	params->r_gain = Tv_FactoryWhiteBalanceFormatOutputFbcGainParams(params->r_gain);
	params->g_gain = Tv_FactoryWhiteBalanceFormatOutputFbcGainParams(params->g_gain);
	params->b_gain = Tv_FactoryWhiteBalanceFormatOutputFbcGainParams(params->b_gain);
	params->r_post_offset = Tv_FactoryWhiteBalanceFormatOutputFbcOffsetParams(params->r_post_offset);
	params->g_post_offset = Tv_FactoryWhiteBalanceFormatOutputFbcOffsetParams(params->g_post_offset);
	params->b_post_offset = Tv_FactoryWhiteBalanceFormatOutputFbcOffsetParams(params->b_post_offset);
	return ret;
}

int CTv::Tv_FactorySet_FBC_GrayPattern(int value)
{
	int ret = -1;
	unsigned char grayValue = 0;
	if (value > 255) {
		grayValue = 255;
	} else if (value < 0) {
		grayValue = 0;
	} else {
		grayValue = (unsigned char)(0xFF & value);
	}
	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		ret = fbcIns->cfbc_WhiteBalance_SetGrayPattern(COMM_DEV_SERIAL, grayValue);
	}
	return ret;
}

int CTv::Tv_FactoryOpen_FBC_GrayPattern()
{
	int ret = -1;
	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		ret = fbcIns->cfbc_WhiteBalance_GrayPattern_OnOff(COMM_DEV_SERIAL, 0);
	}
	return ret;
}

int CTv::Tv_FactoryClose_FBC_GrayPattern()
{
	int ret = -1;
	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		ret = fbcIns->cfbc_WhiteBalance_GrayPattern_OnOff(COMM_DEV_SERIAL, 1);
	}
	return ret;
}

int CTv::Tv_FactorySet_FBC_ColorTemp_Mode( int mode )
{
	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_ColorTemp_Mode(COMM_DEV_SERIAL, mode);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_ColorTemp_Mode ( void )
{
	int temp_mode = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_ColorTemp_Mode(COMM_DEV_SERIAL, &temp_mode);
		return temp_mode;
	}

	return 0;
}
int CTv::Tv_FactorySet_FBC_ColorTemp_Mode_N360( int mode )
{
	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_ColorTemp_Mode(COMM_DEV_SERIAL, mode);
		SSMSaveFBCN360ColorTempVal(mode);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_ColorTemp_Mode_N360 ( void )
{
	int temp_mode = 0;

	SSMReadFBCN360ColorTempVal(&temp_mode);

	return temp_mode;
}

int CTv::Tv_FactorySet_FBC_WB_Initial( int status )
{
	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_WB_Initial(COMM_DEV_SERIAL, status);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_WB_Initial ( void )
{
	int temp_status = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_WB_Initial(COMM_DEV_SERIAL, &temp_status);
		return temp_status;
	}

	return 0;
}

int CTv::Tv_FactorySet_FBC_ColorTemp_Batch(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params)
{
	unsigned char mode = 0, r_gain, g_gain, b_gain, r_offset, g_offset, b_offset;
	switch(Tempmode) {
	case VPP_COLOR_TEMPERATURE_MODE_STANDARD:
		mode = 1;   //COLOR_TEMP_STD
		break;
	case VPP_COLOR_TEMPERATURE_MODE_WARM:
		mode = 2;   //COLOR_TEMP_WARM
		break;
	case VPP_COLOR_TEMPERATURE_MODE_COLD:
		mode = 3;  //COLOR_TEMP_COLD
		break;
	case VPP_COLOR_TEMPERATURE_MODE_USER:
		mode = 3;   //COLOR_TEMP_USER
		break;
	}
	r_gain = (params.r_gain * 255) / 2047; // u1.10, range 0~2047, default is 1024 (1.0x)
	g_gain = (params.g_gain * 255) / 2047;
	b_gain = (params.b_gain * 255) / 2047;
	r_offset = (params.r_post_offset + 1024) * 255 / 2047; // s11.0, range -1024~+1023, default is 0
	g_offset = (params.g_post_offset + 1024) * 255 / 2047;
	b_offset = (params.b_post_offset + 1024) * 255 / 2047;
	LOGD ( "~Tv_FactorySet_FBC_ColorTemp_Batch##%d,%d,%d,%d,%d,%d,##", r_gain, g_gain, b_gain, r_offset, g_offset, b_offset );

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_WB_Batch(COMM_DEV_SERIAL, mode, r_gain, g_gain, b_gain, r_offset, g_offset, b_offset);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_ColorTemp_Batch ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params )
{
	unsigned char mode = 0, r_gain, g_gain, b_gain, r_offset, g_offset, b_offset;
	switch(Tempmode) {
	case VPP_COLOR_TEMPERATURE_MODE_STANDARD:
		mode = 1;   //COLOR_TEMP_STD
		break;
	case VPP_COLOR_TEMPERATURE_MODE_WARM:
		mode = 2;   //COLOR_TEMP_WARM
		break;
	case VPP_COLOR_TEMPERATURE_MODE_COLD:
		mode = 3;  //COLOR_TEMP_COLD
		break;
	case VPP_COLOR_TEMPERATURE_MODE_USER:
		mode = 3;   //COLOR_TEMP_USER
		break;
	}

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_WB_Batch(COMM_DEV_SERIAL, mode, &r_gain, &g_gain, &b_gain, &r_offset, &g_offset, &b_offset);
		LOGD ( "~Tv_FactoryGet_FBC_ColorTemp_Batch##%d,%d,%d,%d,%d,%d,##", r_gain, g_gain, b_gain, r_offset, g_offset, b_offset );

		params->r_gain = (r_gain * 2047) / 255;
		params->g_gain = (g_gain * 2047) / 255;
		params->b_gain = (b_gain * 2047) / 255;
		params->r_post_offset = (r_offset * 2047) / 255 - 1024;
		params->g_post_offset = (g_offset * 2047) / 255 - 1024;
		params->b_post_offset = (b_offset * 2047) / 255 - 1024;

		return 0;
	}

	return -1;
}

int CTv::Tv_FactorySet_WB_G9_To_FBC( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params )
{
	Tv_FactorySet_FBC_ColorTemp_Batch(Tempmode, params);

	return 0;
}

int CTv::Tv_FactoryGet_WB_G9_To_FBC ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params )
{
	int temp_status = 0;

	Tv_FactoryGet_FBC_ColorTemp_Batch(Tempmode, params);

	return temp_status;
}

int CTv::Tv_FactoryGetItemFromBatch(vpp_color_temperature_mode_t colortemp_mode, int item)
{
	tcon_rgb_ogo_t params;
	int ret = 0;

	Tv_FactoryGet_FBC_ColorTemp_Batch((vpp_color_temperature_mode_t)colortemp_mode, &params);
	switch(item) {
	case 0:
		ret = params.r_gain;
		ret = Tv_FactoryWhiteBalanceFormatOutputFbcGainParams(ret);
		break;
	case 1:
		ret = params.g_gain;
		ret = Tv_FactoryWhiteBalanceFormatOutputFbcGainParams(ret);
		break;
	case 2:
		ret = params.b_gain;
		ret = Tv_FactoryWhiteBalanceFormatOutputFbcGainParams(ret);
		break;
	case 3:
		ret = params.r_post_offset;
		ret = Tv_FactoryWhiteBalanceFormatOutputFbcOffsetParams(ret);
		break;
	case 4:
		ret = params.g_post_offset;
		ret = Tv_FactoryWhiteBalanceFormatOutputFbcOffsetParams(ret);
		break;
	case 5:
		ret = params.b_post_offset;
		ret = Tv_FactoryWhiteBalanceFormatOutputFbcOffsetParams(ret);
		break;
	default:
		ret = 0;
	}
	return ret;

}


int CTv::Tv_FactorySet_FBC_CM_OnOff( unsigned char status )
{
	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_CM(COMM_DEV_SERIAL, status);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_CM_OnOff (void)
{
	int temp_status = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_CM(COMM_DEV_SERIAL, &temp_status);
		return temp_status;
	}

	return 0;
}

int CTv::Tv_FactorySet_FBC_DNLP_OnOff( unsigned char status )
{
	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_DNLP(COMM_DEV_SERIAL, status);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_DNLP_OnOff (void)
{
	int temp_status = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_DNLP(COMM_DEV_SERIAL, &temp_status);
		return temp_status;
	}

	return 0;
}

int CTv::Tv_FactorySet_FBC_Gamma_OnOff( unsigned char status )
{
	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_Gamma(COMM_DEV_SERIAL, status);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_Gamma_OnOff (void)
{
	int temp_status = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_Gamma(COMM_DEV_SERIAL, &temp_status);
		return temp_status;
	}

	return 0;
}

int CTv::Tv_FactorySet_FBC_WhiteBalance_OnOff( unsigned char status )
{
	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_WhiteBalance_OnOff(COMM_DEV_SERIAL, status);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_WhiteBalance_OnOff (void)
{
	int temp_status = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_WhiteBalance_OnOff(COMM_DEV_SERIAL, &temp_status);
		return temp_status;
	}

	return 0;
}


int CTv::Tv_FactorySet_FBC_backlight_onoff ( int value )
{
	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_backlight_onoff(COMM_DEV_SERIAL, value);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_backlight_onoff ( void )
{
	int temp_value = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_backlight_onoff(COMM_DEV_SERIAL, &temp_value);
		return temp_value;
	}

	return 0;
}

int CTv::Tv_FactorySet_FBC_LVDS_SSG_Set( int value )
{
	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_LVDS_SSG_Set(COMM_DEV_SERIAL, value);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactorySet_FBC_LightSensor_Status_N310 ( int value )
{
	int temp_value = value;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_LightSensor_N310(COMM_DEV_SERIAL, temp_value);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_LightSensor_Status_N310 ( void )
{
	int temp_value = 0;
	int data = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_LightSensor_N310(COMM_DEV_SERIAL, &temp_value);

		data = temp_value;

		return data;
	}

	return 0;
}
int CTv::Tv_FactorySet_FBC_Dream_Panel_Status_N310 ( int value )
{
	int temp_value = value;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_Dream_Panel_N310(COMM_DEV_SERIAL, temp_value);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_Dream_Panel_Status_N310 ( void )
{
	int temp_value = 0;
	int data = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_Dream_Panel_N310(COMM_DEV_SERIAL, &temp_value);

		data = temp_value;

		return data;
	}

	return 0;
}

int CTv::Tv_FactorySet_FBC_MULT_PQ_Status_N310 ( int value )
{
	int temp_value = value;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_MULT_PQ_N310(COMM_DEV_SERIAL, temp_value);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_MULT_PQ_Status_N310 ( void )
{
	int temp_value = 0;
	int data = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_MULT_PQ_N310(COMM_DEV_SERIAL, &temp_value);

		data = temp_value;

		return data;
	}

	return 0;
}
int CTv::Tv_FactorySet_FBC_MEMC_Status_N310 ( int value )
{
	int temp_value = value;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Set_MEMC_N310(COMM_DEV_SERIAL, temp_value);
		return 0;
	}

	return -1;
}

int CTv::Tv_FactoryGet_FBC_MEMC_Status_N310 ( void )
{
	int temp_value = 0;
	int data = 0;

	CFbcCommunication *fbcIns = GetSingletonFBC();
	if (fbcIns != NULL) {
		fbcIns->cfbc_Get_MEMC_N310(COMM_DEV_SERIAL, &temp_value);

		data = temp_value;

		return data;
	}

	return 0;
}



//audio
int CTv::SetAudioMuteForSystem(int muteOrUnmute)
{
	return mTvAudio.SetAudioMuteForSystem(muteOrUnmute);
}

int CTv::GetAudioMuteForSystem()
{
	return mTvAudio.GetAudioMuteForSystem();
}

int CTv::SetAudioAVOutMute(int muteStatus)
{
	return mTvAudio.SetAudioAVOutMute(muteStatus);
}

int CTv::GetAudioAVOutMute()
{
	return mTvAudio.GetAudioAVOutMute();
}

int CTv::SetAudioSPDIFMute(int muteStatus)
{
	return mTvAudio.SetAudioSPDIFMute(muteStatus);
}

int CTv::GetAudioSPDIFMute()
{
	return mTvAudio.GetAudioSPDIFMute();
}
int CTv::SetAudioMasterVolume(int tmp_vol)
{
	return mTvAudio.SetAudioMasterVolume(tmp_vol);
}

int CTv::GetAudioMasterVolume()
{
	return mTvAudio.GetAudioMasterVolume();
}

int CTv::SaveCurAudioMasterVolume(int tmp_vol)
{
	return mTvAudio.SaveCurAudioMasterVolume(tmp_vol);
}

int CTv::GetCurAudioMasterVolume()
{
	return mTvAudio.GetCurAudioMasterVolume();
}

int CTv::SetAudioBalance(int tmp_val)
{
	return mTvAudio.SetAudioBalance(tmp_val);
}

int CTv::GetAudioBalance()
{
	return mTvAudio.GetAudioBalance();
}

int CTv::SaveCurAudioBalance(int tmp_val)
{
	return mTvAudio.SaveCurAudioBalance(tmp_val);
}

int CTv::GetCurAudioBalance()
{
	return mTvAudio.GetCurAudioBalance();
}


int CTv::SetAudioSupperBassVolume(int tmp_vol)
{
	return mTvAudio.SetAudioSupperBassVolume(tmp_vol);
}

int CTv::GetAudioSupperBassVolume()
{
	return mTvAudio.GetAudioSupperBassVolume();
}

int CTv::SaveCurAudioSupperBassVolume(int tmp_vol)
{
	return mTvAudio.SaveCurAudioSupperBassVolume(tmp_vol);
}

int CTv::GetCurAudioSupperBassVolume()
{
	return mTvAudio.GetCurAudioSupperBassVolume();
}
int CTv::SetAudioSupperBassSwitch(int tmp_val)
{
	return mTvAudio.SetAudioSupperBassSwitch(tmp_val);
}

int CTv::GetAudioSupperBassSwitch()
{
	return mTvAudio.GetAudioSupperBassSwitch();
}

int CTv::SaveCurAudioSupperBassSwitch(int tmp_val)
{
	return mTvAudio.SaveCurAudioSupperBassSwitch(tmp_val);
}

int CTv::GetCurAudioSupperBassSwitch()
{
	return mTvAudio.GetCurAudioSupperBassSwitch();
}

int CTv::SetAudioSRSSurround(int tmp_val)
{
	return mTvAudio.SetAudioSRSSurround(tmp_val);
}

int CTv::GetAudioSRSSurround()
{
	return mTvAudio.GetAudioSRSSurround();
}

int CTv::SaveCurAudioSrsSurround(int tmp_val)
{
	return mTvAudio.SaveCurAudioSrsSurround(tmp_val);
}

int CTv::GetCurAudioSRSSurround()
{
	return mTvAudio.GetCurAudioSRSSurround();
}

int CTv::SetAudioSrsDialogClarity(int tmp_val)
{
	return mTvAudio.SetAudioSrsDialogClarity(tmp_val);
}

int CTv::GetAudioSrsDialogClarity()
{
	return mTvAudio.GetAudioSrsDialogClarity();
}

int CTv::SaveCurAudioSrsDialogClarity(int tmp_val)
{
	return mTvAudio.SaveCurAudioSrsDialogClarity(tmp_val);
}

int CTv::GetCurAudioSrsDialogClarity()
{
	return mTvAudio.GetCurAudioSrsDialogClarity();
}

int CTv::SetAudioSrsTruBass(int tmp_val)
{
	return mTvAudio.SetAudioSrsTruBass(tmp_val);
}

int CTv::GetAudioSrsTruBass()
{
	return mTvAudio.GetAudioSrsTruBass();
}

int CTv::SaveCurAudioSrsTruBass(int tmp_val)
{
	return mTvAudio.SaveCurAudioSrsTruBass(tmp_val);
}

int CTv::GetCurAudioSrsTruBass()
{
	return mTvAudio.GetCurAudioSrsTruBass();
}

int CTv::SetAudioBassVolume(int tmp_vol)
{
	return mTvAudio.SetAudioBassVolume(tmp_vol);
}

int CTv::GetAudioBassVolume()
{
	return mTvAudio.GetAudioBassVolume();
}

int CTv::SaveCurAudioBassVolume(int tmp_vol)
{
	return mTvAudio.SaveCurAudioBassVolume(tmp_vol);
}

int CTv::GetCurAudioBassVolume()
{
	return mTvAudio.GetCurAudioBassVolume();
}

int CTv::SetAudioTrebleVolume(int tmp_vol)
{
	return mTvAudio.SetAudioTrebleVolume(tmp_vol);
}

int CTv::GetAudioTrebleVolume()
{
	return mTvAudio.GetAudioTrebleVolume();
}

int CTv::SaveCurAudioTrebleVolume(int tmp_vol)
{
	return mTvAudio.SaveCurAudioTrebleVolume(tmp_vol);
}

int CTv::GetCurAudioTrebleVolume()
{
	return mTvAudio.GetCurAudioTrebleVolume();
}

int CTv::SetAudioSoundMode(int tmp_val)
{
	return mTvAudio.SetAudioSoundMode(tmp_val);
}

int CTv::GetAudioSoundMode()
{
	return mTvAudio.GetAudioSoundMode();
}

int CTv::SaveCurAudioSoundMode(int tmp_val)
{
	return mTvAudio.SaveCurAudioSoundMode(tmp_val);
}

int CTv::GetCurAudioSoundMode()
{
	return mTvAudio.GetCurAudioSoundMode();
}

int CTv::SetAudioWallEffect(int tmp_val)
{
	return mTvAudio.SetAudioWallEffect(tmp_val);
}

int CTv::GetAudioWallEffect()
{
	return mTvAudio.GetAudioWallEffect();
}

int CTv::SaveCurAudioWallEffect(int tmp_val)
{
	return mTvAudio.SaveCurAudioWallEffect(tmp_val);
}

int CTv::GetCurAudioWallEffect()
{
	return mTvAudio.GetCurAudioWallEffect();
}


int CTv::SetAudioEQMode(int tmp_val)
{
	return mTvAudio.SetAudioEQMode(tmp_val);
}

int CTv::GetAudioEQMode()
{
	return mTvAudio.GetAudioEQMode();
}

int CTv::SaveCurAudioEQMode(int tmp_val)
{
	return mTvAudio.SaveCurAudioEQMode(tmp_val);
}

int CTv::GetCurAudioEQMode()
{
	return mTvAudio.GetCurAudioEQMode();
}

int CTv::GetAudioEQRange(int range_buf[])
{
	return mTvAudio.GetAudioEQRange(range_buf);
}

int CTv::SetAudioEQGain(int gain_buf[])
{
	return mTvAudio.SetAudioEQGain(gain_buf);
}

int CTv::GetAudioEQGain(int gain_buf[])
{
	return mTvAudio.GetAudioEQGain(gain_buf);
}

int CTv::SaveCurAudioEQGain(int gain_buf[])
{
	return mTvAudio.SaveCurAudioEQGain(gain_buf);
}

int CTv::GetCurAudioEQGain(int gain_buf[])
{
	return mTvAudio.GetCurAudioEQGain(gain_buf);
}

int CTv::GetAudioEQBandCount()
{
	return mTvAudio.GetAudioEQBandCount();
}



int CTv::SetAudioEQSwitch(int switch_val)
{
	return mTvAudio.SetAudioEQSwitch(switch_val);
}

int CTv::SetAudioSPDIFSwitch(int tmp_val)
{
	return mTvAudio.SetAudioSPDIFSwitch(tmp_val);
}

/*showboz           public int GetAudioSPDIFSwitch() {
        mTvAudio.GetAudioSPDIFSwitch();
}*/

int CTv::amAudioSetDoubleOutputSwitch(int en_val)
{
	return mTvAudio.amAudioSetDoubleOutputSwitch(en_val);
}
int CTv::UsbAudioOuputModuleSwitch(int sw)
{
	return mTvAudio.UsbAudioOuputModuleSwitch(sw);
}

int CTv::GetAmAudioSetDoubleOutputSwitch()
{
	return mTvAudio.GetAmAudioSetDoubleOutputSwitch();
}

int CTv::GetCurUsbAudioOuputModuleSwitch()
{
	return mTvAudio.GetCurUsbAudioOuputModuleSwitch();
}

int CTv::SaveCurAudioSPDIFSwitch(int tmp_val)
{
	return mTvAudio.SaveCurAudioSPDIFSwitch(tmp_val);
}

int CTv::GetCurAudioSPDIFSwitch()
{
	return mTvAudio.GetCurAudioSPDIFSwitch();
}
int CTv::SetAudioSPDIFMode(int tmp_val)
{
	return mTvAudio.SetAudioSPDIFMode(tmp_val);
}

/*    public int GetAudioSPDIFMode() {
mTvAudio.GetAudioSPDIFMode();
showboz             */


int CTv::SaveCurAudioSPDIFMode(int tmp_val)
{
	return mTvAudio.SaveCurAudioSPDIFMode(tmp_val);
}

int CTv::GetCurAudioSPDIFMode()
{
	return mTvAudio.GetCurAudioSPDIFMode();
}

int CTv::OpenAmAudio(unsigned int sr, int output_device)
{
	return mTvAudio.OpenAmAudio(sr, output_device);
}

int CTv::CloseAmAudio(void)
{
	return mTvAudio.CloseAmAudio();
}

int CTv::SetAmAudioInputSr(unsigned int sr, int output_device)
{
	return mTvAudio.SetAmAudioInputSr(sr, output_device);
}

int CTv::SetAmAudioOutputMode(int mode)
{
	return mTvAudio.SetAmAudioOutputMode(mode);
}

int CTv::SetAmAudioMusicGain(int gain)
{
	return mTvAudio.SetAmAudioMusicGain(gain);
}

int CTv::SetAmAudioLeftGain(int gain)
{
	return mTvAudio.SetAmAudioLeftGain(gain);
}


int CTv::SetAmAudioRightGain(int gain)
{
	return mTvAudio.SetAmAudioRightGain(gain);
}

int CTv::AudioLineInSelectChannel(int audio_channel)
{
	return mTvAudio.AudioLineInSelectChannel(audio_channel);
}

int CTv::SetKalaokIO(int level)
{
	return mTvAudio.SetKalaokIO(level);
}
int CTv::AudioSetAudioInSource(int audio_src_in_type)
{
	return mTvAudio.AudioSetAudioInSource(CC_AUDIO_IN_SOURCE_LINEIN);
}

int CTv::AudioSetLineInCaptureVolume(int l_vol, int r_vol)
{
	return mTvAudio.AudioSetLineInCaptureVolume(l_vol, r_vol);
}

int CTv::AudioHandleHeadsetPlugIn()
{
	return mTvAudio.AudioHandleHeadsetPlugIn();
}

int CTv::AudioHandleHeadsetPullOut()
{
	return mTvAudio.AudioHandleHeadsetPullOut();
}
