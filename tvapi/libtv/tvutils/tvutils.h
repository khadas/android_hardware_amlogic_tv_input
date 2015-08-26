#ifndef __TV_MISC_H__
#define __TV_MISC_H__

#define CC_MIN_ADC_CHANNEL_VAL              (0)
#define CC_MAX_ADC_CHANNEL_VAL              (5)

#define CC_I2C_BUS_ON                       (0)
#define CC_I2C_BUS_OFF                      (1)

#define CPQDB_DB_SIZE_ID       "tv.db.size.id"

struct erase_info_user {
    uint32_t start;
    uint32_t length;
};

extern int *GetFileAttrIntValueStr(const char *fp);
extern int ReadADCSpecialChannelValue(int adc_channel_num);
extern int Tv_MiscRegs(const char *cmd);

extern int TvMisc_SetUserCounterTimeOut(int timeout);
extern int TvMisc_SetUserCounter(int count);
extern int TvMisc_SetUserPetResetEnable(int enable);
extern int TvMisc_SetSystemPetResetEnable(int enable);
extern int TvMisc_SetSystemPetEnable(int enable);
extern int TvMisc_SetSystemPetCounter(int count);
extern void TvMisc_EnableWDT(bool kernelpet_disable, unsigned int userpet_enable, unsigned int kernelpet_timeout, unsigned int userpet_timeout, unsigned int userpet_reset);
extern void TvMisc_DisableWDT(unsigned int userpet_enable);
extern int I2C_WriteNbyte(int i2c_no, int dev_addr, int slave_addr, int len, unsigned char data_buf[]);
extern int I2C_ReadNbyte(int i2c_no, int dev_addr, int slave_addr, int len, unsigned char data_buf[]);
extern int GetTvDBDefineSize();
extern int SetFileAttrValue(const char *fp, const char value[]);
extern int GetFileAttrIntValue(const char *fp);

extern int Get_Fixed_NonStandard(void);
extern int Set_Fixed_NonStandard(int value);

extern int get_hardware_name(char *hardware);

extern int TvMisc_DeleteDirFiles(const char *strPath, int flag);

extern int cfg_get_one_item(const char *key_str, const char *strDelimit, int item_index, char cfg_str[]);
extern int Tv_Utils_CheckFs(void);
extern int Tv_Utils_SetFileAttrStr(const char *file_path, char val_str_buf[]);
extern int Tv_Utils_GetFileAttrStr(const char *file_path, int buf_size, char val_str_buf[]);
extern int Tv_Utils_IsFileExist(const char *file_name);
extern void monitor_info_name_init ( unsigned char *edidbuf );
extern void monitor_info_set_name ( unsigned char *edidbuf );
extern void monitor_info_set_imagesize ( unsigned char *edidbuf );
extern void monitor_info_edid_checksum ( unsigned char *edidbuf );
extern int reboot_sys_by_fbc_edid_info();
extern int reboot_sys_by_fbc_uart_panel_info();
extern int GetPlatformHaveFBCFlag();
extern int GetPlatformHaveDDFlag();

#define CC_PROJECT_INFO_ITEM_MAX_LEN  (64)

typedef struct project_info_s {
    char version[CC_PROJECT_INFO_ITEM_MAX_LEN];
    char panel_type[CC_PROJECT_INFO_ITEM_MAX_LEN];
    char panel_outputmode[CC_PROJECT_INFO_ITEM_MAX_LEN];
    char panel_rev[CC_PROJECT_INFO_ITEM_MAX_LEN];
    char panel_name[CC_PROJECT_INFO_ITEM_MAX_LEN];
    char amp_curve_name[CC_PROJECT_INFO_ITEM_MAX_LEN];
} project_info_t;

extern unsigned int CalCRC32(unsigned int crc, const unsigned char *ptr, unsigned int buf_len);
extern int GetProjectInfo(project_info_t *proj_info_ptr);

//extern void SSMRewriteEdidInfo ( unsigned char *edidbuf );
//extern int HandleEdid ( int op_type, int op_direct, unsigned char edid_buf[], unsigned char def_buf[] );
//extern int  SSMSetHdmiEdid();
#define AML_DBG_REGS_IOC_MAGIC 'R'

typedef struct aml_debug_reg_s {
    unsigned int addr;
    unsigned int val;
    unsigned char mode;
} aml_debug_reg_t;

