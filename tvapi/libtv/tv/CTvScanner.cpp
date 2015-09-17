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
#include "CTvScanner.h"
#include "CTvProgram.h"
#include "CTvRegion.h"
#include "CFrontEnd.h"
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "CTvScanner"
#endif

CTvScanner *CTvScanner::m_s_Scanner = NULL;
//int CTvScanner::getStartPara()
//{
//
//}
CTvScanner::CTvScanner(CTvin *pTvin)
{
	mbScanStart = false;
	mpObserver = NULL;
	mSource = 0xff;
	mMinFreq = 1;
	mMaxFreq = 100;
	mCurScanStartFreq = 1;
	mCurScanEndFreq = 100;
	mpTvin = pTvin;
}

CTvScanner::~CTvScanner()
{
	AM_EVT_Unsubscribe((long)mScanHandle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
	AM_EVT_Unsubscribe((long)mScanHandle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
}


AM_Bool_t CTvScanner::atv_cvbs_lock_check(v4l2_std_id  *colorStd)
{
	tvafe_cvbs_video_t cvbs_lock_status;
	int ret, i = 0;

	*colorStd = 0;
	while (i < 20) {
		ret = mpTvin->AFE_GetCVBSLockStatus(&cvbs_lock_status);

		if (cvbs_lock_status == TVAFE_CVBS_VIDEO_HV_LOCKED)
			//||cvbs_lock_status == TVAFE_CVBS_VIDEO_V_LOCKED
			//||cvbs_lock_status == TVAFE_CVBS_VIDEO_H_LOCKED)
		{
			usleep(2000 * 1000);
			tvin_info_t info;
			mpTvin->VDIN_GetSignalInfo(&info);
			*colorStd = CTvin::CvbsFtmToV4l2ColorStd(info.fmt);
			LOGD("atv_cvbs_lock_check     locked   and cvbs fmt = 0x%x std = 0x%x", info.fmt, *colorStd);
			return true;
		}
		usleep(50 * 1000);
		i++;
	}
	return false;
}

AM_Bool_t CTvScanner::s_atv_cvbs_lock_check(void *data)
{
	if(data == NULL) return false;
	AM_SCAN_ATV_LOCK_PARA_t *pAtvPara = (AM_SCAN_ATV_LOCK_PARA_t *)data;
	CTvScanner *pScan = (CTvScanner *)(pAtvPara->pData);
	v4l2_std_id std;
	AM_Bool_t isLock = pScan->atv_cvbs_lock_check(&std);
	pAtvPara->pOutColorSTD = std;
	return isLock;
}

int CTvScanner::ATVManualScan(int min_freq, int max_freq, int std, int store_Type, int channel_num)
{
	stopScan();
	if(min_freq <= 0 || max_freq <= 0 || min_freq == max_freq) {
		LOGD("atv manual scan freq error start = %d end = %d",  min_freq,  max_freq);
		return -1;
	}
	int direction = (max_freq >= min_freq) ? 1 : 0;

	if(!mbScanStart) { //not start or stop
		//
		mCurScanStartFreq = min_freq;
		mCurScanEndFreq = max_freq;
		AM_SCAN_CreatePara_t para;
		AM_DMX_OpenPara_t dmx_para;
		AM_SCAN_Handle_t handle = 0, i;
		LOGD("startScanThread");
		// Create the scan
		memset(&para, 0, sizeof(para));
		para.fend_dev_id = 0;//default
		para.mode = AM_SCAN_MODE_ATV_DTV;//
		para.atv_para.mode = AM_SCAN_ATVMODE_MANUAL;
		para.atv_para.storeMode = store_Type;
		para.dtv_para.mode = AM_SCAN_DTVMODE_NONE;

		para.atv_para.am_scan_atv_cvbs_lock =  s_atv_cvbs_lock_check;

		para.atv_para.fe_cnt = 3;
		para.atv_para.fe_paras = static_cast<AM_FENDCTRL_DVBFrontendParameters_t *>(calloc(3, sizeof(AM_FENDCTRL_DVBFrontendParameters_t)));
		if(para.atv_para.fe_paras != NULL) {
			memset(para.atv_para.fe_paras, 0, 3 * sizeof(AM_FENDCTRL_DVBFrontendParameters_t));
			para.atv_para.fe_paras[0].m_type = FE_ANALOG;
			para.atv_para.fe_paras[0].analog.para.frequency = min_freq;
			para.atv_para.fe_paras[1].m_type = FE_ANALOG;
			para.atv_para.fe_paras[1].analog.para.frequency = max_freq;
			para.atv_para.fe_paras[2].m_type = FE_ANALOG;
			para.atv_para.fe_paras[2].analog.para.frequency = min_freq;
		}
		para.atv_para.direction = direction;
		para.atv_para.default_std = std;

		para.atv_para.channel_id = -1;
		para.atv_para.cvbs_unlocked_step = 1000000;
		para.atv_para.cvbs_locked_step = 3000000;
		para.atv_para.afc_range = 2000000;

		para.dtv_para.resort_all = AM_FALSE;
		para.dtv_para.sort_method = AM_SCAN_SORT_BY_FREQ_SRV_ID;
		para.store_cb = am_scan_atv_store;
		para.atv_para.channel_num = channel_num;

		memset(&dmx_para, 0, sizeof(dmx_para));
		AM_DMX_Open(para.dtv_para.dmx_dev_id, &dmx_para);

		if ((para.dtv_para.mode & 0x07) != AM_SCAN_DTVMODE_NONE) {
			AM_FEND_SetMode(para.fend_dev_id, para.dtv_para.source);
			tv_scan_reconnect_dmx_to_fend(para.dtv_para.dmx_dev_id, para.fend_dev_id);
		}

		LOGD("atv manual scan mode = %d", para.dtv_para.mode);
		// Start Scan
		if (AM_SCAN_Create(&para, &handle) != AM_SUCCESS) {
			LOGD("SCAN CREATE fail");
			handle = NULL;
		} else {
			mScanHandle = handle;
			AM_SCAN_SetUserData(handle, (void *)this);
			AM_EVT_Subscribe((long)handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
			AM_EVT_Subscribe((long)handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
			if (AM_SCAN_Start(handle) != AM_SUCCESS) {
				AM_SCAN_Destroy(handle, AM_FALSE);
				AM_EVT_Unsubscribe((long)handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
				AM_EVT_Unsubscribe((long)handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
				handle = NULL;
			}
		}
		if (para.atv_para.fe_paras != NULL)
			free(para.atv_para.fe_paras);
		if (para.dtv_para.fe_paras != NULL)
			free(para.dtv_para.fe_paras);

		if (handle == NULL) {
			return -1;
		}
		mbScanStart = true;//start call ok
	} else {
		LOGD("scanning and not stop");
	}

	return 0;

}

void CTvScanner::am_scan_atv_store(AM_SCAN_Result_t *result)
{
	AM_SCAN_TS_t *ts;
	int i, ret, db_sat_id = -1;
	//AM_SCAN_RecTab_t srv_tab;
	for(ts = result->tses; ts != NULL; ts = ts->p_next) {
		//
		m_s_Scanner->mCurEv.mType = ScannerEvent::EVENT_ATV_PROG_DATA;
		m_s_Scanner->mCurEv.mVideoStd = CFrontEnd::stdAndColorToVideoEnum(ts->analog.std);
		m_s_Scanner->mCurEv.mAudioStd = CFrontEnd::stdAndColorToAudioEnum(ts->analog.std);
		m_s_Scanner->mCurEv.mIsAutoStd = ((ts->analog.std  &  V4L2_COLOR_STD_AUTO) ==  V4L2_COLOR_STD_AUTO) ? 1 : 0;
		m_s_Scanner->mCurEv.mFrequency = ts->analog.freq;
		//
		m_s_Scanner->mpObserver->onEvent(m_s_Scanner->mCurEv);
	}
}

void CTvScanner::setGlobalScanerObject(CTvScanner *s)
{
	m_s_Scanner =  s;
}

int CTvScanner::autoAtvScan(int min_freq, int max_freq, int std, int search_type)
{
	stopScan();
	if(min_freq <= 0 || max_freq <= 0 || min_freq > max_freq)
		return -1;
	if(!mbScanStart) { //not start or stop
		mCurScanStartFreq = min_freq;
		mCurScanEndFreq = max_freq;

		AM_SCAN_CreatePara_t para;
		AM_DMX_OpenPara_t dmx_para;
		AM_SCAN_Handle_t handle = 0, i;
		// Create the scan
		memset(&para, 0, sizeof(para));

		para.fend_dev_id = 0;//default
		para.mode = AM_SCAN_MODE_ATV_DTV;//
		if (search_type == 1)
			para.atv_para.storeMode = 1;
		else
			para.atv_para.storeMode = 0;
		para.dtv_para.mode = AM_SCAN_DTVMODE_NONE;

		para.atv_para.am_scan_atv_cvbs_lock =  s_atv_cvbs_lock_check;

		para.atv_para.fe_cnt = 3;
		para.atv_para.fe_paras = static_cast<AM_FENDCTRL_DVBFrontendParameters_t *>(calloc(3, sizeof(AM_FENDCTRL_DVBFrontendParameters_t)));
		if(para.atv_para.fe_paras != NULL) {
			memset(para.atv_para.fe_paras, 0, 3 * sizeof(AM_FENDCTRL_DVBFrontendParameters_t));
			para.atv_para.fe_paras[0].m_type = FE_ANALOG;
			para.atv_para.fe_paras[0].analog.para.frequency = min_freq;
			para.atv_para.fe_paras[1].m_type = FE_ANALOG;
			para.atv_para.fe_paras[1].analog.para.frequency = max_freq;
			para.atv_para.fe_paras[2].m_type = FE_ANALOG;
			para.atv_para.fe_paras[2].analog.para.frequency = 0;
		}

		para.atv_para.direction = 0;
		para.atv_para.default_std = std;

		para.atv_para.channel_id = -1;
		para.atv_para.afc_unlocked_step = 3000000;
		para.atv_para.cvbs_unlocked_step = 1500000;
		para.atv_para.cvbs_locked_step = 6000000;
		para.atv_para.afc_range = 2000000;

		para.dtv_para.resort_all = AM_FALSE;
		para.dtv_para.sort_method = AM_SCAN_SORT_BY_FREQ_SRV_ID;
		para.store_cb = am_scan_atv_store;

		memset(&dmx_para, 0, sizeof(dmx_para));
		AM_DMX_Open(para.dtv_para.dmx_dev_id, &dmx_para);

		if ((para.dtv_para.mode & 0x07) != AM_SCAN_DTVMODE_NONE) {
			AM_FEND_SetMode(para.fend_dev_id, para.dtv_para.source);
			tv_scan_reconnect_dmx_to_fend(para.dtv_para.dmx_dev_id, para.fend_dev_id);
		}

		//    prog->dmx_id = para.dtv_para.dmx_dev_id;
		//    prog->fend_id = para.fend_dev_id;
		//    prog->mode = para.dtv_para.mode;
		LOGD(" mode = %d", para.dtv_para.mode);
		// Start Scan
		if (AM_SCAN_Create(&para, &handle) != AM_SUCCESS) {
			LOGD("SCAN CREATE fail");
			handle = NULL;
		} else {
			mScanHandle = handle;
			AM_SCAN_SetUserData(handle, (void *)this);
			//注册搜索事件
			AM_EVT_Subscribe((long)handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
			//注册信号质量通知事件
			AM_EVT_Subscribe((long)handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
			if (AM_SCAN_Start(handle) != AM_SUCCESS) {
				AM_SCAN_Destroy(handle, AM_FALSE);
				AM_EVT_Unsubscribe((long)handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
				AM_EVT_Unsubscribe((long)handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
				handle = NULL;
			}
		}
		if (para.atv_para.fe_paras != NULL)
			free(para.atv_para.fe_paras);
		if (para.dtv_para.fe_paras != NULL)
			free(para.dtv_para.fe_paras);

		if (handle == NULL) {
			return -1;
		}
		mbScanStart = true;//start call ok
	} else {
		LOGD("scanning and not stop");
	}

	return 0;
}

/*int CTvScanner::manualAtscScan(int channelID,int attennaType,int std)
{
	stopScan();
    if(!mbScanStart) { //not start or stop
        AM_SCAN_CreatePara_t para;
        AM_DMX_OpenPara_t dmx_para;
        AM_SCAN_Handle_t handle = 0;
        int i;
        // Create the scan
        memset(&para, 0, sizeof(para));

        //strcpy(para.default_text_lang,"eng");//config
        //strcpy(para.text_langs, "local eng zho chi chs first");//config
        para.fend_dev_id = 0;//default
        para.mode = AM_SCAN_MODE_ADTV;//DTV
        para.atv_para.mode = AM_SCAN_ATVMODE_NONE;


		para.atv_para.default_std= std;


        para.dtv_para.mode = AM_SCAN_DTVMODE_MANUAL;//DTV_MODE_ALLBAND
        //para.dtv_para.mode |= (*env)->GetIntField(env, para, doptions);//忽略
        para.dtv_para.source = FE_ATSC;//fe_type
        para.dtv_para.dmx_dev_id = 0;//default 0
        para.dtv_para.resort_all = AM_TRUE;
		para.dtv_para.clear_source = AM_FALSE;
		para.dtv_para.mix_tv_radio = AM_FALSE;
		para.dtv_para.sort_method = AM_SCAN_SORT_BY_FREQ_SRV_ID;
        para.store_cb = NULL;
		//para.dtv_para.attenna = attennaType;

		para.dtv_para.standard = AM_SCAN_DTV_STD_ATSC;

		mSource = para.dtv_para.source;

        Vector<sp<CTvChannel> > vcp;
        //CTvDatabase::getChannelParaList("/data/tv_default.xml", vcp);//channel list from xml or db
		getAtscChannelPara(attennaType,vcp);
		LOGD("ANDESEN test attenna type is %d",attennaType);

        int size = vcp.size();
        LOGD("channel list size = %d", size);
        //showboz
        if(size == 0) {
            CTvDatabase::GetTvDb()->importXmlToDB("/etc/tv_default.xml");
            getAtscChannelPara(attennaType,vcp);
            size = vcp.size();
        }

        if(!(para.dtv_para.fe_paras = static_cast<AM_FENDCTRL_DVBFrontendParameters_t*>(calloc(1, sizeof(AM_FENDCTRL_DVBFrontendParameters_t)))))
            return -1;

        //memset(pfp, 0, size * sizeof(AM_FENDCTRL_DVBFrontendParameters_t));

        for (i = 0; i < size; i++) {
			if(channelID == vcp[i]->getLogicalChannelNum())
			{
            	para.dtv_para.fe_paras[0].m_type = 3;//MODE_ATSC
            	para.dtv_para.fe_paras[0].m_logicalChannelNum = vcp[i]->getLogicalChannelNum();
            	para.dtv_para.fe_paras[0].atsc.para.frequency = vcp[i]->getFrequency();
            	para.dtv_para.fe_paras[0].atsc.para.u.vsb.modulation = (fe_modulation_t)(vcp[i]->getModulation());
				LOGD("andesen test atsc manual type=%d,num=%d,freq=%d,modu=%d",
                para.dtv_para.fe_paras[0].m_type,para.dtv_para.fe_paras[0].m_logicalChannelNum,
                para.dtv_para.fe_paras[0].atsc.para.frequency,para.dtv_para.fe_paras[0].atsc.para.u.vsb.modulation);
			}
        }
        //allband
        para.dtv_para.fe_cnt = 1;
        memset(&dmx_para, 0, sizeof(dmx_para));
        AM_DMX_Open(para.dtv_para.dmx_dev_id, &dmx_para);

        if ((para.dtv_para.mode&0x07) != AM_SCAN_DTVMODE_NONE) {
            AM_FEND_SetMode(para.fend_dev_id, para.dtv_para.source);
            tv_scan_reconnect_dmx_to_fend(para.dtv_para.dmx_dev_id, para.fend_dev_id);
        }

        //    prog->dmx_id = para.dtv_para.dmx_dev_id;
        //    prog->fend_id = para.fend_dev_id;
        //    prog->mode = para.dtv_para.mode;
        // Start Scan
        if (AM_SCAN_Create(&para, &handle) != AM_SUCCESS) {
            LOGD("SCAN CREATE fail");
            handle = 0;
        } else {
            mScanHandle = handle;
            AM_SCAN_SetUserData(handle, (void*)this);
            //注册搜索事件
            AM_EVT_Subscribe(handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
            //注册信号质量通知事件
            AM_EVT_Subscribe(handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
            if (AM_SCAN_Start(handle) != AM_SUCCESS) {
                AM_SCAN_Destroy(handle, AM_FALSE);
                AM_EVT_Unsubscribe(handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
                AM_EVT_Unsubscribe(handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
                handle = 0;
            }
        }
        if (para.atv_para.fe_paras != NULL)
            free(para.atv_para.fe_paras);
        if (para.dtv_para.fe_paras != NULL)
            free(para.dtv_para.fe_paras);

        if (handle == 0) {
            return -1;
        }
        mbScanStart = true;//start call ok
    } else {
        LOGD("scanning and not stop");
    }	return 0;
}*/

int CTvScanner::manualDtmbScan(int beginFreq, int endFreq, int modulation)
{
	stopScan();

	if(!mbScanStart) { //not start or stop
		AM_SCAN_CreatePara_t para;
		AM_DMX_OpenPara_t dmx_para;
		AM_SCAN_Handle_t handle = 0;
		int i;
		// Create the scan
		memset(&para, 0, sizeof(para));

		para.fend_dev_id = 0;//default
		para.mode = AM_SCAN_MODE_DTV_ATV;//DTV
		para.atv_para.mode = AM_SCAN_ATVMODE_NONE;
		para.dtv_para.mode = AM_SCAN_DTVMODE_MANUAL;
		para.dtv_para.source = FE_DTMB;//fe_type
		para.dtv_para.dmx_dev_id = 0;//default 0

		Vector<sp<CTvChannel> > vcp;
		CTvRegion::getChannelListByNameAndFreqRange("CHINA,Default DTMB ALL", beginFreq, endFreq, vcp);
		int size = vcp.size();

		//@author:hao.fu
		if(size == 0) {
			return -1;
		}

		if(!(para.dtv_para.fe_paras = static_cast<AM_FENDCTRL_DVBFrontendParameters_t *>(calloc(size, sizeof(AM_FENDCTRL_DVBFrontendParameters_t)))))
			return -1;

		for (i = 0; i < size; i++) {
			para.dtv_para.fe_paras[i].m_type = 5;//MODE_DTMB
			para.dtv_para.fe_paras[i].dtmb.para.frequency = vcp[i]->getFrequency();
			para.dtv_para.fe_paras[i].dtmb.para.inversion = INVERSION_OFF;
			para.dtv_para.fe_paras[i].dtmb.para.u.ofdm.bandwidth = (fe_bandwidth_t)(vcp[i]->getBandwidth());
			if (modulation == -1)
				para.dtv_para.fe_paras[i].cable.para.u.qam.modulation = (fe_modulation_t)(vcp[i]->getModulation());
			else
				para.dtv_para.fe_paras[i].cable.para.u.qam.modulation = (fe_modulation_t)modulation;
		}

		para.dtv_para.fe_cnt = size;
		para.dtv_para.resort_all = AM_FALSE;
		para.dtv_para.sort_method = AM_SCAN_SORT_BY_FREQ_SRV_ID;
		para.store_cb = NULL;


		memset(&dmx_para, 0, sizeof(dmx_para));
		AM_DMX_Open(para.dtv_para.dmx_dev_id, &dmx_para);

		if ((para.dtv_para.mode & 0x07) != AM_SCAN_DTVMODE_NONE) {
			AM_FEND_SetMode(para.fend_dev_id, para.dtv_para.source);
			tv_scan_reconnect_dmx_to_fend(para.dtv_para.dmx_dev_id, para.fend_dev_id);
		}

		// Start Scan
		if (AM_SCAN_Create(&para, &handle) != AM_SUCCESS) {
			LOGD("SCAN CREATE fail");
			handle = NULL;
		} else {
			mScanHandle = handle;
			AM_SCAN_SetUserData(handle, (void *)this);
			AM_EVT_Subscribe((long)handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
			AM_EVT_Subscribe((long)handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
			if (AM_SCAN_Start(handle) != AM_SUCCESS) {
				AM_SCAN_Destroy(handle, AM_FALSE);
				AM_EVT_Unsubscribe((long)handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
				AM_EVT_Unsubscribe((long)handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
				handle = NULL;
			}
		}
		if (para.atv_para.fe_paras != NULL)
			free(para.atv_para.fe_paras);
		if (para.dtv_para.fe_paras != NULL)
			free(para.dtv_para.fe_paras);

		if (handle == NULL) {
			return -1;
		}
		mbScanStart = true;//start call ok
	} else {
		LOGD("scanning and not stop");
	}
	return 0;
}


/*int CTvScanner::autoTvScan()
{
    stopScan();

    if(!mbScanStart) { //not start or stop
        AM_SCAN_CreatePara_t para;
        AM_DMX_OpenPara_t dmx_para;
        int handle = 0,i;
        // Create the scan
        memset(&para, 0, sizeof(para));

        //strcpy(para.default_text_lang,"eng");//config
        //strcpy(para.text_langs, "eng zho chi");//config
        para.fend_dev_id = 0;//default
        para.mode = AM_SCAN_MODE_DTV_ATV;//DTV
        para.atv_para.mode = AM_SCAN_ATVMODE_NONE;
        para.dtv_para.mode = AM_SCAN_DTVMODE_ALLBAND;//DTV_MODE_ALLBAND
        //para.dtv_para.mode |= (*env)->GetIntField(env, para, doptions);//忽略
        para.dtv_para.source = 1;//QAM
        para.dtv_para.dmx_dev_id = 0;//default 0
        Vector<sp<CTvChannel> > vcp;
        //CTvDatabase::getChannelParaList("/data/tv_default.xml", vcp);//channel list from xml or db
        CTvRegion::getChannelListByName("China,DVB-C allband", vcp);

        int size = vcp.size();
        LOGD("channel list size = %d", size);
        if(!(para.dtv_para.fe_paras = static_cast<AM_FENDCTRL_DVBFrontendParameters_t*>(calloc(size, sizeof(AM_FENDCTRL_DVBFrontendParameters_t)))))
            return -1;

        //memset(pfp, 0, size * sizeof(AM_FENDCTRL_DVBFrontendParameters_t));

        for (i = 0; i < size; i++) {
            para.dtv_para.fe_paras[i].m_type = FE_QAM;
            para.dtv_para.fe_paras[i].cable.para.frequency = vcp[i]->getFrequency();
            para.dtv_para.fe_paras[i].cable.para.inversion = INVERSION_OFF;
            para.dtv_para.fe_paras[i].cable.para.u.qam.symbol_rate = vcp[i]->getSymbolRate();
            para.dtv_para.fe_paras[i].cable.para.u.qam.modulation = (fe_modulation_t)(vcp[i]->getModulation());
        }
        //allband
        para.dtv_para.fe_cnt = size;

        para.dtv_para.resort_all = AM_FALSE;
        para.dtv_para.sort_method = AM_SCAN_SORT_BY_FREQ_SRV_ID;
        para.store_cb = NULL;
        memset(&dmx_para, 0, sizeof(dmx_para));
        AM_DMX_Open(para.dtv_para.dmx_dev_id, &dmx_para);

        if ((para.dtv_para.mode&0x07) != AM_SCAN_DTVMODE_NONE) {
            AM_FEND_SetMode(para.fend_dev_id, para.dtv_para.source);
            tv_scan_reconnect_dmx_to_fend(para.dtv_para.dmx_dev_id, para.fend_dev_id);
        }

        //    prog->dmx_id = para.dtv_para.dmx_dev_id;
        //    prog->fend_id = para.fend_dev_id;
        //    prog->mode = para.dtv_para.mode;
        // Start Scan
        if (AM_SCAN_Create(&para, &handle) != AM_SUCCESS) {
            LOGD("SCAN CREATE fail");
            handle = 0;
        } else {
            LOGD("SCAN CREATE handle = %d", handle);
            mScanHandle = handle;
            AM_SCAN_SetUserData(handle, (void*)this);
            //注册搜索事件
            AM_EVT_Subscribe(handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
            //注册信号质量通知事件
            //AM_EVT_Subscribe(handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
            if (AM_SCAN_Start(handle) != AM_SUCCESS) {
                AM_SCAN_Destroy(handle, AM_FALSE);
                AM_EVT_Unsubscribe(handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
                //    AM_EVT_Unsubscribe(handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
                handle = 0;
            }
        }
        if (para.atv_para.fe_paras != NULL)
            free(para.atv_para.fe_paras);
        if (para.dtv_para.fe_paras != NULL)
            free(para.dtv_para.fe_paras);

        if (handle == 0) {
            return -1;
        }
        mbScanStart = true;//start call ok
    } else {
        LOGD("scanning and not stop");
    }
    return 0;
}*/
//only test for dtv allbland auto
/*int CTvScanner::startScan()
{
    stopScan();

    if(!mbScanStart) { //not start or stop
        AM_SCAN_CreatePara_t para;
        AM_DMX_OpenPara_t dmx_para;
        int handle = 0,i;
        // Create the scan
        memset(&para, 0, sizeof(para));

        //strcpy(para.default_text_lang,"eng");//config
        //strcpy(para.text_langs, "eng zho chi");//config
        para.fend_dev_id = 0;//default
        para.mode = AM_SCAN_MODE_DTV_ATV;//DTV
        para.atv_para.mode = AM_SCAN_ATVMODE_NONE;
        para.dtv_para.mode = 3;//DTV_MODE_ALLBAND
        //para.dtv_para.mode |= (*env)->GetIntField(env, para, doptions);//忽略
        para.dtv_para.source = 1;//QAM
        para.dtv_para.dmx_dev_id = 0;//default 0
        Vector<sp<CTvChannel> > vcp;
        //CTvDatabase::getChannelParaList("/data/tv_default.xml", vcp);//channel list from xml or db
        CTvRegion::getChannelListByName("China,DVB-C allband", vcp);

        int size = vcp.size();
        LOGD("channel list size = %d", size);
        if(!(para.dtv_para.fe_paras = static_cast<AM_FENDCTRL_DVBFrontendParameters_t*>(calloc(size, sizeof(AM_FENDCTRL_DVBFrontendParameters_t)))))
            return -1;

        //memset(pfp, 0, size * sizeof(AM_FENDCTRL_DVBFrontendParameters_t));

        for (i = 0; i < size; i++) {
            para.dtv_para.fe_paras[i].m_type = FE_QAM;
            para.dtv_para.fe_paras[i].cable.para.frequency = vcp[i]->getFrequency();
            para.dtv_para.fe_paras[i].cable.para.inversion = INVERSION_OFF;
            para.dtv_para.fe_paras[i].cable.para.u.qam.symbol_rate = vcp[i]->getSymbolRate();
            para.dtv_para.fe_paras[i].cable.para.u.qam.modulation = (fe_modulation_t)(vcp[i]->getModulation());
        }
        //allband
        para.dtv_para.fe_cnt = size;

        para.dtv_para.resort_all = AM_FALSE;
        para.dtv_para.sort_method = AM_SCAN_SORT_BY_FREQ_SRV_ID;
        para.store_cb = NULL;

        memset(&dmx_para, 0, sizeof(dmx_para));
        AM_DMX_Open(para.dtv_para.dmx_dev_id, &dmx_para);

        if ((para.dtv_para.mode&0x07) != AM_SCAN_DTVMODE_NONE) {
            AM_FEND_SetMode(para.fend_dev_id, para.dtv_para.source);
            tv_scan_reconnect_dmx_to_fend(para.dtv_para.dmx_dev_id, para.fend_dev_id);
        }

        //    prog->dmx_id = para.dtv_para.dmx_dev_id;
        //    prog->fend_id = para.fend_dev_id;
        //    prog->mode = para.dtv_para.mode;
        // Start Scan
        if (AM_SCAN_Create(&para, &handle) != AM_SUCCESS) {
            LOGD("SCAN CREATE fail");
            handle = 0;
        } else {
            mScanHandle = handle;
            AM_SCAN_SetUserData(handle, (void*)this);
            //注册搜索事件
            AM_EVT_Subscribe(handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
            //注册信号质量通知事件
            //AM_EVT_Subscribe(handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
            if (AM_SCAN_Start(handle) != AM_SUCCESS) {
                AM_SCAN_Destroy(handle, AM_FALSE);
                AM_EVT_Unsubscribe(handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
                //    AM_EVT_Unsubscribe(handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
                handle = 0;
            }
        }
        if (para.atv_para.fe_paras != NULL)
            free(para.atv_para.fe_paras);
        if (para.dtv_para.fe_paras != NULL)
            free(para.dtv_para.fe_paras);

        if (handle == 0) {
            return -1;
        }
        mbScanStart = true;//start call ok
    } else {
        LOGD("scanning and not stop");
    }
    return 0;
}*/


//only test for dtv allbland auto
int CTvScanner::autoDtmbScan()
{
	stopScan();

	if(!mbScanStart) { //not start or stop
		AM_SCAN_CreatePara_t para;
		AM_DMX_OpenPara_t dmx_para;
		AM_SCAN_Handle_t handle = 0;
		// Create the scan
		memset(&para, 0, sizeof(para));

		//strcpy(para.default_text_lang,"eng");//config
		//strcpy(para.text_langs, "local eng zho chi chs first");//config
		para.fend_dev_id = 0;//default
		para.mode = AM_SCAN_MODE_DTV_ATV;//DTV
		para.atv_para.mode = AM_SCAN_ATVMODE_NONE;
		para.dtv_para.mode = AM_SCAN_DTVMODE_ALLBAND;//DTV_MODE_ALLBAND
		//para.dtv_para.mode |= (*env)->GetIntField(env, para, doptions);//忽略
		para.dtv_para.source = FE_DTMB;//fe_type
		para.dtv_para.dmx_dev_id = 0;//default 0
		Vector<sp<CTvChannel> > vcp;
		//CTvDatabase::getChannelParaList("/data/tv_default.xml", vcp);//channel list from xml or db
		CTvRegion::getChannelListByName("CHINA,Default DTMB ALL", vcp);

		int size = vcp.size();
		LOGD("channel list size = %d", size);
		//showboz
		if(size == 0) {
			CTvDatabase::GetTvDb()->importXmlToDB("/etc/tv_default.xml");
			CTvRegion::getChannelListByName("CHINA,Default DTMB ALL", vcp);
			size = vcp.size();
		}

		if(!(para.dtv_para.fe_paras = static_cast<AM_FENDCTRL_DVBFrontendParameters_t *>(calloc(size, sizeof(AM_FENDCTRL_DVBFrontendParameters_t)))))
			return -1;

		//memset(pfp, 0, size * sizeof(AM_FENDCTRL_DVBFrontendParameters_t));

		for (int i = 0; i < size; i++) {
			para.dtv_para.fe_paras[i].m_type = 5;//MODE_DTMB
			para.dtv_para.fe_paras[i].dtmb.para.frequency = vcp[i]->getFrequency();
			para.dtv_para.fe_paras[i].dtmb.para.inversion = INVERSION_OFF;
			para.dtv_para.fe_paras[i].dtmb.para.u.ofdm.bandwidth = (fe_bandwidth_t)(vcp[i]->getBandwidth());
		}
		//allband
		para.dtv_para.fe_cnt = size;

		para.dtv_para.resort_all = AM_FALSE;
		para.dtv_para.sort_method = AM_SCAN_SORT_BY_FREQ_SRV_ID;
		para.store_cb = NULL;

		memset(&dmx_para, 0, sizeof(dmx_para));
		AM_DMX_Open(para.dtv_para.dmx_dev_id, &dmx_para);

		if ((para.dtv_para.mode & 0x07) != AM_SCAN_DTVMODE_NONE) {
			AM_FEND_SetMode(para.fend_dev_id, para.dtv_para.source);
			tv_scan_reconnect_dmx_to_fend(para.dtv_para.dmx_dev_id, para.fend_dev_id);
		}

		//    prog->dmx_id = para.dtv_para.dmx_dev_id;
		//    prog->fend_id = para.fend_dev_id;
		//    prog->mode = para.dtv_para.mode;
		// Start Scan
		if (AM_SCAN_Create(&para, &handle) != AM_SUCCESS) {
			LOGD("SCAN CREATE fail");
			handle = NULL;
		} else {
			mScanHandle = handle;
			AM_SCAN_SetUserData(handle, (void *)this);
			//注册搜索事件
			AM_EVT_Subscribe((long)handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
			//注册信号质量通知事件
			AM_EVT_Subscribe((long)handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
			if (AM_SCAN_Start(handle) != AM_SUCCESS) {
				AM_SCAN_Destroy(handle, AM_FALSE);
				AM_EVT_Unsubscribe((long)handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
				AM_EVT_Unsubscribe((long)handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
				handle = NULL;
			}
		}
		if (para.atv_para.fe_paras != NULL)
			free(para.atv_para.fe_paras);
		if (para.dtv_para.fe_paras != NULL)
			free(para.dtv_para.fe_paras);

		if (handle == NULL) {
			return -1;
		}
		mbScanStart = true;//start call ok
	} else {
		LOGD("scanning and not stop");
	}
	return 0;
}

/*int CTvScanner::autoAtscScan(int attennaType,int std)
{
    stopScan();

    if(!mbScanStart) { //not start or stop
        AM_SCAN_CreatePara_t para;
        AM_DMX_OpenPara_t dmx_para;
        int handle = 0,i;
        // Create the scan
        memset(&para, 0, sizeof(para));

        //strcpy(para.default_text_lang,"eng");//config
        //strcpy(para.text_langs, "local eng zho chi chs first");//config
        para.fend_dev_id = 0;//default
        para.mode = AM_SCAN_MODE_ADTV;//DTV
        para.atv_para.mode = AM_SCAN_ATVMODE_NONE;


		para.atv_para.default_std= std;


        para.dtv_para.mode = AM_SCAN_DTVMODE_ALLBAND;//DTV_MODE_ALLBAND
        //para.dtv_para.mode |= (*env)->GetIntField(env, para, doptions);//忽略
        para.dtv_para.source = FE_ATSC;//fe_type
        para.dtv_para.dmx_dev_id = 0;//default 0
        para.dtv_para.resort_all = AM_TRUE;
		para.dtv_para.clear_source = AM_TRUE;
		para.dtv_para.mix_tv_radio = AM_FALSE;
		para.dtv_para.sort_method = AM_SCAN_SORT_BY_FREQ_SRV_ID;
        para.store_cb = NULL;
		//para.dtv_para.attenna = attennaType;//see dvb atsc-cnc

		para.dtv_para.standard = AM_SCAN_DTV_STD_ATSC;

		mSource = para.dtv_para.source;

        Vector<sp<CTvChannel> > vcp;
        //CTvDatabase::getChannelParaList("/data/tv_default.xml", vcp);//channel list from xml or db
		getAtscChannelPara(attennaType,vcp);
		LOGD("ANDESEN test attenna type is %d",attennaType);

        int size = vcp.size();
        LOGD("channel list size = %d", size);
        //showboz
        if(size == 0) {
            CTvDatabase::GetTvDb()->importXmlToDB("/etc/tv_default.xml");
            getAtscChannelPara(attennaType,vcp);
            size = vcp.size();
        }

        if(!(para.dtv_para.fe_paras = static_cast<AM_FENDCTRL_DVBFrontendParameters_t*>(calloc(size, sizeof(AM_FENDCTRL_DVBFrontendParameters_t)))))
            return -1;

        //memset(pfp, 0, size * sizeof(AM_FENDCTRL_DVBFrontendParameters_t));

        for (i = 0; i < size; i++) {
            para.dtv_para.fe_paras[i].m_type = 3;//MODE_ATSC
            para.dtv_para.fe_paras[i].m_logicalChannelNum = vcp[i]->getLogicalChannelNum();
            para.dtv_para.fe_paras[i].atsc.para.frequency = vcp[i]->getFrequency();
            para.dtv_para.fe_paras[i].atsc.para.u.vsb.modulation = (fe_modulation_t)(vcp[i]->getModulation());
        }
        //allband
        para.dtv_para.fe_cnt = size;

        memset(&dmx_para, 0, sizeof(dmx_para));
        AM_DMX_Open(para.dtv_para.dmx_dev_id, &dmx_para);

        if ((para.dtv_para.mode&0x07) != AM_SCAN_DTVMODE_NONE) {
            AM_FEND_SetMode(para.fend_dev_id, para.dtv_para.source);
            tv_scan_reconnect_dmx_to_fend(para.dtv_para.dmx_dev_id, para.fend_dev_id);
        }

        //    prog->dmx_id = para.dtv_para.dmx_dev_id;
        //    prog->fend_id = para.fend_dev_id;
        //    prog->mode = para.dtv_para.mode;
        // Start Scan
        if (AM_SCAN_Create(&para, &handle) != AM_SUCCESS) {
            LOGD("SCAN CREATE fail");
            handle = 0;
        } else {
            mScanHandle = handle;
            AM_SCAN_SetUserData(handle, (void*)this);
            //注册搜索事件
            AM_EVT_Unsubscribe(handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
            AM_EVT_Subscribe(handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
            //注册信号质量通知事件
            AM_EVT_Subscribe(handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
            if (AM_SCAN_Start(handle) != AM_SUCCESS) {
                AM_SCAN_Destroy(handle, AM_FALSE);
                AM_EVT_Unsubscribe(handle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
                AM_EVT_Unsubscribe(handle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
                handle = 0;
            }
        }
        if (para.atv_para.fe_paras != NULL)
            free(para.atv_para.fe_paras);
        if (para.dtv_para.fe_paras != NULL)
            free(para.dtv_para.fe_paras);

        if (handle == 0) {
            return -1;
        }
        mbScanStart = true;//start call ok
    } else {
        LOGD("scanning and not stop");
    }
    return 0;
}*/


int CTvScanner::stopScan()
{
	LOGD("StopScan mScanHandle=%d", mScanHandle);
	//requestExit();
	if(mbScanStart) { //if start ok and not stop
		int ret = AM_SCAN_Destroy(mScanHandle, AM_TRUE);
		AM_EVT_Unsubscribe((long)mScanHandle, AM_SCAN_EVT_PROGRESS, tv_scan_evt_callback, NULL);
		AM_EVT_Unsubscribe((long)mScanHandle, AM_SCAN_EVT_SIGNAL, tv_scan_evt_callback, NULL);
		AM_SEC_Cache_Reset(0);
		//stop loop
		LOGD("StopScan , ret=%d", ret);
		mbScanStart = false;//stop ok
	} else {
		LOGD("not start scan or scan stoped");
	}
	return 0;
}

void CTvScanner::tv_scan_evt_callback(long dev_no, int event_type, void *param, void *data)
{
	CTvScanner *pT = NULL;
	long long tmpFreq = 0;
	AM_SCAN_GetUserData((AM_SCAN_Handle_t)dev_no, (void **)&pT);
	if(pT == NULL) {
		return;
	}
	memset(pT->mCurEv.mProgramName, '\0', sizeof(pT->mCurEv.mProgramName));
	memset(pT->mCurEv.mMSG, '\0', sizeof(pT->mCurEv.mMSG));
	if (event_type == AM_SCAN_EVT_PROGRESS) {
		AM_SCAN_Progress_t *evt = (AM_SCAN_Progress_t *)param;
		switch (evt->evt) {
		case AM_SCAN_PROGRESS_SCAN_BEGIN:
			pT->mCurEv.mPercent = 0;
			break;
		case AM_SCAN_PROGRESS_NIT_BEGIN:

			break;
		case AM_SCAN_PROGRESS_NIT_END:

			break;
		case AM_SCAN_PROGRESS_TS_BEGIN: {
			AM_SCAN_TSProgress_t *tp = (AM_SCAN_TSProgress_t *)evt->data;
			if (tp == NULL)
				break;
			pT->mCurEv.mChannelNumber = tp->index;
			pT->mCurEv.mMode = tp->fend_para.m_type;
			pT->mCurEv.mFrequency = ((struct dvb_frontend_parameters *)(&tp->fend_para))->frequency;
			pT->mCurEv.mSymbolRate = tp->fend_para.cable.para.u.qam.symbol_rate;
			pT->mCurEv.mModulation = tp->fend_para.cable.para.u.qam.modulation;
			pT->mCurEv.mBandwidth = tp->fend_para.terrestrial.para.u.ofdm.bandwidth;
			pT->mCurEv.mOfdm_mode = tp->fend_para.terrestrial.para.u.ofdm.ofdm_mode;
			pT->mCurEv.mAudio = tp->fend_para.analog.para.u.analog.audmode;
			pT->mCurEv.mStandard = tp->fend_para.analog.para.u.analog.std;
			pT->mCurEv.mSat_polarisation = tp->fend_para.sat.polarisation;
			if (pT->mCurEv.mMode == FE_ANALOG) {
				if(pT->mSource == FE_ATSC) {
					pT->mCurEv.mPercent = (tp->index * 100) / tp->total;
				} else {
					pT->mCurEv.mPercent = 0;
				}
			} else {
				pT->mCurEv.mPercent = (tp->index * 100) / tp->total;
			}

			if (pT->mCurEv.mTotalChannelCount == 0)
				pT->mCurEv.mTotalChannelCount = tp->total;
			if (pT->mCurEv.mPercent >= 100)
				pT->mCurEv.mPercent = 99;

			pT->mCurEv.mLockedStatus = 0;
			pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;

			pT->mpObserver->onEvent(pT->mCurEv);
		}
		break;
		case AM_SCAN_PROGRESS_TS_END: {
			/*pT->mCurEv.mLockedStatus = 0;
			pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;
			pT->mpObserver->onEvent(pT->mCurEv);*/
		}
		break;

		case AM_SCAN_PROGRESS_PAT_DONE: /*{
                if(pT->mCurEv.mTotalChannelCount == 1) {
                    pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;
                    pT->mpObserver->onEvent(pT->mCurEv);
                }
            }*/
			break;
		case AM_SCAN_PROGRESS_SDT_DONE: /*{
                dvbpsi_sdt_t *sdts = (dvbpsi_sdt_t *)evt->data;
                dvbpsi_sdt_t *sdt;

                if (pT->mCurEv.mTotalChannelCount == 1) {
                    pT->mCurEv.mPercent += 25;
                    if (pT->mCurEv.mPercent >= 100)
                        pT->mCurEv.mPercent = 99;
                    pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;

                    pT->mpObserver->onEvent(pT->mCurEv);
                }
            }*/
			break;
		case AM_SCAN_PROGRESS_CAT_DONE: /*{
                dvbpsi_cat_t *cat = (dvbpsi_cat_t *)evt->data;
                if (pT->mCurEv.mTotalChannelCount == 1) {
                    pT->mCurEv.mPercent += 25;
                    if (pT->mCurEv.mPercent >= 100)
                        pT->mCurEv.mPercent = 99;

                    pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;

                    pT->mpObserver->onEvent(pT->mCurEv);
                }
            }*/
			break;
		case AM_SCAN_PROGRESS_PMT_DONE: /*{
                dvbpsi_pmt_t *pmt = (dvbpsi_pmt_t *)evt->data;
                if (pT->mCurEv.mTotalChannelCount == 1) {
                    pT->mCurEv.mPercent += 25;
                    if (pT->mCurEv.mPercent >= 100)
                        pT->mCurEv.mPercent = 99;

                    pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;
                    pT->mpObserver->onEvent(pT->mCurEv);
                }
            }*/
			break;
		case AM_SCAN_PROGRESS_MGT_DONE: {
			mgt_section_info_t *mgt = (mgt_section_info_t *)evt->data;

			if (pT->mCurEv.mTotalChannelCount == 1) {
				pT->mCurEv.mPercent += 10;
				if (pT->mCurEv.mPercent >= 100)
					pT->mCurEv.mPercent = 99;

				pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;

				pT->mpObserver->onEvent(pT->mCurEv);
			}
		}
		break;
		case AM_SCAN_PROGRESS_VCT_DONE: {
			/*ATSC TVCT*/
			if (pT->mCurEv.mTotalChannelCount == 1) {
				pT->mCurEv.mPercent += 30;
				if (pT->mCurEv.mPercent >= 100)
					pT->mCurEv.mPercent = 99;
				pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;

				pT->mpObserver->onEvent(pT->mCurEv);
			}
		}
		break;
		case AM_SCAN_PROGRESS_NEW_PROGRAM: {
			/* Notify the new searched programs */
			AM_SCAN_ProgramProgress_t *pp = (AM_SCAN_ProgramProgress_t *)evt->data;
			if (pp != NULL) {
				pT->mCurEv.mprogramType = pp->service_type;
				snprintf(pT->mCurEv.mProgramName, sizeof(pT->mCurEv.mProgramName), "%s", pp->name);

				pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;

				pT->mpObserver->onEvent(pT->mCurEv);
			}
		}
		break;
		case AM_SCAN_PROGRESS_BLIND_SCAN: {
			AM_SCAN_DTVBlindScanProgress_t *bs_prog = (AM_SCAN_DTVBlindScanProgress_t *)evt->data;

			if (bs_prog) {
				pT->mCurEv.mPercent = bs_prog->progress;

				snprintf(pT->mCurEv.mMSG, sizeof(pT->mCurEv.mMSG), "%s/%s %dMHz",
						 bs_prog->polar == AM_FEND_POLARISATION_H ? "H" : "V",
						 bs_prog->lo == AM_FEND_LOCALOSCILLATORFREQ_L ? "L-LOF" : "H-LOF",
						 bs_prog->freq / 1000);

				pT->mCurEv.mType = ScannerEvent::EVENT_BLINDSCAN_PROGRESS;

				pT->mpObserver->onEvent(pT->mCurEv);

				if (bs_prog->new_tp_cnt > 0) {
					int i = 0;
					for (i = 0; i < bs_prog->new_tp_cnt; i++) {
						LOGD("====New tp: %dkS/s %d====", bs_prog->new_tps[i].frequency,
							 bs_prog->new_tps[i].u.qpsk.symbol_rate);

						pT->mCurEv.mFrequency = bs_prog->new_tps[i].frequency;
						pT->mCurEv.mSymbolRate = bs_prog->new_tps[i].u.qpsk.symbol_rate;
						pT->mCurEv.mSat_polarisation = bs_prog->polar;

						pT->mCurEv.mType = ScannerEvent::EVENT_BLINDSCAN_NEWCHANNEL;

						pT->mpObserver->onEvent(pT->mCurEv);
					}
				}
				if (bs_prog->progress >= 100) {

					pT->mCurEv.mType = ScannerEvent::EVENT_BLINDSCAN_END;

					pT->mpObserver->onEvent(pT->mCurEv);

					pT->mCurEv.mPercent = 0;
				}
			}
		}
		break;
		case AM_SCAN_PROGRESS_STORE_BEGIN: {
			pT->mCurEv.mType = ScannerEvent::EVENT_STORE_BEGIN;
			pT->mCurEv.mLockedStatus = 0;
			if(pT->mpObserver != NULL)
				pT->mpObserver->onEvent(pT->mCurEv);
		}
		break;
		case AM_SCAN_PROGRESS_STORE_END: {
			pT->mCurEv.mLockedStatus = 0;
			pT->mCurEv.mType = ScannerEvent::EVENT_STORE_END;
			pT->mpObserver->onEvent(pT->mCurEv);
		}
		break;
		case AM_SCAN_PROGRESS_SCAN_END: {
			pT->mCurEv.mPercent = 100;
			pT->mCurEv.mLockedStatus = 0;
			pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_END;
			pT->mpObserver->onEvent(pT->mCurEv);
		}
		break;
		case AM_SCAN_PROGRESS_ATV_TUNING: {
			pT->mCurEv.mFrequency = (int)evt->data;
			pT->mCurEv.mLockedStatus = 0;
			tmpFreq = (pT->mCurEv.mFrequency - pT->mCurScanStartFreq) / 1000000;
			pT->mCurEv.mPercent = tmpFreq * 100UL / ((pT->mCurScanEndFreq - pT->mCurScanStartFreq) / 1000000);
			pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;
			pT->mpObserver->onEvent(pT->mCurEv);
		}
		break;

		default:
			break;
		}
	} else if (event_type == AM_SCAN_EVT_SIGNAL) {
		AM_SCAN_DTVSignalInfo_t *evt = (AM_SCAN_DTVSignalInfo_t *)param;
		pT->mCurEv.mprogramType = 0xff;
		pT->mCurEv.mFrequency = (int)evt->frequency;
		pT->mCurEv.mLockedStatus = (evt->locked ? 1 : 0);

		pT->mCurEv.mType = ScannerEvent::EVENT_SCAN_PROGRESS;
		if(pT->mCurEv.mMode != FE_ANALOG && evt->locked) {
			pT->mCurEv.mStrength = evt->strength;
			pT->mCurEv.mSnr = evt->snr;
		} else {
			pT->mCurEv.mStrength = 0;
			pT->mCurEv.mSnr = 0;
		}

		if (pT->mCurEv.mMode == FE_ANALOG)
			pT->mpObserver->onEvent(pT->mCurEv);
	}
}

void CTvScanner::tv_scan_reconnect_dmx_to_fend(int dmx_no, int fend_no)
{
	AM_DMX_Source_t src;

	if (AM_FEND_GetTSSource(fend_no, &src) == AM_SUCCESS) {
		LOGD("Set demux%d source to %d", dmx_no, src);
		AM_DMX_SetSource(dmx_no, src);
	} else {
		LOGD("Cannot get frontend ts source!!");
	}
}

int CTvScanner::getAtscChannelPara(int attennaType, Vector<sp<CTvChannel> > &vcp)
{
	switch(attennaType) { //region name should be remove to config file and read here
	case 1:
		CTvRegion::getChannelListByName("U.S.,ATSC Air", vcp);
		break;
	case 2:
		CTvRegion::getChannelListByName("U.S.,ATSC Cable Standard", vcp);
		break;
	case 3:
		CTvRegion::getChannelListByName("U.S.,ATSC Cable IRC", vcp);
		break;
	case 4:
		CTvRegion::getChannelListByName("U.S.,ATSC Cable HRC", vcp);
		break;
	default:
		return -1;
	}

	return 0;
}

