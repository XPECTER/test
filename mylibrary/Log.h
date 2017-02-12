#pragma once
//#define MYSQL

#define ON_LOG

#ifdef ON_LOG
#define SYSLOG(Category, LogLevel, fmt, ...)				\
do{															\
	if (LOG::iLogLevel <= LogLevel)							\
	{														\
		wsprintf(g_szLogBuff, fmt, ##__VA_ARGS__);			\
		LOG::printLog(Category, LogLevel, g_szLogBuff);		\
			}												\
} while (0)										

#define LOGLEVELCHANGE()
#else
#define SYSLOG(Category, LogLevel, fmt, ...)
#endif

class LOG
{
public :
	enum define
	{
		OUT_CONSOLE = 1,
		OUT_FILE = 2,
		OUT_DATABASE = 4,

		LEVEL_DEBUG = 1,
		LEVEL_WARNING = 2,
		LEVEL_ERROR = 3
	};

public :
	LOG();
	~LOG();

	static void logSet(int mode, int logLevel);
	static void LogLevelChange(void);
	bool connectDatabase(void);
	static void printLog(WCHAR *category, int logLevel, WCHAR *fmt, ...);
	
public:
	static bool bConsole;
	static bool bFile;
	static bool bDatabase;

	static int iLogLevel;

private :
	static volatile __int64 logNo;
	static SRWLOCK _srwLock;
	
	// mysql
#ifdef MYSQL
private:
	MYSQL *connector;
	MYSQL conn;
#endif
};

extern LOG SysLog;
extern wchar_t g_szLogBuff[1024];