typedef struct aml_debug_bit_s {
    unsigned int addr;
    unsigned int val;
    unsigned int start;
    unsigned int len;
} aml_debug_bit_t;

/*ioctl for reg*/
#define AMLDBG_IOC_CBUS_REG_RD      _IOR(AML_DBG_REGS_IOC_MAGIC, 0x01, struct aml_debug_reg_s)
#define AMLDBG_IOC_CBUS_REG_WR      _IOW(AML_DBG_REGS_IOC_MAGIC, 0x02, struct aml_debug_reg_s)
#define AMLDBG_IOC_APB_REG_RD       _IOR(AML_DBG_REGS_IOC_MAGIC, 0x03, struct aml_debug_reg_s)
#define AMLDBG_IOC_APB_REG_WR       _IOW(AML_DBG_REGS_IOC_MAGIC, 0x04, struct aml_debug_reg_s)
#define AMLDBG_IOC_AXI_REG_RD       _IOR(AML_DBG_REGS_IOC_MAGIC, 0x05, struct aml_debug_reg_s)
#define AMLDBG_IOC_AXI_REG_WR       _IOW(AML_DBG_REGS_IOC_MAGIC, 0x06, struct aml_debug_reg_s)
#define AMLDBG_IOC_AHB_REG_RD       _IOR(AML_DBG_REGS_IOC_MAGIC, 0x07, struct aml_debug_reg_s)
#define AMLDBG_IOC_AHB_REG_WR       _IOW(AML_DBG_REGS_IOC_MAGIC, 0x08, struct aml_debug_reg_s)
#define AMLDBG_IOC_MPEG_REG_RD      _IOR(AML_DBG_REGS_IOC_MAGIC, 0x09, struct aml_debug_reg_s)
#define AMLDBG_IOC_MPEG_REG_WR      _IOW(AML_DBG_REGS_IOC_MAGIC, 0x0a, struct aml_debug_reg_s)

/*ioctl for bit*/
#define AMLDBG_IOC_CBUS_BIT_RD      _IOR(AML_DBG_REGS_IOC_MAGIC, 0x21, aml_debug_bit_t)
#define AMLDBG_IOC_CBUS_BIT_WR      _IOW(AML_DBG_REGS_IOC_MAGIC, 0x22, aml_debug_bit_t)
#define AMLDBG_IOC_APB_BIT_RD       _IOR(AML_DBG_REGS_IOC_MAGIC, 0x23, aml_debug_bit_t)
#define AMLDBG_IOC_APB_BIT_WR       _IOW(AML_DBG_REGS_IOC_MAGIC, 0x24, aml_debug_bit_t)
#define AMLDBG_IOC_AXI_BIT_RD       _IOR(AML_DBG_REGS_IOC_MAGIC, 0x25, aml_debug_bit_t)
#define AMLDBG_IOC_AXI_BIT_WR       _IOW(AML_DBG_REGS_IOC_MAGIC, 0x26, aml_debug_bit_t)
#define AMLDBG_IOC_AHB_BIT_RD       _IOR(AML_DBG_REGS_IOC_MAGIC, 0x27, aml_debug_bit_t)
#define AMLDBG_IOC_AHB_BIT_WR       _IOW(AML_DBG_REGS_IOC_MAGIC, 0x28, aml_debug_bit_t)
#define AMLDBG_IOC_MPEG_BIT_RD      _IOR(AML_DBG_REGS_IOC_MAGIC, 0x29, aml_debug_bit_t)
#define AMLDBG_IOC_MPEG_BIT_WR      _IOW(AML_DBG_REGS_IOC_MAGIC, 0x30, aml_debug_bit_t)

/*ioctl for gamma*/
#define AMLDBG_IOC_SGR_WR           _IOW(AML_DBG_REGS_IOC_MAGIC, 0X15, int)
#define AMLDBG_IOC_SGG_WR           _IOW(AML_DBG_REGS_IOC_MAGIC, 0X16, int)
#define AMLDBG_IOC_SGB_WR           _IOW(AML_DBG_REGS_IOC_MAGIC, 0X17, int)

#define AMLDBG_IOC_AAPB_REG_RD       _IOR(AML_DBG_REGS_IOC_MAGIC, 0x18, struct aml_debug_reg_s)
#define AMLDBG_IOC_AAPB_REG_WR       _IOW(AML_DBG_REGS_IOC_MAGIC, 0x19, struct aml_debug_reg_s)
#endif  //__TV_MISC_H__
