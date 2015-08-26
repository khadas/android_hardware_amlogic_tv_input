//
//
//  amlogic 2013
//
//  @ Project : tv
//  @ File Name : CTvChannel.h
//  @ Date : 2013-11
//  @ Author :
//
//
#include <utils/Timers.h>
#include "CTvLog.h"
#include <time.h>
//#include <cutils/tztime.h>
#include <sys/times.h>
#include <sys/sysinfo.h>

#define LOG_TAG "CTVTIME"
/**
 *TV时间管理
 */
class CTvTime
{
public:
    CTvTime()
    {
        mDiff = 0;
        mHZ = sysconf( _SC_CLK_TCK );
    }

    /**
     *设定当前时间
     *@param time 当前时间（毫秒单位）
     */
    long getSysUTCTime()
    {
        //struct tm tm;
        //time_t t;
        //int64_t r = mktime_tz(&(tm), NULL);
        //time_t t = time(NULL);
        //LOGD("---------utc t = %ld time t=%ld", r, t);
        return 0;
    }
    void setTime(long t)
    {
        //long utcMS;
        //time(&utcMS);
        //nsecs_t ns = systemTime(CLOCK_REALTIME);
        //nsecs_t tm = ns2s(ns);
        //unsigned long ticks = times(NULL);
        //long tm = ticks/mHZ;
        struct sysinfo s_info;
        int error;
        error = sysinfo(&s_info);

        mDiff = t - s_info.uptime;
        LOGD("--- mDiff=%ld", mDiff);
    }

    /**
     *取得当前时间
     *@return 返回当前时间
     */
    long getTime()
    {
        //long utcMS;
        //time(&utcMS);
        //nsecs_t ns = systemTime(CLOCK_REALTIME);
        //nsecs_t sec = ns2s(ns);

        //unsigned long ticks = times(NULL);
        //long sec = ticks/mHZ;
        struct sysinfo s_info;
        int error;
        error = sysinfo(&s_info);


        LOGD("--- mDiff=%ld, sec=%ld", mDiff, s_info.uptime);
        return s_info.uptime + mDiff;
    }

    /**
     *取得TDT/STT与系统时间的差值
     *@return 返回差值时间
     */
    long getDiffTime()
    {
        return mDiff;
    }

    /**
     *设置TDT/STT与系统时间的差值
     */
    void setDiffTime(long diff)
    {
        this->mDiff = diff;
    }

private:
    long mDiff;
    unsigned long mHZ;
};
