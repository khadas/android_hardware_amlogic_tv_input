/***
 * haier 360 project
 * ***/
#include "CTv.h"
class CTvHaierDtmb360: public CTv, public CTv2d4GHeadSetDetect::IHeadSetObserver
{
public:
    CTvHaierDtmb360();
    ~CTvHaierDtmb360();
    virtual int OpenTv ( void );
    virtual int StartTvLock ();
    virtual int SetSourceSwitchInput (tv_source_input_t source_input );
    int StartHeadSetDetect();
    virtual void onHeadSetDetect(int state, int para);

private:
    CTv2d4GHeadSetDetect mHeadSet;
};
