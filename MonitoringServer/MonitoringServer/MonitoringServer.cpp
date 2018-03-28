#include <iostream>

#include "MonitoringServer.h"

using namespace std;

CMonitoringServer::CMonitoringServer()
{
	_pTime = new struct tm;
	_Timer = time(NULL);
	_pLog = _pLog->GetInstance();
	InitializeSRWLock(&_DB_srwlock);
	InitializeSRWLock(&_ClientList_srwlock);
	InitializeSRWLock(&_Timer_srwlock);
	_pLanServer = new CLanServer;
	_pLanServer->Set(this);
}

CMonitoringServer::~CMonitoringServer()
{
	delete _pLanServer;
	delete _pTime;
}

void CMonitoringServer::OnClientJoin(st_SessionInfo Info)
{
	AcquireSRWLockExclusive(&_Timer_srwlock);
	localtime_s(_pTime, &_Timer);
	//	접속 IP, Port, 시간 출력 및 로그로 남김
	wprintf(L"[Time : %d/%d/%d %d:%d:%d] IP : %s, Port : %d", _pTime->tm_year + 1900, _pTime->tm_mon + 1, _pTime->tm_mday, _pTime->tm_hour, _pTime->tm_min, _pTime->tm_sec, Info.SessionIP, Info.SessionPort);
	_pLog->Log(L"Connect", LOG_SYSTEM, L"[Time : %d/%d/%d %d:%d:%d] IP : %s, Port : %d", 
		_pTime->tm_year + 1900, _pTime->tm_mon + 1, _pTime->tm_mday, _pTime->tm_hour, _pTime->tm_min, _pTime->tm_sec, Info.SessionIP, Info.SessionPort);
	ReleaseSRWLockExclusive(&_Timer_srwlock);
	//	클라이언트INFO 동적할당
	INFO *pInfo = new INFO;
	pInfo->iClientID = Info.iClientID;
	strcpy_s(pInfo->IP, sizeof(pInfo->IP), Info.SessionIP);
	pInfo->Port = Info.SessionPort;

	AcquireSRWLockExclusive(&_ClientList_srwlock);
	_ClientList.push_front(pInfo);
	ReleaseSRWLockExclusive(&_ClientList_srwlock);
	return;
}

void CMonitoringServer::OnClientLeave(unsigned __int64 iClientID)
{
	INFO *pInfo;
	//	클라이언트ID로 리스트에서 검색
	list<INFO*>::iterator iter;
	AcquireSRWLockExclusive(&_ClientList_srwlock);
	for (iter = _ClientList.begin(); iter != _ClientList.end(); iter++)
	{
		if (iClientID == (*iter)->iClientID)
		{
			pInfo = (*iter);
			_ClientList.erase(iter);
			break;
		}
	}
	ReleaseSRWLockExclusive(&_ClientList_srwlock);
	//	해제 IP, Port, 시간 출력 및 로그로 남김
	AcquireSRWLockExclusive(&_Timer_srwlock);
	localtime_s(_pTime, &_Timer);
	wprintf(L"[Time : %d/%d/%d %d:%d:%d] IP : %s, Port : %d", _pTime->tm_year + 1900, _pTime->tm_mon + 1, _pTime->tm_mday, _pTime->tm_hour, _pTime->tm_min, _pTime->tm_sec, pInfo->IP, pInfo->Port);
	_pLog->Log(L"Connect", LOG_SYSTEM, L"[Time : %d/%d/%d %d:%d:%d] IP : %s, Port : %d",
		_pTime->tm_year + 1900, _pTime->tm_mon + 1, _pTime->tm_mday, _pTime->tm_hour, _pTime->tm_min, _pTime->tm_sec, pInfo->IP, pInfo->Port);
	ReleaseSRWLockExclusive(&_Timer_srwlock);
	//	클라이언트INFO 할당해제
	delete pInfo;
	return;
}

void CMonitoringServer::OnConnectionRequest(WCHAR *pClientIP, int iPort)
{

	return;
}

void CMonitoringServer::OnError(int iErrorCode, WCHAR *pError)
{

	return;
}

bool CMonitoringServer::OnRecv(unsigned __int64 iClientID, CPacket *pPacket)
{
	//	로그인 인증키 확인
	WORD Type;
	*pPacket >> Type;
	if (en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN != Type)
	{
		Type = en_PACKET_CS_MONITOR_TOOL_RES_LOGIN;
		BYTE Result = 0;
		CPacket *pNewPacket = CPacket::Alloc();
		*pNewPacket << Type << Result;
		SendPacketAndDisConnect(iClientID, pNewPacket);
		pNewPacket->Free();
		return false;
	}
	char SessionKey[32];
	pPacket->PopData(SessionKey, sizeof(SessionKey));
	if (0 != strcmp(SessionKey, Config.SESSIONKEY))
	{
		Type = en_PACKET_CS_MONITOR_TOOL_RES_LOGIN;
		BYTE Result = 0;
		CPacket *pNewPacket = CPacket::Alloc();
		*pNewPacket << Type << Result;
		SendPacketAndDisConnect(iClientID, pNewPacket);
		pNewPacket->Free();
		return false;
	}
	Type = en_PACKET_CS_MONITOR_TOOL_RES_LOGIN;
	BYTE Result = 1;
	CPacket *pNewPacket = CPacket::Alloc();
	*pNewPacket << Type << Result;
	SendPacketAndDisConnect(iClientID, pNewPacket);
	pNewPacket->Free();
	return true;
}

void CMonitoringServer::UpdateThread_Update()
{
	while (1)
	{

	}
	return;
}

bool CMonitoringServer::MakePacket(BYTE Type, CPacket *pPacket)
{

	return true;
}