#ifndef PERFORMANCE_H
#define PERFORMANCE_H

#include <string>
#include <cstdio>
#include <time.h>
#if defined(WIN32)||defined(_MSC_VER)
#include <windows.h>
#else
#  include <sys/time.h>
#endif
#if defined(WIN32)||defined(_MSC_VER)
int gettimeofday(struct timeval *tp, void *tzp)
{
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;
	GetLocalTime(&wtm);
	tm.tm_year = wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm.tm_isdst = -1;
	clock = mktime(&tm);
	tp->tv_sec = clock;
	tp->tv_usec = wtm.wMilliseconds * 1000;
	return (0);
}
#endif
using namespace std;
class Performance
{
public:
	Performance(const char* name):name_(name==nullptr?"":name)
	{
		gettimeofday(&time_,NULL);
	}
	~Performance()
	{
		struct timeval cur_time;
		gettimeofday(&cur_time,NULL);
		long us = (cur_time.tv_sec - time_.tv_sec)*1e6 + (cur_time.tv_usec - time_.tv_usec);
		printf("%s cast time %ld us\n", name_.c_str(),us);
		
	}
private:
	string name_;
	struct timeval time_;
	
};

#endif