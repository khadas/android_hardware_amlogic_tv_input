#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "CBlobDeviceE2prom.h"
#include "../tvconfig/tvconfig.h"
#include "../tvutils/tvutils.h"
#include "CTvLog.h"

#define LOG_TAG "CBlobDeviceE2prom"

#define msleep(x) usleep(x*1000)

CBlobDeviceE2prom::CBlobDeviceE2prom()
{

}

CBlobDeviceE2prom::~CBlobDeviceE2prom()
{
}

int CBlobDeviceE2prom::WriteBytes(int offset, int size, unsigned char *buf)
{
    return 0;
}
int CBlobDeviceE2prom::ReadBytes(int offset, int size, unsigned char *buf)
{
    return 0;
}
int CBlobDeviceE2prom::EraseAllData()
{
    return 0;
}
int CBlobDeviceE2prom::InitCheck()
{
    return 0;
}
int CBlobDeviceE2prom::OpenDevice()
{
    return 0;
}
int CBlobDeviceE2prom::CloseDevice()
{
    return 0;
}
//int CBlobDeviceE2prom::InitCheck() {
/* int i, tmp_dev_total_size = 0, tmp_offset = 0, rw_check_count = 4;
 unsigned char *tmp_buf = NULL;
 unsigned char w_ch = 0, r_ch = 0;

 SetWriteProtectStatus (CC_SSM_PROTECT_ON);

 if (ValidOperateCheck() < 0) {
     return -1;
 }

 tmp_offset = E2P_DEV_RW_TEST_OFFSET;

 if (tmp_offset >= 0) {
     for (i = 0; i < rw_check_count; i++) {
         w_ch = 0x5A;
         WriteNBytes(tmp_offset, 1, &w_ch);

         r_ch = 0x00;
         ReadNBytes(tmp_offset, 1, &r_ch);

         if (r_ch != w_ch) {
             LOGE("%s, Device rw check error, write %d, read %d.\n", "TV", w_ch, r_ch);
             return -1;
         }

         w_ch = 0xA5;
         WriteNBytes(tmp_offset, 1, &w_ch);

         r_ch = 0x00;
         ReadNBytes(tmp_offset, 1, &r_ch);

         if (r_ch != w_ch) {
             LOGE("%s, Device rw check error, write %d, read %d.\n", "TV", w_ch, r_ch);
             return -1;
         }
     }
 }

 if (device_buf != NULL) {
     delete device_buf;
     device_buf = NULL;
 }

 if (device_use_buffer) {
     tmp_dev_total_size = GetDeviceTotalSize();
     if (tmp_dev_total_size <= 0) {
         LOGE("%s, Device file size must be more than 0.\n", "TV");
         return -1;
     }

     tmp_buf = new unsigned char[tmp_dev_total_size];
     if (tmp_buf != NULL) {
         if (ReadNBytes(0, tmp_dev_total_size, tmp_buf) < 0) {
             delete tmp_buf;
             tmp_buf = NULL;

             LOGE("%s, Read data from device error when read data to device rw buffer!!!.\n", "TV");
             return -1;
         }

         device_buf = new unsigned char[tmp_dev_total_size];
         if (device_buf != NULL) {
             memcpy((void *) device_buf, (void *) tmp_buf, tmp_dev_total_size);
         }

         delete tmp_buf;
         tmp_buf = NULL;
     }
 }
*/
//    return 0;
//}

//int CBlobDeviceE2prom::OpenDevice() {
/*int device_fd = CC_ERR_FILE_HANDLE;

if (gFilePathBuf == NULL) {
    LOGE("%s, Device file name is NULL.\n", "TV");
    return CC_ERR_FILE_HANDLE;
}

device_fd = open(gFilePathBuf, O_RDWR);
if (device_fd < 0) {
    LOGE("%s, Open device file \"%s\" error: %s.\n", "TV", gFilePathBuf, strerror(errno));
    return CC_ERR_FILE_HANDLE;
}

return device_fd;*/
//    return 0;
//}

//int CBlobDeviceE2prom::CloseDevice() {
/*if (*device_fd >= 0) {
    close(*device_fd);
    *device_fd = CC_ERR_FILE_HANDLE;
}*/

//    return 0;
//}

/*int CBlobDeviceE2prom::SetWriteProtectStatus(int protect_status) {
}*/

