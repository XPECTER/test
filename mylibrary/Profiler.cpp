#include <Windows.h>
#include <wchar.h>
#include <stdio.h>
#include "Profiler.h"

PROFILE_THREAD profileArray[dfPROFILER_THREAD_MAX];
CRITICAL_SECTION _csProfile;
LARGE_INTEGER frequency;
double microFrequency;

void Profiler_Init(void)
{
	InitializeCriticalSection(&_csProfile);
	QueryPerformanceFrequency(&frequency);

	microFrequency = (double)frequency.QuadPart / 1000000;

	PROFILE_SAMPLE *sample = NULL;

	for (int i = 0; i < dfPROFILER_THREAD_MAX; ++i)
	{
		profileArray[i]._threadID = -1;

		for (int j = 0; j < dfPROFILER_SAMPLE_MAX; ++j)
		{
			sample = &(profileArray[i]._sample[j]);
			
			sample->_used = FALSE;
			*sample->_tag = 0;
			sample->_startProcessTime.QuadPart = 0;
			sample->_totalProcessTime = 0;
			sample->_minProcessTime[0] = 0x7fffffff;
			sample->_minProcessTime[1] = 0x7fffffff;
			sample->_maxProcessTime[0] = 0;
			sample->_maxProcessTime[1] = 0;
		}
	}
	return;
}

void Profiler_Clear(void)
{
	PROFILE_SAMPLE *sample = NULL;

	for (int i = 0; i < dfPROFILER_THREAD_MAX; ++i)
	{
		if (-1 == profileArray[i]._threadID)
			break;

		for (int j = 0; j < dfPROFILER_SAMPLE_MAX; ++j)
		{
			sample = &(profileArray[i]._sample[j]);

			if (TRUE == sample->_used)
			{
				sample->_startProcessTime.QuadPart = 0;
				sample->_totalProcessTime = 0;
				sample->_minProcessTime[0] = 0x7fffffff;
				sample->_minProcessTime[1] = 0x7fffffff;
				sample->_maxProcessTime[0] = 0;
				sample->_maxProcessTime[1] = 0;
			}
			else
				break;
		}
	}

	return;
}


void Profiler_Begin(WCHAR *_szTag)
{
	PROFILE_SAMPLE *sample = NULL;
	PROFILE_THREAD *profile = NULL;

	long threadID = (long)GetCurrentThreadId();
	int index = -1;

	for (int i = 0; i < dfPROFILER_THREAD_MAX; ++i)
	{
		if (-1 == profileArray[i]._threadID)
		{
			if (-1 != InterlockedExchange(&profileArray[i]._threadID, threadID))
			{
				throw 1;
			}

			index = i;
			break;
		}
		else if (threadID == profileArray[i]._threadID)
		{
			index = i;
			break;
		}
	}

	if (-1 == index)
		throw 1;

	profile = &profileArray[index];

	for (int i = 0; i < dfPROFILER_SAMPLE_MAX; ++i)
	{
		sample = &(profile->_sample[i]);

		if (FALSE == sample->_used)
		{
			sample->_used = TRUE;

			wmemcpy_s(sample->_tag, dfTAG_LENGTH, _szTag, wcslen(_szTag));
			sample->_callCount++;
			QueryPerformanceCounter(&(sample->_startProcessTime));
			return;
		}
		else if (0 == wcscmp(sample->_tag, _szTag))
		{
			if (0 != sample->_startProcessTime.QuadPart)
				throw 1;

			sample->_callCount++;
			QueryPerformanceCounter(&(sample->_startProcessTime));
			return;
		}
	}

	return;
}


void Profiler_End(WCHAR *_szTag)
{
	PROFILE_SAMPLE *sample = NULL;
	PROFILE_THREAD *profile = NULL;

	LARGE_INTEGER endProcessTime;
	QueryPerformanceCounter(&endProcessTime);

	long threadID = (long)GetCurrentThreadId();
	int index = -1;

	for (int i = 0; i < dfPROFILER_THREAD_MAX; ++i)
	{
		if (threadID == profileArray[i]._threadID)
		{
			index = i;
			break;
		}
	}

	if (-1 == index)
		throw 2;

	profile = &profileArray[index];

	double diffTime = 0;
	for (int i = 0; i < dfPROFILER_SAMPLE_MAX; ++i)
	{
		sample = &(profile->_sample[i]);

		if (0 == wcscmp(sample->_tag, _szTag))
		{
			if (0 == sample->_startProcessTime.QuadPart)
				throw 1;

			diffTime = ((double)endProcessTime.QuadPart - (double)sample->_startProcessTime.QuadPart) / (double)frequency.QuadPart * 1000000;

			sample->_totalProcessTime += diffTime;
			
			if (diffTime < sample->_minProcessTime[1])
			{
				if (diffTime < sample->_minProcessTime[0])
				{
					sample->_minProcessTime[1] = sample->_minProcessTime[0];
					sample->_minProcessTime[0] = diffTime;
				}
				else
				{
					sample->_minProcessTime[1] = diffTime;
				}
			}

			if (diffTime > sample->_maxProcessTime[1])
			{
				if (diffTime > sample->_maxProcessTime[0])
				{
					sample->_maxProcessTime[1] = sample->_maxProcessTime[0];
					sample->_maxProcessTime[0] = diffTime;
				}
				else
				{
					sample->_maxProcessTime[1] = diffTime;
				}
			}

			sample->_startProcessTime.QuadPart = 0;
			return;
		}
		else if (FALSE == sample->_used)
		{
			// Â¦À» ¾È¸ÂÃè°Å³ª tag°¡ Æ²·È´Ù.
			throw 3;
		}
	}

	return;
}

void Profiler_Print(void)
{
	FILE *pf = NULL;
	errno_t err;
	PROFILE_SAMPLE *sample = NULL;
	PROFILE_THREAD *profile = NULL;
	long threadID = 0;

	err = _wfopen_s(&pf, L"Profiling_Result.txt", L"w,ccs=UNICODE");
	if (0 != err)
		return;


	fputws(L"  ThreadID |                Name  |      Average  |         Min   |         Max   |         Call |\n", pf);
	fputws(L"--------------------------------------------------------------------------------------------------\n", pf);

	for (int i = 0; i < dfPROFILER_THREAD_MAX; ++i)
	{
		threadID = profileArray[i]._threadID;
		if (-1 == threadID)
			break;

		profile = &profileArray[i];
		
		
		for (int j = 0; j < dfPROFILER_SAMPLE_MAX; ++j)
		{
			sample = &profile->_sample[j];

			if (FALSE == sample->_used)
				break;

			fwprintf_s(pf, L"  %8d | %20s |  %10.4lf%c |  %10.4lf%c |  %10.4lf%c | %18d |\n",
				threadID, sample->_tag, sample->_totalProcessTime / sample->_callCount, L'§Á', sample->_minProcessTime[1], L'§Á', sample->_maxProcessTime[1], L'§Á', sample->_callCount);
		}
		fputws(L"--------------------------------------------------------------------------------------------------\n", pf);
	}

	fclose(pf);
	return;
}

