#ifndef _C_VPP_H
#define _C_VPP_H
#include "amstream.h"
#include "cm.h"
#include "amvecm.h"
#include "pqdata.h"
#include "../tvin/CTvin.h"
#define GLOBAL_OGO_FORMAT_FLAG  0x6688
#define DEFAULT_RANGE   0
#define LIMIT_RANGE     1
#define FULL_RANGE      2
#define RGB_FORMAT          0
#define YCbCr_422_FORMAT    1
#define YCbCr_444_FORMAT    2
#define PQ_USER_DATA_FROM_E2P   0
#define PQ_USER_DATA_FROM_DB    1

#define VPP_DEV_PATH    "/dev/amvecm"
#define VPP_3D_DEV_PATH    "/dev/amvideo"

#define MODE_VPP_3D_DISABLE     0x00000000
#define MODE_VPP_3D_ENABLE      0x00000001
#define MODE_VPP_3D_AUTO        0x00000002
#define MODE_VPP_3D_LR          0x00000004
#define MODE_VPP_3D_TB          0x00000008
#define MODE_VPP_3D_LA          0x00000010
#define MODE_VPP_3D_FA         0x00000020
#define MODE_VPP_3D_LR_SWITCH   0x00000100
#define MODE_VPP_3D_TO_2D_L     0x00000200
#define MODE_VPP_3D_TO_2D_R     0x00000400

typedef union tag_suc {
    short s;
    unsigned char c[2];
} SUC;

typedef union tag_usuc {
    unsigned short s;
    unsigned char c[2];
} USUC;

typedef enum is_3d_type_e {
    INDEX_3D_INVALID = -1,
    INDEX_2D = 0,
    INDEX_3D = 1,
} is_3d_type_t;

typedef enum vpp_panorama_mode_e {
    VPP_PANORAMA_MODE_FULL,
    VPP_PANORAMA_MODE_NORMAL,
    VPP_PANORAMA_MODE_MAX,
} vpp_panorama_mode_t;

typedef enum vpp_color_space_type_e {
    VPP_COLOR_SPACE_AUTO,
    VPP_COLOR_SPACE_YUV,
    VPP_COLOR_SPACE_RGB,
} vpp_color_space_type_t;

typedef enum vpp_display_mode_e {
    VPP_DISPLAY_MODE_169,
    VPP_DISPLAY_MODE_PERSON,
    VPP_DISPLAY_MODE_MOVIE,
    VPP_DISPLAY_MODE_CAPTION,
    VPP_DISPLAY_MODE_MODE43,
    VPP_DISPLAY_MODE_FULL,
    VPP_DISPLAY_MODE_NORMAL,
    VPP_DISPLAY_MODE_NOSCALEUP,
    VPP_DISPLAY_MODE_CROP_FULL,
    VPP_DISPLAY_MODE_CROP,
    VPP_DISPLAY_MODE_ZOOM,
    VPP_DISPLAY_MODE_FULL_REAL,//add for N360 by haifeng.liu
    VPP_DISPLAY_MODE_MAX,
} vpp_display_mode_t;

typedef enum vpp_color_demomode_e {
    VPP_COLOR_DEMO_MODE_ALLON,
    VPP_COLOR_DEMO_MODE_YOFF,
    VPP_COLOR_DEMO_MODE_COFF,
    VPP_COLOR_DEMO_MODE_GOFF,
    VPP_COLOR_DEMO_MODE_MOFF,
    VPP_COLOR_DEMO_MODE_ROFF,
    VPP_COLOR_DEMO_MODE_BOFF,
    VPP_COLOR_DEMO_MODE_RGBOFF,
    VPP_COLOR_DEMO_MODE_YMCOFF,
    VPP_COLOR_DEMO_MODE_ALLOFF,
    VPP_COLOR_DEMO_MODE_MAX,
} vpp_color_demomode_t;

typedef enum vpp_color_basemode_e {
    VPP_COLOR_BASE_MODE_OFF,
    VPP_COLOR_BASE_MODE_OPTIMIZE,
    VPP_COLOR_BASE_MODE_ENHANCE,
    VPP_COLOR_BASE_MODE_DEMO,
    VPP_COLOR_BASE_MODE_MAX,
} vpp_color_basemode_t;

