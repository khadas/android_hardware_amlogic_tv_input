//
//
//  amlogic 2013
//
//  @ Project : tv
//  @ Date : 2013-11
//  @ Author :
//
//
#include "CTvSubtitle.h"
CTvSubtitle::CTvSubtitle()
{
    mpObser = NULL;
}
CTvSubtitle::~CTvSubtitle()
{
}
void CTvSubtitle::setObser(IObserver *pObser)
{
    mpObser = pObser;
}
void CTvSubtitle::stopDecoder()
{
}
/**
 * 开始字幕信息解析showboz sync
 */
void CTvSubtitle::startSub()
{
}

/**
 * 停止图文/字幕信息解析
 */
void CTvSubtitle::stop()
{
}

/**
 * 停止图文/字幕信息解析并清除缓存数据
 */
void CTvSubtitle::clear()
{

}

/**
 * 在图文模式下进入下一页
 */
void CTvSubtitle::nextPage()
{

}

/**
 * 在图文模式下进入上一页
 */
void CTvSubtitle::previousPage()
{

}

/**
 * 在图文模式下跳转到指定页
 * @param page 要跳转到的页号
 */
void CTvSubtitle::gotoPage(int page)
{

}

/**
 * 在图文模式下跳转到home页
 */
void CTvSubtitle::goHome()
{

}

/**
 * 在图文模式下根据颜色跳转到指定链接
 * @param color 颜色，COLOR_RED/COLOR_GREEN/COLOR_YELLOW/COLOR_BLUE
 */
void CTvSubtitle::colorLink(int color)
{

}

/**
 * 在图文模式下设定搜索字符串
 * @param pattern 搜索匹配字符串
 * @param casefold 是否区分大小写
 */
void CTvSubtitle::setSearchPattern(char *pattern, bool casefold)
{

}

/**
 * 搜索下一页
 */
void CTvSubtitle::searchNext()
{
}

/**
 * 搜索上一页
 */
void CTvSubtitle::searchPrevious()
{
}

int CTvSubtitle::sub_init()
{
    return 0;
}

int CTvSubtitle::sub_destroy()
{
    return 0;
}

int CTvSubtitle::sub_lock()
{
    return 0;
}

int CTvSubtitle::sub_unlock()
{
    return 0;
}

int CTvSubtitle::sub_clear()
{
    return 0;
}

int CTvSubtitle::sub_start_dvb_sub(int dmx_id, int pid, int page_id, int anc_page_id)
{
    return 0;
}

int CTvSubtitle::sub_start_dtv_tt(int dmx_id, int region_id, int pid, int page, int sub_page, bool is_sub)
{
    return 0;
}

int CTvSubtitle::sub_stop_dvb_sub()
{
    return 0;
}

int CTvSubtitle::sub_stop_dtv_tt()
{
    return 0;
}

int CTvSubtitle::sub_tt_goto(int page)
{
    return 0;
}

int CTvSubtitle::sub_tt_color_link(int color)
{
    return 0;
}

int CTvSubtitle::sub_tt_home_link()
{
    return 0;
}

int CTvSubtitle::sub_tt_next(int dir)
{
    return 0;
}

int CTvSubtitle::sub_tt_set_search_pattern(char *pattern, bool casefold)
{
    return 0;
}

int CTvSubtitle::sub_tt_search(int dir)
{
    return 0;
}

/*
 * 1, Set the country first and parameters should be either USA or KOREA
#define CMD_SET_COUNTRY_USA                 0x5001
#define CMD_SET_COUNTRY_KOREA            0x5002

2, Set the source type which including
    a)VBI data(for analog program only)
    b)USER data(for AIR or Cable service)
CMD_CC_SET_VBIDATA   = 0x7001,
CMD_CC_SET_USERDATA = 0x7002,
2.1 If the frontend type is Analog we must set the channel Index
     with command 'CMD_CC_SET_CHAN_NUM' and the parameter is like 57M
     we set 0x20000, this should according to USA standard frequency
     table.

3, Next is to set the CC service type

#define CMD_CC_1                        0x3001
#define CMD_CC_2                        0x3002
#define CMD_CC_3                        0x3003
#define CMD_CC_4                        0x3004

//this doesn't support currently
#define CMD_TT_1                        0x3005
#define CMD_TT_2                        0x3006
#define CMD_TT_3                        0x3007
#define CMD_TT_4                        0x3008

#define CMD_SERVICE_1                 0x4001
#define CMD_SERVICE_2                 0x4002
#define CMD_SERVICE_3                 0x4003
#define CMD_SERVICE_4                 0x4004
#define CMD_SERVICE_5                 0x4005
#define CMD_SERVICE_6                 0x4006

4, Then set CMD_CC_START to start the CC service, and you needn't to stop

CC service while switching services

5, CMD_CC_STOP should be called in some cases like switch source, change

program, no signal, blocked...*/

