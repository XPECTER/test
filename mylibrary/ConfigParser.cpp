#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include "ConfigParser.h"

using namespace std;

CConfigParser::CConfigParser()
{
	_contents = nullptr;
	_pos = nullptr;
	_bParse = false;
	_totalFileLen = 0;
}

CConfigParser::~CConfigParser()
{

}

bool CConfigParser::OpenConfigFile(const wchar_t *configFileName)
{
	FILE *pf = nullptr;
	errno_t err;

	err = _wfopen_s(&pf, configFileName, L"r, ccs=UNICODE");
	if (0 != err)
	{
		wprintf_s(L"File was not opened. ErrorNo : %d\n", err);
		return false;
	}
	else
	{
		if (true == this->_bParse)
			free(this->_contents);
		
		this->_bParse = true;
		fseek(pf, 0, SEEK_END);
		this->_totalFileLen = ftell(pf);
		this->_contents = (wchar_t *)malloc(this->_totalFileLen);
		wmemset(this->_contents, 0, this->_totalFileLen / 2);
		fseek(pf, 2, SEEK_SET);
		fread(this->_contents, this->_totalFileLen, 1, pf);
		//wprintf_s(L"%s", this->contents);
		fclose(pf);
		return true;
	}
}

bool CConfigParser::MoveConfigBlock(const wchar_t *configBlockName)
{
	this->_pos = this->_contents;

	if (true == this->_bParse)
	{
		while (true)
		{
			this->_pos = wcsstr(this->_pos, configBlockName);
			if (nullptr == this->_pos)
				return false;
			else
			{
				if (0x3a == *(this->_pos - 1))
					return true;
				else // 아니라면 찾은 위치 + 1에서 다시 찾아봐야 함. 
					this->_pos += 1;
			}
		}
	}
	else
		return false;
}

bool CConfigParser::GetValue(const wchar_t *key, wchar_t *outParam, int len)
{
	wchar_t *endPos = wcsstr(this->_pos, L"}");
	int strLen = (int)((char *)endPos - (char *)this->_pos) + 2;

	wchar_t *copyStr = (wchar_t *)malloc(strLen);
	memcpy_s(copyStr, strLen, this->_pos, strLen);
	wchar_t *token = nullptr, *nextToken = nullptr;
	int iCloseBlockCount = 0;

	token = wcstok_s(copyStr, L" \t\r\n", &nextToken);
	while (true)
	{
		if (nullptr == token)
			break;
		else
		{
			if (0 == wcscmp(token, key))
			{
				if ('/' != *token && '/' != *(token + 1))
				{
					token = wcstok_s(NULL, L" \t", &nextToken);
					token = wcstok_s(NULL, L"\"", &nextToken);
					swprintf_s(outParam, len, L"%s", token);
					return true;
				}
			}
			
			token = wcstok_s(NULL, L" \t\r\n", &nextToken);
		}
	}

	return false;
}

bool CConfigParser::GetValue(const wchar_t *key, int *outParam)
{
	wchar_t *endPos = wcsstr(this->_pos, L"}");
	int strLen = (int)((char *)endPos - (char *)this->_pos) + 2;

	wchar_t *copyStr = (wchar_t *)malloc(strLen);
	memcpy_s(copyStr, strLen, this->_pos, strLen);
	wchar_t *token = nullptr, *nextToken = nullptr;
	int iCloseBlockCount = 0;

	token = wcstok_s(copyStr, L" \t\r\n", &nextToken);
	while (true)
	{
		if (nullptr == token)
			break;
		else
		{
			if (0 == wcscmp(token, key))
			{
				if ('/' != *token && '/' != *(token + 1))
				{
					token = wcstok_s(NULL, L" \t", &nextToken);
					token = wcstok_s(NULL, L"\"\r\n", &nextToken);
					*outParam = _wtoi(token);
					return true;
				}
			}

			token = wcstok_s(NULL, L" \t\r\n", &nextToken);
		}
	}

	return false;
}

bool CConfigParser::GetValue(const wchar_t *key, bool *outParam)
{
	wchar_t *endPos = wcsstr(this->_pos, L"}");
	int strLen = (int)((char *)endPos - (char *)this->_pos) + 2;

	wchar_t *copyStr = (wchar_t *)malloc(strLen);
	memcpy_s(copyStr, strLen, this->_pos, strLen);
	wchar_t *token = nullptr, *nextToken = nullptr;
	int iCloseBlockCount = 0;

	token = wcstok_s(copyStr, L" \t\r\n", &nextToken);
	while (true)
	{
		if (nullptr == token)
			break;
		else
		{
			if (0 == wcscmp(token, key))
			{
				if ('/' != *token && '/' != *(token + 1))
				{
					token = wcstok_s(NULL, L" \t", &nextToken);
					token = wcstok_s(NULL, L"\"\r\n", &nextToken);
					
					if (0 == wcscmp(token, L"TRUE") || 0 == wcscmp(token, L"true"))
						*outParam = true;
					else if (0 == wcscmp(token, L"FALSE") || 0 == wcscmp(token, L"false"))
						*outParam = false;
					else
						return false;

					return true;
				}
			}
			
			token = wcstok_s(NULL, L" \t\r\n", &nextToken);
		}
	}

	return false;
}