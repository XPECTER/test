#pragma once

#define dfPROFILER_THREAD_MAX 300
#define dfPROFILER_SAMPLE_MAX 50
#define dfTAG_LENGTH	64

//#define ON_PROFILING

#ifdef ON_PROFILING
#define PROFILING_BEGIN(_tag) Profiler_Begin(_tag)
#define PROFILING_END(_tag) Profiler_End(_tag)
#else
#define PROFILING_BEGIN(X)
#define PROFILING_END(X)
#endif

typedef struct st_PROFILE_SAMPLE
{
	BOOL _used;

	WCHAR _tag[dfTAG_LENGTH];

	LARGE_INTEGER _startProcessTime;
	double _totalProcessTime;
	double _minProcessTime[2];
	double _maxProcessTime[2];

	__int64 _callCount;
}PROFILE_SAMPLE;

typedef struct st_PROFILE_TRHEAD
{
	long _threadID;
	PROFILE_SAMPLE _sample[dfPROFILER_SAMPLE_MAX];
}PROFILE_THREAD;


void Profiler_Init(void);
void Profiler_Clear(void);

void Profiler_Begin(WCHAR *_szTag);
void Profiler_End(WCHAR *_szTag);

void Profiler_Print(void);




