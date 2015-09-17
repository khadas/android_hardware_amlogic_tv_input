#ifndef __TV_AUDIO_CFG_H__
#define __TV_AUDIO_CFG_H__

#include "../tvconfig/tvconfig.h"
#include "../tvutils/tvutils.h"

#define CC_LUT_BUF_MASTER                       (0)
#define CC_LUT_BUF_SUPPERBASS                   (1)
#define CC_LUT_BUF_HEADSET                      (2)

#define CC_LUT_BUF_SIZE                         (101)

#define CC_EQ_PRESET_BUF_SIZE                   (24)

#define CC_SUPPERBASS_ADD_SIZE                  (24)

#define CC_NO_LINE_POINTS_MAX_CNT               (101)

#define CC_GET_LUT_TV                           (0)
#define CC_GET_LUT_AV                           (1)
#define CC_GET_LUT_COMP                         (2)
#define CC_GET_LUT_HDMI                         (3)
#define CC_GET_LUT_VGA                          (4)
#define CC_GET_LUT_MPEG                         (5)
#define CC_GET_LUT_HDMI_4K2K                    (6)
#define CC_GET_LUT_USB_4K2K                     (7)
#define CC_GET_TYPE_CNT                         (8)

#define CC_CFG_NOLINE_CFG_SWITCH_IND            (0)
#define CC_CFG_NOLINE_CFG_IND_IND               (1)
#define CC_CFG_LUT_BUF_CFG_IND                  (2)
#define CC_CFG_EQ_CFG_IND                       (3)
#define CC_CFG_EQ_PRESET_BUF_IND                (4)

#define CC_SRS_SURROUND_GAIN_VAL                                        (50)
#define CC_SRS_SURROUND_AMP_MASTER_GAIN_VAL                             (18)
#define CC_SRS_SURROUND_AMP_SUPPERBASS_GAIN_VAL                         (21)

#define CC_SRS_TRUBASS_ONLY_GAIN_VAL                                    (30)
#define CC_SRS_TRUBASS_ONLY_AMP_MASTER_GAIN_VAL                         (12)
#define CC_SRS_TRUBASS_ONLY_AMP_SUPPERBASS_GAIN_VAL                     (15)

#define CC_SRS_DIALOGCLARITY_ONLY_GAIN_VAL                              (30)
#define CC_SRS_DIALOGCLARITY_ONLY_AMP_MASTER_GAIN_VAL                   (12)
#define CC_SRS_DIALOGCLARITY_ONLY_AMP_SUPPERBASS_GAIN_VAL               (21)
#define CC_SRS_DIALOGCLARITY_ONLY_DEFINITION_GAIN_VAL                   (20)

#define CC_SRS_TRUBASS_DIALOGCLARITY_TRUBASS_GAIN_VAL                   (28)
#define CC_SRS_TRUBASS_DIALOGCLARITY_DIALOGCLARITY_GAIN_VAL             (20)
#define CC_SRS_TRUBASS_DIALOGCLARITY_DEFINITION_GAIN_VAL                (5)

#define CC_SRS_TRUBASS_DIALOGCLARITY_AMP_MASTER_GAIN_VAL                (12)
#define CC_SRS_TRUBASS_DIALOGCLARITY_AMP_SUPPERBASS_GAIN_VAL            (15)

#define CC_EQ_AMP_MASTER_GAIN_VAL                                       (0)
#define CC_EQ_AMP_SUPPERBASS_GAIN_VAL                                   (0)

#define CC_DAC_M6C_EXTERNAL_DAC_ON_BOARD                                (0)
#define CC_DAC_T866_EXTERNAL_DAC_ON_BOARD                               (1)
#define CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_FBC                          (2)
#define CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_CUSTOMER_LIB                 (3)
#define CC_DAC_T866_EXTERNAL_DAC_OFF_BOARD_DIGITAL                      (4)
#define CC_DAC_T866_INTERNAL_DAC                                        (5)

#define CC_AUD_RESAMPLE_TYPE_HW                                         (1)
#define CC_AUD_RESAMPLE_TYPE_SW                                         (2)

extern int *GetAudioAmplifierNoLinePointsIndexBuffer(int lut_buf_type,
		int get_type);
extern int *GetAudioAmplifierNoLinePointsDataBuffer(int lut_buf_type,
		int get_type);

extern int GetAudioAmplifierMasterNoLineSwitchFlag();
extern int GetAudioAmplifierMasterNoLinePointsCount();
extern int *GetAudioAmplifierMasterNoLinePointsDataBuffer(int get_type);
extern int *GetAudioAmplifierMasterNoLinePointsIndexBuffer(int get_type);
extern int *GetAudioAmplifierMasterNoLinePointsDefIndexBuffer();
extern int *GetAudioAmplifierMasterNoLinePointsDefDataBuffer(int buf_type);

extern int GetAudioAmplifierSupperBassNoLineSwitchFlag();
extern int GetAudioAmplifierSupperBassNoLinePointsCount();
extern int *GetAudioAmplifierSupperBassNoLinePointsDataBuffer(int get_type);
extern int *GetAudioAmplifierSupperBassNoLinePointsIndexBuffer(int get_type);
extern int *GetAudioAmplifierSupperBassNoLinePointsDefIndexBuffer();
extern int *GetAudioAmplifierSupperBassNoLinePointsDefDataBuffer();

extern int GetAudioAmplifierBalanceExchangeCFG();