/*int CBlobDeviceE2prom::CheckDeviceWrAvaliable(int offset, int len) {
    int tmp_dev_start_offset = 0;
    int tmp_dev_end_offset = 0;

    GetDeviceRWStartOffset(&tmp_dev_start_offset);
    if (tmp_dev_start_offset < 0) {
        LOGE("%s, Device file r/w start offset must be greater than or euqal to 0.\n", "TV");
        return -1;
    }

    GetDeviceRWEndOffset(&tmp_dev_end_offset);
    if (tmp_dev_end_offset < 0) {
        LOGE("%s, Device file r/w end offset must be more than 0.\n", "TV");
        return -1;
    }

    if (len <= 0) {
        LOGE("%s, The w/r length should be more than 0.\n", "TV");
        return -1;
    }

    if ((len + tmp_dev_start_offset + offset) > tmp_dev_end_offset + 1) {
        LOGE("%s, w/r would be overflow!!! len = %d, start offset = %d, offset = %d, end offset = %d.\n", "TV", len, tmp_dev_start_offset, offset, tmp_dev_end_offset);
        return -1;
    }

    if (ValidOperateCheck() < 0) {
        return -1;
    }

    return 0;
}

/*int CBlobDeviceE2prom::WriteSpecialBytes(int offset, int len, unsigned char data_buf[]) {
    int i = 0, tmp_ret = 0;
    int tmp_dev_w_page_size = 0;
    int tmp_dev_slave_addr = 0;
    int device_fd = CC_ERR_FILE_HANDLE;
    struct i2c_rdwr_ioctl_data ctl_data;
    struct i2c_msg msg;
    unsigned char msg_buf[E2P_MSG_BUF_SIZE];

    //GetDeviceWritePageSize(&tmp_dev_w_page_size);
    //if (len > tmp_dev_w_page_size) {
    //    LOGE("%s, The write length should be less than page size(%d).\n", "TV", tmp_dev_w_page_size);
    //    return -1;
    //}

    if (CheckDeviceWrAvaliable(offset, len) < 0) {
        return -1;
    }

    device_fd = OpenDevice();
    if (device_fd == CC_ERR_FILE_HANDLE) {
        return -1;
    }

    memset((void *) msg_buf, 0, E2P_MSG_BUF_SIZE);

    msg_buf[0] = (unsigned char) (offset >> 8);
    msg_buf[1] = (unsigned char) (offset & 0x00ff);

    memcpy((void *) &msg_buf[2], data_buf, len);

    //GetDeviceSlaveAddress(&tmp_dev_slave_addr);

    //showboz
    msg.addr = //
    //tmp_dev_slave_addr//0;
    msg.flags = I2C_M_WR;
    msg.len = 2 + len;
    msg.buf = msg_buf;
    ctl_data.nmsgs = 1;
    ctl_data.msgs = &msg;

    SetWriteProtectStatus (CC_SSM_PROTECT_OFF);

    tmp_ret = ioctl(device_fd, I2C_RDWR, &ctl_data);

    SetWriteProtectStatus (CC_SSM_PROTECT_ON);

    if (tmp_ret < 0) {
        LOGE("%s, ioctl failed, error %d (%s).\n", "TV", errno, strerror(errno));

        CloseDevice(&device_fd);
        return -1;
    }

    if (device_use_buffer && device_buf != NULL) {
        memcpy((void *) (device_buf + offset), (void *) data_buf, len);
    }

    CloseDevice(&device_fd);

    msleep(10);

    return 0;
}*/

/*int CBlobDeviceE2prom::ReadSpecialBytes(int offset, int len, unsigned char data_buf[]) {
    int i = 0, tmp_ret = 0;
    int device_fd = CC_ERR_FILE_HANDLE;
    int tmp_dev_slave_addr = 0;
    struct i2c_rdwr_ioctl_data ctl_data;
    struct i2c_msg msg;
    unsigned char msg_buf[E2P_MSG_BUF_SIZE];

    if (CheckDeviceWrAvaliable(offset, len) < 0) {
        return -1;
    }

    if (device_use_buffer && device_buf != NULL) {
        memcpy((void *) data_buf, (void *) (device_buf + offset), len);
        return 0;
    }

    device_fd = OpenDevice();
    if (device_fd == CC_ERR_FILE_HANDLE) {
        return -1;
    }

    //GetDeviceSlaveAddress(&tmp_dev_slave_addr);

    msg_buf[0] = (unsigned char) (offset >> 8);
    msg_buf[1] = (unsigned char) (offset & 0x00ff);
    msg.addr = tmp_dev_slave_addr;
    msg.flags = I2C_M_WR;
    msg.len = 2;
    msg.buf = msg_buf;
    ctl_data.nmsgs = 1;
    ctl_data.msgs = &msg;

    tmp_ret = ioctl(device_fd, I2C_RDWR, &ctl_data);

    if (tmp_ret < 0) {
        LOGE("%s, ioctl failed(erase), error %d (%s).\n", "TV", errno, strerror(errno));
        CloseDevice(&device_fd);
        return -1;
    }

    msg.addr = tmp_dev_slave_addr;
    msg.flags |= I2C_M_RD;
    msg.len = len;
    msg.buf = data_buf;
    ctl_data.nmsgs = 1;
    ctl_data.msgs = &msg;
    tmp_ret = ioctl(device_fd, I2C_RDWR, &ctl_data);
    if (tmp_ret < 0) {
        LOGE("%s, ioctl failed, error %d (%s).\n", "TV", errno, strerror(errno));
        CloseDevice(&device_fd);
        return -1;
    }

    CloseDevice(&device_fd);

    msleep(10);

    return 0;
}

int CTvSettingDeviceE2prom::ValidOperateCheck() {
    int tmp_dev_total_size = 0;
    int tmp_dev_start_offset = 0;
    int tmp_dev_end_offset = 0;

    if (gFilePathBuf == NULL) {
        LOGE("%s, Device file name is NULL.\n", "TV");
        return -1;
    }

    if (IsFileExist(gFilePathBuf) < 0) {
        LOGE("%s, Device file is not exist.\n", "TV");
        return -1;
    }

    tmp_dev_total_size = GetDeviceTotalSize();
    if (tmp_dev_total_size < 0) {
        LOGE("%s, Device file total size must be greater than or euqal to 0.\n", "TV");
        return -1;
    }

    //GetDeviceRWStartOffset(&tmp_dev_start_offset);
    //if (tmp_dev_start_offset < 0) {
    //    LOGE("%s, Device file r/w start offset must be greater than or euqal to 0.\n", "TV");
    //    return -1;
    //}

    //GetDeviceRWEndOffset(&tmp_dev_end_offset);
    //if (tmp_dev_end_offset < 0) {
    //    LOGE("%s, Device file r/w end offset must be more than 0.\n", "TV");
    //    return -1;
    //}

    //if (tmp_dev_end_offset - tmp_dev_start_offset + 1 > tmp_dev_total_size) {
    //    LOGE("%s, end offset (%d) - start offset (%d) + 1 is more than total size(%d).\n", "TV", tmp_dev_end_offset, tmp_dev_start_offset, tmp_dev_total_size);
    //    return -1;
    //}

    return 0;
}*/
