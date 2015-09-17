#ifndef __TV_AUDIO_API_H__
#define __TV_AUDIO_API_H__

#include "tvsetting/audio_cfg.h"
#include "audio_alsa.h"
#include "audio_effect.h"

enum CC_AUDIO_SWITCH_STATUS {
	CC_SWITCH_OFF,
	CC_SWITCH_ON,
};

enum CC_AMAUDIO_OUT_MODE {
	CC_AMAUDIO_OUT_MODE_DIRECT,
	CC_AMAUDIO_OUT_MODE_MIX,
};

static const  int  CC_SPDIF_MODE_PCM = 0;
static const int CC_SPDIF_MODE_RAW = 1;

static const int  CC_MIN_SOUND_VOL  = 0;
static const int  CC_MAX_SOUND_VOL   =   (100);
static const int  CC_DEF_SOUND_VOL                     =   (30);
static const int  CC_MIN_SUPPERBASS_VOL             =      (0);
static const int  CC_MAX_SUPPERBASS_VOL               =    (100);
static const int  CC_DEF_SUPPERBASS_VOL                =   (50);

static const int  CC_VOLUME_COMPENSATION_MIN_VAL   =       (-20);
static const int  CC_VOLUME_COMPENSATION_MAX_VAL     =     (20);
static const int  CC_VOLUME_COMPENSATION_DEF_VAL        =  (0);

static const int  CC_DEF_BASS_TREBLE_VOL            =      50;
static const int  CC_EQ_BASS_IND               =           1;
static const int  CC_EQ_TREBLE_IND           =             3;
static const int  CC_EQ_DEF_UI_MIN_GAIN     =              0;
static const int  CC_EQ_DEF_UI_MAX_GAIN      =             100;
static const int  CC_LUT_SEL_TV                  =         0;
static const int  CC_LUT_SEL_AV                =           1;
static const int  CC_LUT_SEL_COMP        =                 2;
static const int  CC_LUT_SEL_HDMI           =              3;
static const int  CC_LUT_SEL_VGA             =             4;
static const int  CC_LUT_SEL_MPEG          =               5;
static const int  CC_LUT_SEL_COUNT       =   6;


enum CC_AUD_SOUND_MODE {
	CC_SOUND_MODE_START = 0,
	CC_SOUND_MODE_STD = 0,
	CC_SOUND_MODE_MUSIC,
	CC_SOUND_MODE_NEWS,
	CC_SOUND_MODE_THEATER,
	CC_SOUND_MODE_USER,
	CC_SOUND_MODE_END = CC_SOUND_MODE_USER
};

enum CC_AUD_EQ_MODE {
	CC_EQ_MODE_START = 0,
	CC_EQ_MODE_NOMAL = 0,
	CC_EQ_MODE_POP,
	CC_EQ_MODE_JAZZ,
	CC_EQ_MODE_ROCK,
	CC_EQ_MODE_CLASSIC,
	CC_EQ_MODE_DANCE,
	CC_EQ_MODE_PARTY,
	CC_EQ_MODE_BASS,
	CC_EQ_MODE_TREBLE,
	CC_EQ_MODE_CUSTOM,
	CC_EQ_MODE_END = CC_EQ_MODE_CUSTOM
};
class CTvAudio {
public:
	CTvAudio();
	~CTvAudio();
	//Audio Mute
	int SetAudioMuteForSystem(int);
	int GetAudioMuteForSystem();
	int SetAudioMuteForTv(int );
	int GetAudioMuteForTv();
	int SetAudioAVOutMute(int muteStatus);
	int GetAudioAVOutMute();
	int SetAudioSPDIFMute(int muteStatus);
	int GetAudioSPDIFMute();

	//Audio Master Volume
	int SetAudioMasterVolume(int tmp_vol);
	int GetAudioMasterVolume();
	int SaveCurAudioMasterVolume(int tmp_vol);
	int GetCurAudioMasterVolume();

	//Audio Balance
	int SetAudioBalance(int tmp_val);
	int GetAudioBalance();
	int SaveCurAudioBalance(int tmp_val);
	int GetCurAudioBalance();

	//Audio SupperBass Volume
	int SetAudioSupperBassVolume(int tmp_vol);
	int GetAudioSupperBassVolume();
	int SaveCurAudioSupperBassVolume(int tmp_vol);
	int GetCurAudioSupperBassVolume();

	//Audio SupperBass Switch
	int SetAudioSupperBassSwitch(int tmp_val);
	int GetAudioSupperBassSwitch();
	int SaveCurAudioSupperBassSwitch(int tmp_val);
	int GetCurAudioSupperBassSwitch();