typedef enum vpp_color_management2_e {
    VPP_COLOR_MANAGEMENT2_MODE_OFF,
    VPP_COLOR_MANAGEMENT2_MODE_OPTIMIZE,
    VPP_COLOR_MANAGEMENT2_MODE_ENHANCE,
    VPP_COLOR_MANAGEMENT2_MODE_DEMO,
    VPP_COLOR_MANAGEMENT2_MODE_MAX,
} vpp_color_management2_t;

typedef enum vpp_noise_reduction2_mode_e {
    VPP_NOISE_REDUCTION2_MODE_OFF,
    VPP_NOISE_REDUCTION2_MODE_LOW,
    VPP_NOISE_REDUCTION2_MODE_MID,
    VPP_NOISE_REDUCTION2_MODE_HIGH,
    VPP_NOISE_REDUCTION2_MODE_AUTO,
    VPP_NOISE_REDUCTION2_MODE_MAX,
} vpp_noise_reduction2_mode_t;
typedef enum vpp_xvycc_mode_e {
    VPP_XVYCC_MODE_OFF,
    VPP_XVYCC_MODE_STANDARD,
    VPP_XVYCC_MODE_ENHANCE,
    VPP_XVYCC_MODE_MAX,
} vpp_xvycc_mode_t;

typedef enum vpp_mcdi_mode_e {
    VPP_MCDI_MODE_OFF,
    VPP_MCDI_MODE_STANDARD,
    VPP_MCDI_MODE_ENHANCE,
    VPP_MCDI_MODE_MAX,
} vpp_mcdi_mode_t;

typedef enum vpp_picture_mode_e {
    VPP_PICTURE_MODE_STANDARD,
    VPP_PICTURE_MODE_BRIGHT,
    VPP_PICTURE_MODE_SOFT,
    VPP_PICTURE_MODE_USER,
    VPP_PICTURE_MODE_MOVIE,
    VPP_PICTURE_MODE_COLORFUL,
    VPP_PICTURE_MODE_MAX,
} vpp_picture_mode_t;

typedef enum vpp_color_temperature_mode_e {
    VPP_COLOR_TEMPERATURE_MODE_STANDARD,
    VPP_COLOR_TEMPERATURE_MODE_WARM,
    VPP_COLOR_TEMPERATURE_MODE_COLD,
    VPP_COLOR_TEMPERATURE_MODE_USER,
    VPP_COLOR_TEMPERATURE_MODE_MAX,
} vpp_color_temperature_mode_t;

typedef enum vpp_noise_reduction_mode_e {
    VPP_NOISE_REDUCTION_MODE_OFF,
    VPP_NOISE_REDUCTION_MODE_LOW,
    VPP_NOISE_REDUCTION_MODE_MID,
    VPP_NOISE_REDUCTION_MODE_HIGH,
    VPP_NOISE_REDUCTION_MODE_AUTO,
    VPP_NOISE_REDUCTION_MODE_MAX,
} vpp_noise_reduction_mode_t;

typedef enum vpp_test_pattern_e {
    VPP_TEST_PATTERN_NONE,
    VPP_TEST_PATTERN_RED,
    VPP_TEST_PATTERN_GREEN,
    VPP_TEST_PATTERN_BLUE,
    VPP_TEST_PATTERN_WHITE,
    VPP_TEST_PATTERN_BLACK,
    VPP_TEST_PATTERN_MAX,
} vpp_test_pattern_e;

typedef struct vpp_pq_para_s {
    int brightness;
    int contrast;
    int saturation;
    int hue;
    int sharpness;
    int backlight;
    int nr;
} vpp_pq_para_t;

typedef enum noline_params_type_e {
    NOLINE_PARAMS_TYPE_BRIGHTNESS,
    NOLINE_PARAMS_TYPE_CONTRAST,
    NOLINE_PARAMS_TYPE_SATURATION,
    NOLINE_PARAMS_TYPE_HUE,
    NOLINE_PARAMS_TYPE_SHARPNESS,
    NOLINE_PARAMS_TYPE_VOLUME,
    NOLINE_PARAMS_TYPE_BACKLIGHT,
    NOLINE_PARAMS_TYPE_MAX,
} noline_params_type_t;

