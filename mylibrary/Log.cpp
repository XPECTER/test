#include <Windows.h>
#include <ctime>
#include <stdarg.h>
#include <strsafe.h>

#ifdef MYSQL
#include <mysql.h>
#include <my_global.h>
#endif

#include "Log.h"

bool LOG::bConsole = false;
bool LOG::bFile = true;
bool LOG::bDatabase = false;
int LOG::iLogLevel;
volatile __int64 LOG::logNo = 0;
SRWLOCK LOG::_srwLock;
wchar_t g_szLogBuff[1024] = { 0, };
LOG SysLog;

LOG::LOG()
{
	InitializeSRWLock(&this->_srwLock);
}

LOG::~LOG()
{

}

void LOG::logSet(int mode, int logLevel)
{
	if (0 != (mode & OUT_CONSOLE))
		bConsole = true;
	else
		bConsole = false;

	if (0 != (mode & OUT_FILE))
		bFile = true;
	else
		bFile = false;

	if (0 != (mode & OUT_DATABASE))
		bDatabase = true;
	else
		bDatabase = false;

	iLogLevel = logLevel;

	return;
}

void LOG::LogLevelChange(void)
{
	if (LOG::LEVEL_DEBUG == iLogLevel)
	{
		iLogLevel = LOG::LEVEL_WARNING;
		return;
	}
	else if (LOG::LEVEL_WARNING == iLogLevel)
	{
		iLogLevel = LOG::LEVEL_ERROR;
	}
	else
	{
		iLogLevel = LOG::LEVEL_DEBUG;
	}
}

bool LOG::connectDatabase(void)
{
#ifdef MYSQL
	mysql_init(&conn);

	connector = mysql_real_connect(&conn, "localhost", "root", "menistream@2460", "log", 3306, (char *)NULL, 0);
#else
	return false;
#endif
}

void LOG::printLog(WCHAR *category, int logLevel, WCHAR *fmt, ...)
{
	if (logLevel < iLogLevel)
		return;

	tm t;
	WCHAR buf[1024] = { 0, };
	time_t unixTime = time(NULL);
	localtime_s(&t, &unixTime);
	
	va_list vl;
	INT64 templogNo = InterlockedIncrement64(&logNo);
	int buffSize = 0;

	WCHAR szLevel[10] = { 0, };

	switch (logLevel)
	{
	case LEVEL_DEBUG:
		{
			swprintf_s(szLevel, L"DEBUG");
			break;
		}
	case LEVEL_WARNING:
		{
			swprintf_s(szLevel, L"WARNG");
			break;
		}
	case LEVEL_ERROR:
		{
			swprintf_s(szLevel, L"ERROR");
			break;
		}
	}

	buffSize = swprintf_s(buf, 1024, L"[%s][%04d-%02d-%02d %02d:%02d:%02d / %s / %010d]",
		category, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, szLevel, templogNo);

	va_start(vl, fmt);
	StringCchVPrintf(buf + buffSize, 1024 - buffSize, fmt, vl);
	va_end(vl);

	FILE *pf;
	
	if (true == bConsole)
	{
		wprintf_s(L"%s\n", buf);
	}

	if (true == bFile)
	{
		
		WCHAR filename[256] = { 0, };
		swprintf_s(filename, L"%04d%02d_%s.txt", t.tm_year + 1900, t.tm_mon + 1, category);

		AcquireSRWLockExclusive(&_srwLock);
		_wfopen_s(&pf, filename, L"a");
		fputws(buf, pf);
		fputws(L"\n", pf);
		fclose(pf);
		ReleaseSRWLockExclusive(&_srwLock);
	}

#ifdef MYSQL
	if (true == bDatabase)
	{
		char query[256] = { 0, };
		sprintf_s(query, "INSERT INTO ....");
		mysql_query(connector, query);

		// 결과에 따라서 테이블 만들고 다시 넣고 하면 됨.
	}
#endif

	return;
}