	//Audio SRS Surround switch
	int SetAudioSRSSurround(int tmp_val);
	int GetAudioSRSSurround();
	int SaveCurAudioSrsSurround(int tmp_val);
	int GetCurAudioSRSSurround();

	//Audio SRS Dialog Clarity
	int SetAudioSrsDialogClarity(int tmp_val);
	int GetAudioSrsDialogClarity();
	int SaveCurAudioSrsDialogClarity(int tmp_val);
	int GetCurAudioSrsDialogClarity();

	//Audio SRS Trubass
	int SetAudioSrsTruBass(int tmp_val);
	int GetAudioSrsTruBass();
	int SaveCurAudioSrsTruBass(int tmp_val);
	int GetCurAudioSrsTruBass();

	//Audio SPDIF switch
	int SetAudioSPDIFSwitch(int tmp_val);
	int GetCurAudioSPDIFSwitch();
	int SaveCurAudioSPDIFSwitch(int tmp_val);

	//Audio SPDIF Mode
	int SetAudioSPDIFMode(int tmp_val);
	int GetCurAudioSPDIFMode();
	int SaveCurAudioSPDIFMode(int tmp_val);

	//Audio Bass
	int SetAudioBassVolume(int tmp_vol);
	int GetAudioBassVolume();
	int SaveCurAudioBassVolume(int tmp_vol);
	int GetCurAudioBassVolume();

	//Audio Treble
	int SetAudioTrebleVolume(int tmp_vol);
	int GetAudioTrebleVolume();
	int SaveCurAudioTrebleVolume(int tmp_vol);
	int GetCurAudioTrebleVolume();

	//Audio Sound Mode
	int SetAudioSoundMode(int tmp_val);
	int GetAudioSoundMode();
	int SaveCurAudioSoundMode(int tmp_val);
	int GetCurAudioSoundMode();

	//Audio Wall Effect
	int SetAudioWallEffect(int tmp_val);
	int GetAudioWallEffect();
	int SaveCurAudioWallEffect(int tmp_val);
	int GetCurAudioWallEffect();

	//Audio EQ Mode
	int SetAudioEQMode(int tmp_val);
	int GetAudioEQMode();
	int SaveCurAudioEQMode(int tmp_val);
	int GetCurAudioEQMode();

	//Audio EQ Gain
	int GetAudioEQRange(int range_buf[]);
	int GetAudioEQBandCount();
	int SetAudioEQGain(int gain_buf[]);
	int GetAudioEQGain(int gain_buf[]);
	int GetCurAudioEQGain(int gain_buf[]);
	int SaveCurAudioEQGain(int gain_buf[]);
	int SetAudioEQSwitch(int switch_val);

	// amAudio
	int OpenAmAudio(unsigned int sr, int output_device);
	int CloseAmAudio(void);
	int SetAmAudioInputSr(unsigned int sr, int output_device);
	int SetAmAudioOutputMode(int mode);
	int SetAmAudioMusicGain(int gain);
	int SetAmAudioLeftGain(int gain);
	int SetAmAudioRightGain(int gain);

	int SetAudioVolumeCompensationVal(int tmp_vol_comp_val);
	int GetAudioVolumeCompensationVal();

	int AudioLineInSelectChannel(int audio_channel);
	int AudioSetLineInCaptureVolume(int l_vol, int r_vol);
	int SetKalaokIO(int level);

	int AudioHandleHeadsetPlugIn();
	int AudioHandleHeadsetPullOut();
	int amAudioSetDoubleOutputSwitch(int en_val);
	int setAudioPcmPlaybackVolume(int val);
	int UsbAudioOuputModuleSwitch(int sw);
	int GetAmAudioSetDoubleOutputSwitch();
	int GetCurUsbAudioOuputModuleSwitch();
	int amAudioSetTvDoubleOutput(int en_val, unsigned int sr, int input_device, int output_device);

	void AudioCtlInitializeLoad();
	void AudioCtlInit();
	void AudioCtlUninit();
	void AudioSetVolumeDigitLUTBuf(int lut_sel_flag, int add_srs_gain_flag, int add_eq_gain_flag, int audio_std);
	int openTvAudio(int sr);

