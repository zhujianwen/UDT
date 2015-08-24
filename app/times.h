#ifndef __ZHD_TIMES_H__
#define __ZHD_TIMES_H__


#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "results.h"


#define NPT_DATETIME_YEAR_MIN 1901
#define NPT_DATETIME_YEAR_MAX 2262


//////////////////////////////////////////////////////////////////////////
// TimeStamp class
class TimeStamp
{
public:
	// methods
	TimeStamp(const TimeStamp& other);
	TimeStamp() : m_NanoSeconds(0) {}
	TimeStamp(Int64 nanoseconds) : m_NanoSeconds(nanoseconds) {}
	TimeStamp(double seconds);
	TimeStamp& operator+=(const TimeStamp& other);
	TimeStamp& operator-=(const TimeStamp& other);
	bool operator==(const TimeStamp& other) const { return m_NanoSeconds == other.m_NanoSeconds; }
	bool operator!=(const TimeStamp& other) const { return m_NanoSeconds != other.m_NanoSeconds; }
	bool operator> (const TimeStamp& other) const { return m_NanoSeconds >  other.m_NanoSeconds; }
	bool operator< (const TimeStamp& other) const { return m_NanoSeconds <  other.m_NanoSeconds; }
	bool operator>=(const TimeStamp& other) const { return m_NanoSeconds >= other.m_NanoSeconds; }
	bool operator<=(const TimeStamp& other) const { return m_NanoSeconds <= other.m_NanoSeconds; }

	// accessors
	void SetNanos(Int64 nanoseconds) { m_NanoSeconds = nanoseconds;          }
	void SetMicros(Int64 micros)     { m_NanoSeconds = micros  * 1000;       }
	void SetMillis(Int64 millis)     { m_NanoSeconds = millis  * 1000000;    }
	void SetSeconds(Int64 seconds)   { m_NanoSeconds = seconds * 1000000000; }

	// conversion
	operator double() const               { return (double)m_NanoSeconds/1E9; }
	void FromNanos(Int64 nanoseconds) { m_NanoSeconds = nanoseconds;      }
	Int64 ToNanos() const             { return m_NanoSeconds;             }
	Int64 ToMicros() const            { return m_NanoSeconds/1000;        }
	Int64 ToMillis() const            { return m_NanoSeconds/1000000;     }
	Int64 ToSeconds() const           { return m_NanoSeconds/1000000000;  }

private:
	Int64 m_NanoSeconds;
};


//////////////////////////////////////////////////////////////////////////
// TimeDate class
class TimeDate
{
public:
	// types
	enum Format {
		FORMAT_ANSI,
		FORMAT_W3C,
		FORMAT_RFC_1123,  // RFC 822 updated by RFC 1123
		FORMAT_RFC_1036   // RFC 850 updated by RFC 1036
	};

	enum FormatFlags {
		FLAG_EMIT_FRACTION      = 1,
		FLAG_EXTENDED_PRECISION = 2
	};

	// class methods
	int GetLocalTimeZone();

	// constructors
	TimeDate();
	TimeDate(const TimeStamp& timestamp, bool local=false);

	// methods
	int ChangeTimeZone(int timezone);
	int FromTimeStamp(const TimeStamp& timestamp, bool local=false);
	int ToTimeStamp(TimeStamp& timestamp) const;
	int FromString(const char* date, Format format = FORMAT_ANSI);
	char* ToString(Format format = FORMAT_ANSI, unsigned int flags=0) const;

	// members
	int m_Year;        // year
	int m_Month;       // month of the year (1-12)
	int m_Day;         // day of the month (1-31)
	int m_Hours;       // hours (0-23)
	int m_Minutes;     // minutes (0-59)
	int m_Seconds;     // seconds (0-59)
	int m_NanoSeconds; // nanoseconds (0-999999999)
	int m_TimeZone;    // minutes offset from GMT
};

#endif	// __ZHD_TIMES_H__