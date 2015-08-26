#include "CTvHaierDtmb360.h"
#include <cutils/properties.h>
int audio_device_switch = 1;
CTvHaierDtmb360::CTvHaierDtmb360()
{
    mHeadSet.setObserver(this);
}

CTvHaierDtmb360::~CTvHaierDtmb360()
{

}

int CTvHaierDtmb360::OpenTv ( void )
{
    CTv::OpenTv();
    const char *value = config_get_str ( "TV", "tvin.set.N360backlight.en", "null" );
    //add only for haier
    if ( strcmp ( value, "enable" ) == 0 ) {
        Tv_FactorySet_FBC_Backlight_N360(Tv_FactoryGet_FBC_Backlight_N360());
        Tv_FactorySet_FBC_ELEC_MODE(Tv_FactoryGet_FBC_ELEC_MODE());
        Tv_FactorySet_FBC_ColorTemp_Mode_N360(Tv_FactoryGet_FBC_ColorTemp_Mode_N360());
    }
    return 0;
}

int CTvHaierDtmb360::StartTvLock ()
{
    CTv::StartTvLock();
    const char *value;
    value = config_get_str ( "TV", "tvin.2d4G.headset.en", "null" );
    if ( strcmp ( value, "enable" ) == 0 ) {
        StartHeadSetDetect();
        property_set("audio.tv_open.flg", "1");
    }
    return 0;
}

int CTvHaierDtmb360::SetSourceSwitchInput (tv_source_input_t source_input )
{
    CTv::SetSourceSwitchInput(source_input);

    const char *config_value;
    config_value = config_get_str("TV", "audio.switch.karaok.av.enable", "null");
    if (strtoul(config_value, NULL, 10) == 1) {
        if ( source_input == SOURCE_AV1 || source_input == SOURCE_AV2 ) {
            mTvAudio.SetAudioSwitchIO(0);
        } else {
            mTvAudio.SetAudioSwitchIO(1);
        }
    }
    return 0;
}

void CTvHaierDtmb360::onHeadSetDetect(int state, int para)
{
    TvEvent::HeadSetOf2d4GEvent ev;
    char prop[256];
    char prop1[256];
    ev.state = state;
    ev.para = para;
    if (state == 1)
        property_set("audio.headset_plug.enable", "1");
    else
        property_set("audio.headset_plug.enable", "0");
    LOGD ( "%s, %d", __FUNCTION__, __LINE__);
    sendTvEvent(ev);
    property_get("audio.tv_open.flg", prop, "null");
    property_get("audio.output.double_output", prop1, "null");
    if (audio_device_switch == state && (strcmp(prop, "1") == 0)
        && (strcmp(prop1, "0") == 0 || strcmp(prop1, "null") == 0)
        && (SOURCE_DTV != m_source_input)) {
        mTvAudio.openTvAudio(audio_sr);
    }
}


int CTvHaierDtmb360::StartHeadSetDetect()
{
    mHeadSet.startDetect();

    return 0;
}