	int AudioDataInitForDtv();
	int AudioDataInit(int sr, bool enableNoiseGate);
	int AudioDataUnInit();
	int AudioChangeSampleRate(int sr);
	int AudioSetAudioInSource(int audio_src_in_type);
	int AudioSetAudioSourceType(int source_type);
	int AudioSSMRestoreDefaultSetting();
	int AudioRegisterInterface();
	int AudioGetSessionId();
	int AudioNewAudioEffects(void);
	void AudioFreeAudioEffects(void);
	int SetAudioDumpDataFlag(int tmp_flag);
	int GetAudioDumpDataFlag();
	int SetAudioLeftRightMode(unsigned int mode);
	unsigned int GetAudioLeftRightMode();
	int AudioSetMicroPhoneSwitch(int switch_val);
	void AudioSetNoiseGateThreshold(int thresh);
	int AudioSetRecordDataZero(bool flag);
	int SwitchAVOutBypass (int);
	int SetAudioSwitchIO(int value);
	int SetOutput_Swap(int value);

private:
	void RealHandleVolumeDigitLUTBuf(int lut_buf_type, int lut_sel_flag,
									 int vol_buf[], int add_srs_gain_flag, int add_eq_gain_flag);
	int handleAudioSPDIFMute(int muteStatus);
	int LoadCurAudioSPDIFMode();
	int LoadCurAudioMasterVolume();
	int LoadCurAudioBalance();
	int LoadCurAudioSupperBassVolume();
	int LoadCurAudioSupperBassSwitch();
	int LoadCurAudioSrsSurround();
	int LoadCurAudioSrsDialogClarity();
	void RealSetSRS();
	int LoadCurAudioSPDIFSwitch();
	void HanldeSupperBassSRSSpeakerSize();
	int LoadCurAudioSoundMode();
	int LoadCurAudioEQMode();
	int LoadCurAudioSrsTruBass();
	int SetDacGain();
	int RealSaveCurAudioBassVolume(int, int);
	int LoadCurAudioBassVolume();
	int RealSaveCurAudioTrebleVolume(int, int);
	int LoadCurAudioTrebleVolume();
	int HandleTrebleBassVolume();
	int LoadCurAudioWallEffect();
	int RealReadCurAudioEQGain(int *);
	int RealSaveCurAudioEQGain(int *, int);
	int LoadCurAudioEQGain();
	int MappingEQGain(int *, int *, int);
	int RestoreToAudioDefEQGain(int *);
	int CheckAudioEQGain(int *, int);
	int GetCustomEQGain(int *);
	int SetCustomEQGain();
	int AudioSetEQGain(int *);
	int handleEQGainBeforeSet(int *, int *);
	int RealSetEQGain(int *);
	int SetSpecialModeEQGain(int);
	int SetSpecialIndexEQGain(int, int);
	int SaveSpecialIndexEQGain(int, int);
	void LoadAudioCtl();
	void InitSetAudioCtl();
	int GetAudioFirstInitSetFlag();
	int HanldeAudioInputSr(unsigned int);
	int waitServiceManager();
	void *AudioCtlInitSetMain(void *);
	void AudioCtlInitializeSet();
	int GetEQMinGainVal();
	int GetEQMaxGainVal();
	int GetEQUIMinGainVal();
	int GetEQUIMaxGainVal();
	int GetBassUIMinGainVal();
	int GetBassUIMaxGainVal();
	int GetTrebleUIMinGainVal();
	int GetTrebleUIMaxGainVal();
	int MappingLine(int, int, int, int, int);
	int MappingTrebleBassAndEqualizer(int, int, int, int);
	void HandleSupperBassSwicth();
	int InitSetTvAudioCard();
	int UnInitSetTvAudioCard();
	int SetSPDIFMode(int mode_val);


	CAudioAlsa mAudioAlsa;
	CAudioEffect mAudioEffect;
	int mAudioMuteStatusForSystem;
	int mAudioMuteStatusForTv;

	int mCurAudioMasterVolume;
	int mCurAudioBalance;
	int mCurAudioSupperBassVolume;
	int mCurAudioSupperBassSwitch;
	int mCurAudioSRSSurround;
	int mCurAudioSrsDialogClarity;
	int mCurAudioSrsTruBass;
	int mCurAudioSPDIFSwitch;
	int mCurAudioSPDIFMode;
	int mCurAudioBassVolume;
	int mCurAudioTrebleVolume;
	int mCurAudioSoundMode;
	int mCurAudioWallEffect;
	int mCurAudioEQMode;
	int mCustomAudioMasterVolume;
	int mCustomAudioBalance;
	int mCustomAudioSupperBassVolume;
	int mCustomAudioSupperBassSwitch;
	int mCustomAudioSRSSurround;
	int mCustomAudioSrsDialogClarity;
	int mCustomAudioSrsTruBass;
	int mCustomAudioBassVolume;
	int mCustomAudioTrebleVolume;
	int mCustomAudioSoundMode;
	int mCustomAudioWallEffect;
	int mCustomAudioEQMode;
	int mCustomAudioSoundEnhancementSwitch;
	volatile int mVolumeCompensationVal;
};
#endif  //__TV_AUDIO_API_H__