typedef struct noline_params_s {
    int osd0;
    int osd25;
    int osd50;
    int osd75;
    int osd100;
} noline_params_t;

typedef enum vpp_dream_panel_e {
    VPP_DREAM_PANEL_OFF,
    VPP_DREAM_PANEL_LIGHT,
    VPP_DREAM_PANEL_SCENE,
    VPP_DREAM_PANEL_FULL,
    VPP_DREAM_PANEL_DEMO,
    VPP_DREAM_PANEL_MAX,
} vpp_dream_panel_t;

class CPqData;

class CVpp
{
public:
    CVpp();
    ~CVpp();
    int Vpp_Init ( void );
    int Vpp_Uninit ( void );
    CPqData *getPqData();
    int Vpp_ResetLastVppSettingsSourceType ( void );
    int Vpp_SetColorDemoMode ( vpp_color_demomode_t demomode );
    int Vpp_SetBaseColorMode ( vpp_color_basemode_t basemode , tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt);
    int Vpp_SetColorTemperatureUser ( vpp_color_temperature_mode_t temp_mode, tv_source_input_type_t source_type );
    int Vpp_SetColorTemperature ( vpp_color_temperature_mode_t Tempmode, tv_source_input_type_t source_type, tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt);
    int Vpp_SetBrightness ( int value, tv_source_input_type_t source_type , tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    int Vpp_SetContrast ( int value, tv_source_input_type_t source_type , tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    int Vpp_SetSaturation ( int value, tv_source_input_type_t source_type , tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    int Vpp_SetHue ( int value, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, tvin_port_t source_port, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt  );
    int Vpp_SetSharpness ( int value, tv_source_input_type_t source_type , tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    int Vpp_SetPQMode ( vpp_picture_mode_t pq_mode, tv_source_input_type_t source_type, tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    int Vpp_SetNoiseReductionMode ( vpp_noise_reduction_mode_t nr_mode,  tv_source_input_type_t source_type , tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    int Vpp_SetXVYCCMode ( vpp_xvycc_mode_t xvycc_mode, tv_source_input_type_t source_type, tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    int Vpp_SetMCDIMode ( vpp_mcdi_mode_t mcdi_mode, tv_source_input_type_t source_type , tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt);
    int Vpp_SetZoom ( int value );
    int Vpp_LoadBasicRegs ( tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    int VppRegisterInterface();
    int Tv_RGBGainValueSSMToRisterMapping ( int gainValue );
    int Tv_RGBOffsetValueSSMToRisterMapping ( int gainValue );
    int Tv_SetRGBValue ( vpp_color_temperature_mode_t temp_mode,  unsigned char data_buf[] );
    int Tv_GetRGBValue ( vpp_color_temperature_mode_t temp_mode,  tcon_rgb_ogo_t *p_rgbogo );
    int Tv_SetColorSpaceMode ( vpp_color_space_type_t colorSpace );
    //int Tv_SetGammaValue(int gammaValue);
    int Tv_GetGammaValue();
    //int Tv_SetColorDemoMode ( vpp_color_demomode_t demomode );
    vpp_color_demomode_t Tv_GetColorDemoMode ( void );

    int Tv_SetBaseColorMode ( vpp_color_basemode_t basemode , tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt);
    vpp_color_basemode_t Tv_GetBaseColorMode ( void );
    int Tv_SetBaseColorModeWithoutSave ( vpp_color_basemode_t basemode , tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt);
    int Tv_SaveBaseColorMode ( vpp_color_basemode_t basemode );
    int Tv_SetColorTemperature ( vpp_color_temperature_mode_t Tempmode, tv_source_input_type_t source_type, int is_save );
    vpp_color_temperature_mode_t Tv_GetColorTemperature ( tv_source_input_type_t source_type );
    int Tv_SetColorTempWithoutSave ( vpp_color_temperature_mode_t Tempmode, tv_source_input_type_t source_type );
    int Tv_SaveColorTemp ( vpp_color_temperature_mode_t Tempmode, tv_source_input_type_t source_type );
    int Tv_SetBrightness ( int value, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save );
    int Tv_GetBrightness ( tv_source_input_type_t source_type );
    int Tv_SetContrast ( int value, tv_source_input_type_t source_type,  tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save );
    int Tv_GetContrast ( tv_source_input_type_t source_type );
    int Tv_SetSaturation ( int value, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save );
    int Tv_GetSaturation ( tv_source_input_type_t source_type );
    int Tv_SetHue ( int value, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save );
    int Tv_GetHue ( tv_source_input_type_t source_type );
    int Tv_SetSharpness ( int value, tv_source_input_type_t source_type, int is_enable, is_3d_type_t is3d, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, int is_save );
    int Tv_GetSharpness ( tv_source_input_type_t source_type );
    int Tv_SetBacklight ( int value, tv_source_input_type_t source_type, int is_save );
    int Tv_GetBacklight ( tv_source_input_type_t source_type );
    int Tv_SetBacklightWithoutSave ( int value, tv_source_input_type_t source_type );
    int Tv_SaveBacklight ( int value, tv_source_input_type_t source_type );
    int Tv_SetPQMode ( vpp_picture_mode_t pq_mode, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save );
    vpp_picture_mode_t Tv_GetPQMode ( tv_source_input_type_t source_type );
    int Tv_SavePQMode ( vpp_picture_mode_t pq_mode, tv_source_input_type_t source_type );
    int Tv_SetDisplayMode ( vpp_display_mode_t display_mode, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, int is_save );
    vpp_display_mode_t Tv_GetDisplayMode ( tv_source_input_type_t source_type );
    int Tv_SaveNoiseReductionMode ( vpp_noise_reduction_mode_t nr_mode, tv_source_input_type_t source_type );
    int Tv_SetNoiseReductionMode ( vpp_noise_reduction_mode_t nr_mode, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt, int is_save );
    vpp_noise_reduction_mode_t Tv_GetNoiseReductionMode ( tv_source_input_type_t source_type );
    int Tv_SetRGBOGO_PostOffset(int rgb, int offset);
    int Tv_SetRGBOGO_Gain(int rgb, int gain);
    int Tv_GetRGBOGO_PostOffset ( int rgb );
    int Tv_GetRGBOGO_Gain( int rgb );
    int Tv_SetDNLP ( tv_source_input_type_t source_type, tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    int Tv_LoadVppSettings ( tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    //int Tv_SetVppParamDefault(void);


    int Tv_FactorySetPQMode_Brightness ( int source_type, int pq_mode, int brightness );
    int Tv_FactoryGetPQMode_Brightness ( int source_type, int pq_mode );
    int Tv_FactorySetPQMode_Contrast ( int source_type, int pq_mode, int contrast );
    int Tv_FactoryGetPQMode_Contrast ( int source_type, int pq_mode );
    int Tv_FactorySetPQMode_Saturation ( int source_type, int pq_mode, int saturation );
    int Tv_FactoryGetPQMode_Saturation ( int source_type, int pq_mode );
    int Tv_FactorySetPQMode_Hue ( int source_type, int pq_mode, int hue );
    int Tv_FactoryGetPQMode_Hue ( int source_type, int pq_mode );
    int Tv_FactorySetPQMode_Sharpness ( int source_type, int pq_mode, int sharpness );
    int Tv_FactoryGetPQMode_Sharpness ( int source_type, int pq_mode );
    int Tv_FactorySetColorTemp_Rgain ( int source_type, int colortemp_mode, int rgain );
    int Tv_FactorySaveColorTemp_Rgain ( int source_type, int colortemp_mode, int rgain );
    int Tv_FactoryGetColorTemp_Rgain ( int source_type, int colortemp_mode );
    int Tv_FactorySetColorTemp_Ggain ( int source_type, int colortemp_mode, int ggain );
    int Tv_FactorySaveColorTemp_Ggain ( int source_type, int colortemp_mode, int ggain );
    int Tv_FactoryGetColorTemp_Ggain ( int source_type, int colortemp_mode );
    int Tv_FactorySetColorTemp_Bgain ( int source_type, int colortemp_mode, int bgain );
    int Tv_FactorySaveColorTemp_Bgain ( int source_type, int colortemp_mode, int bgain );
    int Tv_FactoryGetColorTemp_Bgain ( int source_type, int colortemp_mode );
    int Tv_FactorySetColorTemp_Roffset ( int source_type, int colortemp_mode, int roffset );
    int Tv_FactorySaveColorTemp_Roffset ( int source_type, int colortemp_mode, int roffset );
    int Tv_FactoryGetColorTemp_Roffset ( int source_type, int colortemp_mode );
    int Tv_FactorySetColorTemp_Goffset ( int source_type, int colortemp_mode, int goffset );
    int Tv_FactorySaveColorTemp_Goffset ( int source_type, int colortemp_mode, int goffset );
    int Tv_FactoryGetColorTemp_Goffset ( int source_type, int colortemp_mode );
    int Tv_FactorySetColorTemp_Boffset ( int source_type, int colortemp_mode, int boffset );
    int Tv_FactorySaveColorTemp_Boffset ( int source_type, int colortemp_mode, int boffset );
    int Tv_FactoryGetColorTemp_Boffset ( int source_type, int colortemp_mode );
    //int Tv_FactorySaveRGBDatatoAllSrc ( int source_type, int colortemp_mode );
    int Tv_FactoryGetTestPattern ( void );
    int Tv_FactoryResetPQMode ( void );
    int TV_FactoryResetNonlinear ( void );
    int Tv_FactoryResetColorTemp ( void );
    int Tv_FactorySetParamsDefault ( void );
    int Tv_FactorySetDDRSSC ( int step );
    int Tv_FactoryGetDDRSSC();
    int Tv_FactorySetLVDSSSC ( int step );
    int Tv_FactoryGetLVDSSSC();
    int Tv_FactorySetNolineParams ( int type, int source_type, noline_params_t noline_params );
    noline_params_t Tv_FactoryGetNolineParams ( int type, int source_type );
    int Tv_FactorySetOverscan ( int source_type, int fmt, int status_3d, int trans_fmt, tvin_cutwin_t cutwin_t );
    tvin_cutwin_t Tv_FactoryGetOverscan ( int source_type, int fmt, is_3d_type_t is3d, int trans_fmt );
    int Tv_FactorySetBacklightPWM_Frequency(int freq);
    int Tv_FactoryGetBacklightPWM_Frequency ( void );
    int Tv_FactorySetBacklight_Switch_status ( int status );
    int Tv_FactoryGetBacklight_Switch_status ( void );
    int Tv_FactorySetBacklightPWM_Duty ( int duty );
    int Tv_FactoryGetBacklightPWM_Duty ( void );
    int Tv_FactorySetLVDS_ColorDepth ( int depth );
    int Tv_FactoryGetLVDS_ColorDepth ( void );
    int Tv_FactorySetLVDS_ColorDither_status ( int status );
    int Tv_FactoryGetLVDS_ColorDither_status ( void );
    int Tv_FactorySetLVDS_Mapping_status ( int status );
    int Tv_FactoryGetLVDS_Mapping_status ( void );
    int Tv_FactorySetLVDS_PortSwap_status ( int status );
    int Tv_FactoryGetLVDS_PortSwap_status ( void );

    int VPPSSMRestoreDefault();
    int VPPSSMFacRestoreDefault();


    int Tv_GetHistogram_AVE();
    int Tv_SetDnlp_OFF();
    int Tv_SetDnlp_ON ( void );
    int Tv_GetDnlp_Status();

    int GetColorTemperatureParams ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params );
    int ReadColorTemperatureParams ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params );
    int SetColorTemperatureParams ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params );
    int SaveColorTemperatureParams ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params );
    unsigned short CalColorTemperatureParamsChecksum ( void );
    int SetColorTempParamsChecksum ( void );
    unsigned short GetColorTempParamsChecksum ( void );
    int CheckTempDataLable ( void );
    int SetTempDataLable ( void );
    int CheckColorTemperatureParams ( void );
    int RestoeColorTemperatureParamsFromDB ( tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt );
    int CheckColorTemperatureParamAlldata ( tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt );
    void  Tvin_SetVideoScreenColorType ( int type );
    int Vpp_GetVppConfig();
    int Vpp_GetPQModeValue ( tv_source_input_type_t, vpp_picture_mode_t, vpp_pq_para_t * );
    int Vpp_SetPQParams ( tv_source_input_type_t source_type, vpp_picture_mode_t pq_mode, vpp_pq_para_t pq_para , tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt);

    int Vpp_LoadGammaDefault ( tv_source_input_type_t, tvin_sig_fmt_t );
    int Vpp_LoadGammaSpecial ( int );
    int Vpp_LoadGamma ( tv_source_input_type_t, tvin_sig_fmt_t );
    int Tv_FactoryResetNonlinear();
    tvin_cutwin_t Tv_GetOverscan ( tv_source_input_type_t source_type, tvin_sig_fmt_t fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    static is_3d_type_t Check2Dor3D ( is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    //api
    int VPP_SetVEBlackExtension ( const struct ve_bext_s *pBExt );
    int VPP_SetVideoCrop ( int Voffset0, int Hoffset0, int Voffset1, int Hoffset1 );
    int VPP_SetNonLinearFactor ( int value );
    int VPP_SetGrayPattern(int value);
    int VPP_GetGrayPattern();
private:
    //
    int VPP_OpenModule ( void );
    int VPP_CloseModule ( void );
    int VPP_SetVideoBrightness ( int value );
    int VPP_SetVideoContrast ( int value );
    int VPP_SetVideoSaturationHue ( int satVal, int hueVal );
    int VPP_SetCMRegisterMap ( struct cm_regmap_s *pRegMap );
    int Vpp_LoadRegs ( am_regs_t regs );
    int VPP_SetRGBOGO ( const struct tcon_rgb_ogo_s *rgbogo );
    int VPP_GetRGBOGO ( const struct tcon_rgb_ogo_s *rgbogo );
    int VPP_SetGammaOnOff ( unsigned char onoff );
    int VPP_SetGammaTbl_R ( unsigned short red[256] );
    int VPP_SetGammaTbl_G ( unsigned short green[256] );
    int VPP_SetGammaTbl_B ( unsigned short blue[256] );
    void video_set_saturation_hue ( signed char saturation, signed char hue, signed long *mab );
    void video_get_saturation_hue ( signed char *sat, signed char *hue, signed long *mab );
    int VPP_SetBackLightLevel ( int value );
    int VPP_SetVEDNLP ( const struct ve_dnlp_s *pDNLP );
    int VPP_SetVENewDNLP ( const ve_dnlp_table_t *pDNLP );
    int Vpp_GetAVGHistogram ( struct ve_hist_s *hist );
    int Vpp_SetDnlpOff ( void );
    int Vpp_SetDnlpOn ( void );
    int VPP_SetVESharpness ( const ve_hsvs_s * );
    int VPP_SetVEChromaCoring ( const ve_ccor_s * );
    int VPP_SetVEBlueEnh ( const ve_benh_s * );
    int VPP_SetVEDemo ( const ve_demo_s * );
    int VPP_SetVERegisterMap ( const ve_regmap_s * );
    int VPP_SetVEDebug ( const long long unsigned int * );
    int VPP_SetCMRegion ( const cm_region_s * );
    int VPP_SetCMTopLayer ( const cm_top_s * );
    int VPP_SetCMDemo ( const cm_demo_s * );

    int VPP_DeviceIOCtl ( int request, ... );
    int VPP3D_DeviceIOCtl ( int, ... );
    int VPP_SetCMDebug ( const long long unsigned int * );
    int VPP_SetAVSyncEnable ( unsigned int );
    int VPP_SetVideoNoiseReduction ( int );
    int VPP_SetVideoSaturation ( int );
    int VPP_SetVideoHue ( int );
    int VPP_SetDeinterlaceMode ( int );


    tv_source_input_type_t vpp_setting_last_source_type;
    tvin_sig_fmt_t vpp_setting_last_sig_fmt;
    tvin_trans_fmt_t vpp_setting_last_trans_fmt;
    //cfg
    bool mbVppCfg_backlight_reverse;
    bool mbVppCfg_backlight_init;
    bool mbVppCfg_pqmode_without_hue;
    bool mbVppCfg_hue_reverse;
    bool mbVppCfg_gamma_onoff;
    bool mbVppCfg_whitebalance_sameparam;
    bool mbVppCfg_new_cm;
    bool mbVppCfg_new_nr;
    bool mbVppCfg_panorama_switch;
    bool mbVppCfg_pqmode_depend_bklight;
    bool mbVppCfg_colortemp_by_source;

    CPqData *mpPqData;

    int vpp_amvideo_fd;
    int vpp_amvideo_3d_fd;
};
#endif
