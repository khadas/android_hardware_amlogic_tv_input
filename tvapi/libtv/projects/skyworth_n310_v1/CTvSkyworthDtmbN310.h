/***
 * for skyworth project n310
 * 1.atv is 256 programs ,default.
 * 2.dtv can scan between start/end freq
 * 3.more program info,example pid/vid
 * 4.
 * ***/
#include "tv/CTv.h"
class CTvSkyworthDtmbN310: public CTv
{
public:
    CTvSkyworthDtmbN310();
    ~CTvSkyworthDtmbN310();
    virtual int OpenTv ( void );
    virtual int StartTvLock ();
    virtual int atvAutoScan(int videoStd, int audioStd, int searchType);
private:
};
