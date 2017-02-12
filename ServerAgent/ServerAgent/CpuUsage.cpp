#include "stdafx.h"
#include "CpuUsage.h"


CCpuUsage::CCpuUsage(HANDLE hProcess)
{
	if (INVALID_HANDLE_VALUE == hProcess)
		this->_hProcess = GetCurrentProcess();
	else
		this->_hProcess = hProcess;

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	this->_iNumberOfProcessors = systemInfo.dwNumberOfProcessors;	// included hyper threading func

	_dProcessorTotal = 0;
	_dProcessorUser = 0;
	_dProcessorKernel = 0;

	_ftProcessorLastIdle.QuadPart = 0;
	_ftProcessorLastKernel.QuadPart = 0;
	_ftProcessorLastUser.QuadPart = 0;

	_dProcessTotal = 0;
	_dProcessKernel = 0;
	_dProcessUser = 0;

	_ftProcessLastTime.QuadPart = 0;
	_ftProcessLastKernel.QuadPart = 0;
	_ftProcessLastUser.QuadPart = 0;

	this->UpdateCpuTime();
}

void CCpuUsage::ProcessHandleChange(HANDLE hProcess)
{
	this->_hProcess = hProcess;
	this->UpdateCpuTime();
	return;
}

void CCpuUsage::UpdateCpuTime(void)
{
	ULARGE_INTEGER Idle;
	ULARGE_INTEGER Kernel;
	ULARGE_INTEGER User;

	if (false == GetSystemTimes((PFILETIME)&Idle, (PFILETIME)&Kernel, (PFILETIME)&User))
		return;

	// Kernel은 idle을 포함하고 있다.
	unsigned __int64 idleDiff	= Idle.QuadPart - _ftProcessorLastIdle.QuadPart;
	unsigned __int64 kernelDiff = Kernel.QuadPart - _ftProcessorLastKernel.QuadPart;
	unsigned __int64 userDiff	= User.QuadPart - _ftProcessorLastUser.QuadPart;

	unsigned __int64 Total = kernelDiff + userDiff;
	unsigned __int64 timeDiff;

	if (0 == Total)
	{
		_dProcessorTotal = 0.0f;
		_dProcessorKernel = 0.0f;
		_dProcessorUser = 0.0f;
	}
	else
	{
		_dProcessorTotal = (double)(Total - idleDiff) / Total * 100.0f;
		_dProcessorKernel = (double)(kernelDiff - idleDiff) / Total * 100.0f;
		_dProcessorUser = (double)userDiff / Total * 100.0f; 
	}

	_ftProcessorLastIdle = Idle;
	_ftProcessorLastKernel = Kernel;
	_ftProcessorLastUser = User;

	FILETIME None;
	ULARGE_INTEGER NowTime;

	GetSystemTimeAsFileTime((LPFILETIME)&NowTime);
	GetProcessTimes(this->_hProcess, &None, &None, (LPFILETIME)&Kernel, (LPFILETIME)&User);

	timeDiff = NowTime.QuadPart - _ftProcessLastTime.QuadPart;
	kernelDiff = Kernel.QuadPart - _ftProcessLastKernel.QuadPart;
	userDiff = User.QuadPart - _ftProcessLastUser.QuadPart;

	Total = kernelDiff + userDiff;

	_dProcessTotal = Total / (double)_iNumberOfProcessors / (double)timeDiff * 100.0f;
	_dProcessKernel = kernelDiff / (double)_iNumberOfProcessors / (double)timeDiff * 100.0f;
	_dProcessUser = userDiff / (double)_iNumberOfProcessors / (double)timeDiff * 100.0f;

	_ftProcessLastTime = NowTime;
	_ftProcessLastKernel = Kernel;
	_ftProcessLastUser = User;

	return;
}


double CCpuUsage::ProcessorTotal(void)	{ return this->_dProcessorTotal; }
double CCpuUsage::ProcessorKernel(void) { return this->_dProcessorKernel; }
double CCpuUsage::ProcessorUser(void)	{ return this->_dProcessorUser; }

double CCpuUsage::ProcessTotal(void)	{ return this->_dProcessTotal; }
double CCpuUsage::ProcessKernel(void)	{ return this->_dProcessKernel; }
double CCpuUsage::ProcessUser(void)		{ return this->_dProcessUser; }
