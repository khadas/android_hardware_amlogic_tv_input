#include "CTvSkyworthDtmbN310.h"
#include <cutils/properties.h>

CTvSkyworthDtmbN310::CTvSkyworthDtmbN310()
{
    LOGD("CTvSkyworthDtmbN310");
    if (CTvDatabase::GetTvDb()->isAtv256ProgInsertForSkyworth() == false) {
        CTvDatabase::GetTvDb()->insert256AtvProgForSkyworth();
        LOGD("Atv has not 256 program,insert 256 blank program to db\n");
    }
}

CTvSkyworthDtmbN310::~CTvSkyworthDtmbN310()
{
}

//searchType 0:not 256 1:is 256 Program
int CTvSkyworthDtmbN310::atvAutoScan(int videoStd, int audioStd, int searchType)
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
        Vector<sp <CTvChannel> > v_channel;
        CTvChannel::getChannelListBySrc(CTvChannel::MODE_ANALOG, v_channel);
        int size = v_channel.size();
        LOGD("showbo                          --size = %d", size);
        CTvChannel::CleanAllChannelBySrc (CTvChannel::MODE_ANALOG);
        CTvDatabase::GetTvDb()->beginTransaction();
        for (int i = 0; i < size; i++) {
            CTvProgram::deleteChannelsProgram(*v_channel[i]);
        }
        CTvDatabase::GetTvDb()->commitTransaction();
        if (CTvDatabase::GetTvDb()->isAtv256ProgInsertForSkyworth() == false) {
            CTvDatabase::GetTvDb()->insert256AtvProgForSkyworth();
        }
    }
    minScanFreq = mFrontDev.formatATVFreq ( minScanFreq );
    maxScanFreq = mFrontDev.formatATVFreq ( maxScanFreq );
    LOGD("%s, atv auto scan vstd=%d, astd=%d stdandcolor=%lld", __FUNCTION__, vStd, aStd, stdAndColor);
    mSigDetectThread.setVdinNoSigCheckKeepTimes(1000, false);
    mFrontDev.Open(FE_ANALOG);
    mTvScanner.autoAtvScan ( minScanFreq, maxScanFreq, stdAndColor, searchType );
    return 0;
}

int CTvSkyworthDtmbN310::OpenTv ( void )
{
    CTv::OpenTv();
    StartTvLock();
    //fast start
    setSourceSwitchAndPlay();
    return 0;
}

int CTvSkyworthDtmbN310::StartTvLock ()
{
    CTv::StartTvLock();
    return 0;
}

