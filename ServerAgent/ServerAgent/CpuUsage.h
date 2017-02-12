#pragma once

class CCpuUsage
{
public:
	CCpuUsage(HANDLE hProcess = INVALID_HANDLE_VALUE);

	void UpdateCpuTime(void);

	void ProcessHandleChange(HANDLE hProcess);

	double	ProcessorTotal(void);
	double	ProcessorKernel(void);
	double	ProcessorUser(void);

	double	ProcessTotal(void);
	double	ProcessKernel(void);
	double	ProcessUser(void);

private:
	HANDLE	_hProcess;
	int		_iNumberOfProcessors;

	////////////////////////////////////////////////////////
	// About Processor
	double	_dProcessorTotal;
	double	_dProcessorKernel;
	double	_dProcessorUser;

	ULARGE_INTEGER	_ftProcessorLastIdle;
	ULARGE_INTEGER	_ftProcessorLastKernel;
	ULARGE_INTEGER	_ftProcessorLastUser;

	////////////////////////////////////////////////////////
	// About Process 
	double	_dProcessTotal;
	double	_dProcessKernel;
	double	_dProcessUser;

	ULARGE_INTEGER	_ftProcessLastTime;
	ULARGE_INTEGER	_ftProcessLastKernel;
	ULARGE_INTEGER	_ftProcessLastUser;
};