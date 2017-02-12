#pragma once

class CCrashDump
{
public :
	CCrashDump()
	{
		_DumpCount = 0;
		_invalid_parameter_handler oldHandler, newHandler;
		newHandler = myInvalidParameterHandler;

		oldHandler = _set_invalid_parameter_handler(newHandler);
		_CrtSetReportMode(_CRT_WARN, 0);
		_CrtSetReportMode(_CRT_ASSERT, 0);
		_CrtSetReportMode(_CRT_ERROR, 0);

		_CrtSetReportHook(_custom_Report_hook);

		_set_purecall_handler(myPurecallHandler);
		_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
		signal(SIGABRT, signalHandler);
		signal(SIGINT, signalHandler);
		signal(SIGILL, signalHandler);
		signal(SIGFPE, signalHandler);
		signal(SIGSEGV, signalHandler);
		signal(SIGTERM, signalHandler);

		SetHandlerDump();
	}

	static void Crash(void)
	{
		int *p = nullptr;
		*p = 0;
	}

	static LONG WINAPI MyExceptionFilter(__in PEXCEPTION_POINTERS pExceptionPointer)
	{
		int iWorkingMemory = 0;
		SYSTEMTIME stNowTime;

		long DumpCount = InterlockedIncrement(&_DumpCount);

		HANDLE hProcess = 0;
		PROCESS_MEMORY_COUNTERS pmc;

		hProcess = GetCurrentProcess();

		if (NULL == hProcess)
			return 0;

		if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
		{
			iWorkingMemory = (int)(pmc.WorkingSetSize / 1024 / 1024);
		}

		CloseHandle(hProcess);

		WCHAR filename[MAX_PATH];

		GetLocalTime(&stNowTime);
		wsprintf(filename, L"Dump_%d%02d%02d_%02d.%02d.%02d_%d)%dMB.dmp", 
			stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, DumpCount, iWorkingMemory);
			
		wprintf(L"\n\n\n!!! Crash Error !!!		%d.%d.%d / %d:%d:%d \n", 
			stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond);

		wprintf(L"Now Save dump file...\n");

		HANDLE hDumpFile = ::CreateFile(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (INVALID_HANDLE_VALUE != hDumpFile)
		{
			_MINIDUMP_EXCEPTION_INFORMATION MinidumpExceptionInformation;

			MinidumpExceptionInformation.ThreadId = ::GetCurrentThreadId();
			MinidumpExceptionInformation.ExceptionPointers = pExceptionPointer;
			MinidumpExceptionInformation.ClientPointers = TRUE;

			MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpWithFullMemory, &MinidumpExceptionInformation, NULL, NULL);
			CloseHandle(hDumpFile);

			wprintf(L"CrashDump Save Finish !");
		}

		return EXCEPTION_EXECUTE_HANDLER;
	}

	static LONG WINAPI RedirectedSetUnhandledExceptionFilter(EXCEPTION_POINTERS *exceptionInfo)
	{
		MyExceptionFilter(exceptionInfo);
		return EXCEPTION_EXECUTE_HANDLER;
	}

	static void SetHandlerDump(void)
	{
		SetUnhandledExceptionFilter(MyExceptionFilter);

		static CAPIHook apiHook("kernel32.dll", "SetUnhandledExceptionFilter", (PROC)RedirectedSetUnhandledExceptionFilter, true);
	}

	static void myInvalidParameterHandler(const wchar_t *expression, const wchar_t *function, const wchar_t *filfile, unsigned int ling, uintptr_t pReserved)
	{
		Crash();
	}

	static int _custom_Report_hook(int ireposttype, char *message, int *returnvalue)
	{
		Crash();
		return true;
	}

	static void myPurecallHandler(void)
	{
		Crash();
	}

	static void signalHandler(int Error)
	{
		Crash();
	}

public :
	static long _DumpCount;
};