extern int GetAudioTrebleBassType();
extern int *GetAudioAmplifierNoLinePointsBuffer(int get_type);
extern int GetAudioMainVolLutBufNameModifyFromUbootenv();
extern int GetAudioMainVolLutBufNameModifyUbootenvCFG();
extern int GetAudioMainVolLutBufNameModifyEnableCFG();
extern int GetAudioMainVolLutBufNameCFG(int get_type, char name_buf[]);
extern int *GetAudioAmplifierMasterVolumeLUTBufferPtr(int get_type,
		int audio_std);
extern int GetAudioSupperBassVolLutBufNameModifyEnableCFG();
extern int GetAudioSupperBassVolLutBufNameCFG(int get_type, char name_buf[]);
extern int *GetAudioAmplifierSupperBassVolumeLUTBufferPtr(int get_type,
		int audio_std);
extern int *GetAudioEQPresetBufferPtr();
extern int GetAudioDVISupportEnable();
extern int GetTvAudioCardID(char tv_card_id_buf[]);
extern int GetTvAudioCardName(char tv_card_name_buf[]);
extern int GetTvAudioCardNeedSet();
extern int GetAudioSRSModuleDisableCFG();
extern int GetAudioSRSSourroundDisableCFG();
extern int GetAudioSRSTrubassDisableCFG();
extern int GetAudioSRSDialogClarityDisableCFG();
extern int GetAudioSRSSourroundGainCfg();
extern int GetAudioSRSSourroundAmplifierMasterGainCfg();
extern int GetAudioSRSSourroundAmplifierSupperBassGainCfg();
extern int GetAudioSRSTrubassOnlyGainCfg();
extern int GetAudioSRSTrubassOnlyAmplifierMasterGainCfg();
extern int GetAudioSRSTrubassOnlyAmplifierSupperBassGainCfg();
extern int GetAudioSRSDialogClarityOnlyGainCfg();
extern int GetAudioSRSDialogClarityOnlyAmplifierMasterGainCfg();
extern int GetAudioSRSDialogClarityOnlyAmplifierSupperBassGainCfg();
extern int GetAudioSRSDialogClarityOnlyDefinitionGainCfg();
extern int GetAudioSRSTrubassAndDialogClarityTrubassGainCfg();
extern int GetAudioSRSTrubassAndDialogClarityDialogClarityGainCfg();
extern int GetAudioSRSTrubassAndDialogClarityDefinitionGainCfg();
extern int GetAudioSRSTrubassAndDialogClarityAmplifierMasterGainCfg();
extern int GetAudioSRSTrubassAndDialogClarityAmplifierSupperBassGainCfg();
extern int GetAudioSRSSupperBassTrubassSpeakerSizeCfg(int get_flag);
extern int GetAudioEQAmplifierMasterAddGainCfg();
extern int GetAudioEQAmplifierSupperBassAddGainCfg();
extern int GetAudioEqModuleDisableCFG();
extern int GetAudioEqUsingAndroidCFG();
extern int GetAudioEQGainRangeCFG(int *min_val, int *max_val);
extern int GetAudioEQUIGainRangeCFG(int *min_val, int *max_val);

extern int GetAudioDumpDataEnableFlagCfg();
extern int GetAudioKeyMuteCfg();
extern int GetAudioAVOutInitMuteStatusCFG();
extern int GetAudioSupperBassModuleDisableCFG();
extern int GetAudioSupperBassSwitchDisableCFG();
extern int GetAudioSrcInputTVLineInVol(int *l_vol, int *r_vol);
extern int GetAudioSrcInputAVLineInVol(int *l_vol, int *r_vol);
extern int GetAudioSrcInputCOMPLineInVol(int *l_vol, int *r_vol);
extern int GetAudioSrcInputHDMILineInVol(int *l_vol, int *r_vol);
extern int GetAudioSrcInputVGALineInVol(int *l_vol, int *r_vol);
extern int GetAudioSrcInputMPEGLineInVol(int *l_vol, int *r_vol);
extern int GetAudioSrcInputTVLineOutVol(int *l_vol, int *r_vol);
extern int GetAudioSrcInputAVLineOutVol(int *l_vol, int *r_vol);
extern int GetAudioSrcInputCOMPLineOutVol(int *l_vol, int *r_vol);
extern int GetAudioSrcInputHDMILineOutVol(int *l_vol, int *r_vol);
extern int GetAudioSrcInputVGALineOutVol(int *l_vol, int *r_vol);
extern int GetAudioSrcInputMPEGLineOutVol(int *l_vol, int *r_vol);
extern int GetAudioWallEffectTypeCfg();
extern int GetAudioMainVolumeLutBufTVCompensation();
extern int GetAudioMainVolumeLutBufAVCompensation();
extern int GetAudioMainVolumeLutBufCOMPCompensation();
extern int GetAudioMainVolumeLutBufHDMICompensation();
extern int GetAudioMainVolumeLutBufVGACompensation();
extern int GetAudioMainVolumeLutBufMPEGCompensation();
extern int GetAudioMainVolumeLutBufDTVCompensation();
extern int GetAudioNoiseGateEnableCFG();
extern int GetAudioNoiseGateUpperBoundCFG();
extern int GetAudioNoiseGateThresholdCFG();
extern int GetDisableResetSystemFrameSize();
extern int GetDisableReset_System_framesizeCFG();

extern int GetAudioArchitectureTypeCFG();
extern int GetAudioResampleTypeCFG();
extern int GetamAudioDisableFlagCFG();
extern int GetAudioFirstInitSetTypeCFG();

#endif  //__TV_AUDIO_CFG_H__
