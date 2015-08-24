#ifndef WIN32
    #include <time.h>
    #include <errno.h>
    #include <pthread.h>
    #include <unistd.h>
    #include <sys/time.h>
#else
    #include <windows.h>
    #include <sys/timeb.h>
#endif


#include "times.h"
#include "results.h"


//////////////////////////////////////////////////////////////////////////
// TimeStamp
TimeStamp::TimeStamp(const TimeStamp& other)
{
	m_NanoSeconds = other.m_NanoSeconds;
}

TimeStamp::TimeStamp(double seconds)
{
	m_NanoSeconds = (Int64)(seconds * 1e9);
}

TimeStamp& TimeStamp::operator+=(const TimeStamp& other)
{
	m_NanoSeconds += other.m_NanoSeconds;
	return *this;
}

TimeStamp& TimeStamp::operator-=(const TimeStamp& other)
{
	m_NanoSeconds -= other.m_NanoSeconds;
	return *this;
}


//////////////////////////////////////////////////////////////////////////
// TimeDate
TimeDate::TimeDate()
	: m_Year(1970)
	, m_Month(1)
	, m_Day(1)
	, m_Hours(0)
	, m_Minutes(0)
	, m_Seconds(0)
	, m_NanoSeconds(0)
	, m_TimeZone(0)
{
}