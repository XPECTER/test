// stdafx.h : ���� ��������� ���� ��������� �ʴ�
// ǥ�� �ý��� ���� ���� �� ������Ʈ ���� ���� ������
// ��� �ִ� ���� �����Դϴ�.
//

#pragma once
#pragma comment(lib, "Ws2_32.lib")

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <WinSock2.h>
#include <map>
#include <list>

// TODO: ���α׷��� �ʿ��� �߰� ����� ���⿡�� �����մϴ�.
#include "../../mylibrary/APIHook.h"
#include "../../mylibrary/CrashDump.h"

#include "../../mylibrary/MemoryPool.h"
#include "../../mylibrary/MemoryPoolTLS.h"

#include "../../mylibrary/LockFreeStack.h"
#include "../../mylibrary/LockFreeQueue.h"

#include "../../mylibrary/Packet.h"
#include "../../mylibrary/StreamQueue.h"

#include "../../mylibrary/Log.h"
#include "../../mylibrary/ConfigParser.h"

#include "../../mylibrary/NetServer.h"
#include "../../mylibrary/LanServer.h"
#include "../../mylibrary/LanClient.h"