#pragma once

class CConfigParser
{
public :
	CConfigParser();
	~CConfigParser();

	bool OpenConfigFile(const wchar_t *configFileName);
	bool MoveConfigBlock(const wchar_t *configBlockName);
	
	bool GetValue(const wchar_t *key, wchar_t *outParam, int len);
	bool GetValue(const wchar_t *key, int *outParam);
	bool GetValue(const wchar_t *key, bool *outParam);

private:
	wchar_t		*_contents;
	wchar_t		*_pos;
	int			_totalFileLen;
	bool		_bParse;
};