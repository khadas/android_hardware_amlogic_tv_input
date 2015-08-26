#ifndef __TV_KEY_DATA_H__
#define __TV_KEY_DATA_H__

#include <pthread.h>
#include <stdint.h>

#include "../tv/CTvLog.h"
#define LOG_TAG "TvKeyData"

#define SSM_CR_RGBOGO_LEN                           (256)
#define SSM_CR_RGBOGO_CHKSUM_LEN                    (2)
#define SSM_HDMI_PORT_MAX                           (3)
#define SSM_HDMI_EDID_SIZE                          (256)

#define CS_KEY_DATA_NAME_DEV_PATH                   "/sys/class/aml_keys/aml_keys/key_name"
#define CS_KEY_DATA_READ_DEV_PATH                   "/sys/class/aml_keys/aml_keys/key_read"
#define CS_KEY_DATA_WRITE_DEV_PATH                  "/sys/class/aml_keys/aml_keys/key_write"

#define CS_MAC_KEY_NAME                             "mac"
#define CS_BARCODE_KEY_NAME                         "usid"
#define CS_RX_HDCP_KEY_NAME                         "rxhdcp20"
#define CS_PROJECT_ID_KEY_NAME                      "projid"

int ReadKeyData(const char *key_name, unsigned char data_buf[]);
int WriteKeyData(const char *key_name, int wr_size, char data_buf[]);

int KeyData_SaveProjectID(int rw_val);
int KeyData_ReadProjectID();

int KeyData_GetMacAddressDataLen();
int KeyData_ReadMacAddress(unsigned char data_buf[]);
int KeyData_SaveMacAddress(unsigned char data_buf[]);

int KeyData_GetBarCodeDataLen();
int KeyData_ReadBarCode(unsigned char data_buf[]);
int KeyData_SaveBarCode(unsigned char data_buf[]);

int SSMReadHDCPKey(unsigned char hdcp_key_buf[]);
int SSMSaveHDCPKey(unsigned char hdcp_key_buf[]);
int SSMSetHDCPKey();
int SSMGetHDCPKeyDataLen();
int SSMRefreshHDCPKey();


int SSMReadHDMIEdid(int port, unsigned char hdmi_edid_buf[]);
int SSMSetHDMIEdid(int port);

int SSMSaveRGBOGOValue(int offset, int size, unsigned char data_buf[]);
int SSMReadRGBOGOValue(int offset, int size, unsigned char data_buf[]);

int SSMReadAudioNoLinePoints(int offset, int size, unsigned char tmp_buf[]);
int SSMSaveAudioNoLinePoints(int offset, int size, unsigned char tmp_buf[]);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif  //__TV_KEY_DATA_H__