//channel_num == 0 ,if frontend is dtv
//else != 0
int CTvSubtitle::sub_start_atsc_cc(enum cc_param_country country, enum cc_param_source_type src_type, int channel_num, enum cc_param_caption_type caption_type)
{
    LOGD("----sub_start_atsc_cc-1--- country=%d,src=%d,ctype=%d", country, src_type, caption_type);
    switch (country) {
    case CC_PARAM_COUNTRY_USA:
        AM_CC_Cmd(CMD_SET_COUNTRY_USA);
        break;
    case CC_PARAM_COUNTRY_KOREA:
        AM_CC_Cmd(CMD_SET_COUNTRY_KOREA);
        break;
    default:
        AM_CC_Cmd(CMD_SET_COUNTRY_USA);
        break;
    }

    switch (src_type) {
    case CC_PARAM_SOURCE_VBIDATA:
        AM_CC_Cmd(CMD_CC_SET_VBIDATA);
        break;
    case CC_PARAM_SOURCE_USERDATA:
        AM_CC_Cmd(CMD_CC_SET_USERDATA);
        break;
    default:
        AM_CC_Cmd(CMD_CC_SET_USERDATA);
        break;
    }

    //just for test
    if (channel_num == 0) {
    } else {
        //AM_CC_Cmd(CMD_CC_SET_CHAN_NUM);
    }

    AM_CLOSECAPTION_cmd_t cc_t_cmd;
    switch (caption_type) {
    case CC_PARAM_ANALOG_CAPTION_TYPE_CC1:
        cc_t_cmd = CMD_CC_1;
        break;
    case CC_PARAM_ANALOG_CAPTION_TYPE_CC2:
        cc_t_cmd = CMD_CC_2;
        break;
    case CC_PARAM_ANALOG_CAPTION_TYPE_CC3:
        cc_t_cmd = CMD_CC_3;
        break;
    case CC_PARAM_ANALOG_CAPTION_TYPE_CC4:
        cc_t_cmd = CMD_CC_4;
        break;
    case CC_PARAM_DIGITAL_CAPTION_TYPE_SERVICE1:
        cc_t_cmd = CMD_SERVICE_1;
        break;
    case CC_PARAM_DIGITAL_CAPTION_TYPE_SERVICE2:
        cc_t_cmd = CMD_SERVICE_2;
        break;
    case CC_PARAM_DIGITAL_CAPTION_TYPE_SERVICE3:
        cc_t_cmd = CMD_SERVICE_3;
        break;
    case CC_PARAM_DIGITAL_CAPTION_TYPE_SERVICE4:
        cc_t_cmd = CMD_SERVICE_4;
        break;
    default:
        cc_t_cmd = CMD_SERVICE_1;
        break;
    }
    AM_CC_Cmd(cc_t_cmd);

    AM_CC_Set_CallBack(close_caption_callback, this);
    AM_VCHIP_Set_CallBack(atv_vchip_callback, this);
    //start
    AM_CC_Cmd(CMD_CC_START);
    LOGD("----sub_start_atsc_cc-2--- country=%d,src=%d,ctype=%d", country, src_type, caption_type);
    return 0;
}

int CTvSubtitle::sub_stop_atsc_cc()
{
    LOGD("----sub_stop_atsc_cc----");
    AM_CC_Cmd(CMD_CC_STOP);
    return 0;
}
int CTvSubtitle::ResetVchipChgStat()
{
    avchip_chg = 0;
    AM_CC_Cmd(CMD_VCHIP_RST_CHGSTAT);
    return 0;
}
int CTvSubtitle::IsVchipChange()
{
    return avchip_chg;
}
//cnt :data buf len
//databuf len  is max 512
//cmdbuf len is max 128
void CTvSubtitle::close_caption_callback(char *str, int cnt, int data_buf[], int cmd_buf[], void *user_data)
{

    CTvSubtitle *pSub = (CTvSubtitle *)user_data;

    if (pSub == NULL) {
        LOGD("sub cc callback is null user data for this");
        return;
    }

    if (pSub->mpObser == NULL) return;

    pSub->mCurCCEv.mDataBufSize = cnt;
    pSub->mCurCCEv.mpDataBuffer = data_buf;
    pSub->mCurCCEv.mCmdBufSize = 128;//max
    pSub->mCurCCEv.mpCmdBuffer = cmd_buf;

    pSub->mpObser->onEvent(pSub->mCurCCEv);
}
void CTvSubtitle::atv_vchip_callback(int Is_chg,  void *user_data)
{
    CTvSubtitle *pSub = (CTvSubtitle *)user_data;
    pSub->avchip_chg = Is_chg;
}
