//
//
//  amlogic 2013
//
//  @ Project : tv
//  @ File Name : CTvBooking.h
//  @ Date : 2013-11
//  @ Author :
//
//


#if !defined(_CTVBOOKING_H)
#define _CTVBOOKING_H

#include "CTvDatabase.h"
#include "CTvProgram.h"
#include "CTvEvent.h"
#include <utils/String8.h>
#include <utils/RefBase.h>
#include <stdlib.h>
#include "CTvLog.h"
#define LOG_TAG "CTvBooking"

// ??????/ ???
class CTvBooking : public LightRefBase<CTvBooking> {
public:
	CTvBooking(CTvDatabase::Cursor &c);
	CTvBooking();
	~CTvBooking();

	static int selectByID(int id, CTvBooking &CtvBook);

	int bookEvent(int evtId, bool bBookFlag);
	int getBookedEventList(Vector<sp<CTvBooking> > &vBv);

	int getBookId()
	{
		return id;
	};
	int getProgramId()
	{
		return programId;
	};
	int getEventId()
	{
		return eventId;
	};
	int getStartTime()
	{
		return start;
	};
	int getDurationTime()
	{
		return duration;
	};
	String8 &getProgName()
	{
		return progName;
	};
	String8 &getEvtName()
	{
		return evtName;
	};

private:
	int deleteBook(int evtIdFlag) ;
	int bookProgram(CTvProgram &prog, CTvEvent &evt);
	int InitFromCursor(CTvDatabase::Cursor &c);
private:
	int id;
	int programId;
	int eventId;
	int flag;
	int status;
	int repeat;
	long start;
	long duration;
	String8 progName;
	String8 evtName;
};

#endif  //_CTVBOOKING_H
