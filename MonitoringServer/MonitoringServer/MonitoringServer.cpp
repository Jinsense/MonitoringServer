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
	WCHAR IP[20];
	UTF8toUTF16(Info.SessionIP, IP, sizeof(Info.SessionIP));
	AcquireSRWLockExclusive(&_Timer_srwlock);
	_Timer = time(NULL);
	localtime_s(_pTime, &_Timer);
	//	���� IP, Port, �ð� ��� �� �α׷� ����
	printf("Connect : %d/%d/%d %d:%d:%d IP : %s, Port : %d\n", _pTime->tm_year + 1900, _pTime->tm_mon + 1, _pTime->tm_mday, _pTime->tm_hour, _pTime->tm_min, _pTime->tm_sec, Info.SessionIP, Info.SessionPort);
	_pLog->Log(L"Connect", LOG_SYSTEM, L"Connect : %d/%d/%d %d:%d:%d IP : %s, Port : %d", 
		_pTime->tm_year + 1900, _pTime->tm_mon + 1, _pTime->tm_mday, _pTime->tm_hour, _pTime->tm_min, _pTime->tm_sec, IP, Info.SessionPort);
	ReleaseSRWLockExclusive(&_Timer_srwlock);
	//	Ŭ���̾�ƮINFO �����Ҵ�
	MONITORINGINFO *pInfo = new MONITORINGINFO;
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
	WCHAR IP[20];
	MONITORINGINFO *pInfo = nullptr;
	//	Ŭ���̾�ƮID�� ����Ʈ���� �˻�
	list<MONITORINGINFO*>::iterator iter;
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
	//	���� IP, Port, �ð� ��� �� �α׷� ����
	UTF8toUTF16(pInfo->IP, IP, sizeof(pInfo->IP));
	AcquireSRWLockExclusive(&_Timer_srwlock);
	_Timer = time(NULL);
	localtime_s(_pTime, &_Timer);
	printf("Release : %d/%d/%d %d:%d:%d IP : %s, Port : %d\n", _pTime->tm_year + 1900, _pTime->tm_mon + 1, _pTime->tm_mday, _pTime->tm_hour, _pTime->tm_min, _pTime->tm_sec, pInfo->IP, pInfo->Port);
	_pLog->Log(L"Connect", LOG_SYSTEM, L"Release : %d/%d/%d %d:%d:%d IP : %s, Port : %d",
		_pTime->tm_year + 1900, _pTime->tm_mon + 1, _pTime->tm_mday, _pTime->tm_hour, _pTime->tm_min, _pTime->tm_sec, IP, pInfo->Port);
	ReleaseSRWLockExclusive(&_Timer_srwlock);
	//	Ŭ���̾�ƮINFO �Ҵ�����
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
	//	�α��� ����Ű Ȯ��
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

	char SessionKey[32] = { 0, };
	pPacket->PopData(SessionKey, sizeof(SessionKey));

	if (0 != memcmp(&SessionKey, &Config.SESSIONKEY, sizeof(SessionKey)))
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
	SendPacket(iClientID, pNewPacket);
	pNewPacket->Free();
	return true;
}

void CMonitoringServer::UpdateThread_Update()
{
	while (1)
	{
		Sleep(1000);
		if (_ClientList.empty())
			continue;
	
		for (int i = 0; i < LAN_MONITOR_NUM; i++)
		{
			CPacket *pPacket = CPacket::Alloc();
			if (false == MakePacket(i + 1, pPacket))
			{
				pPacket->Free();
				continue;
			}
			pPacket->AddRef();
			list<MONITORINGINFO*>::iterator iter;
			AcquireSRWLockExclusive(&_ClientList_srwlock);
			for (iter = _ClientList.begin(); iter != _ClientList.end(); iter++)
			{
				SendPacket((*iter)->iClientID, pPacket);
			}
			ReleaseSRWLockExclusive(&_ClientList_srwlock);
			pPacket->Free();
		}
	}
	return;
}

void CMonitoringServer::DBWriteThread_Update()
{
	bool Res;
	while (1)
	{
		Sleep(60000);
		for (int i = 0; i < LAN_MONITOR_NUM; i++)
		{
			if (true == _pLanServer->_Monitor[i].Recv)
			{
				//	ServerNo�� ���� ����
				Res = _MonitorDB.Query_Save(L"INSERT INTO `logdb`.`monitorlog` (`logtime`, `serverno`, `type`, `value`, `min`, `max`, `avr`) VALUES (now(), '2', '%d', '%d', '%d', '%d', '%f'); ",
					_pLanServer->_Monitor[i].Type, _pLanServer->_Monitor[i].Value, _pLanServer->_Monitor[i].Min, _pLanServer->_Monitor[i].Max, _pLanServer->_Monitor[i].Avr);
				if (false == Res)
					_pLog->Log(L"Error", LOG_SYSTEM, L"INSERT INTO `logdb`.`monitorlog` (`logtime`, `serverno`, `type`, `value`, `min`, `max`, `avr`) VALUES (now(), '2', '%d', '%d', '%d', '%d', '%f'); ",
						_pLanServer->_Monitor[i].Type, _pLanServer->_Monitor[i].Value, _pLanServer->_Monitor[i].Min, _pLanServer->_Monitor[i].Max, _pLanServer->_Monitor[i].Avr);
			}
		}
	}
	return;
}

bool CMonitoringServer::MakePacket(BYTE DataType, CPacket *pPacket)
{
	if (false == _pLanServer->_Monitor[DataType - 1].Recv)
		return false;
	WORD Type = en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE;
	BYTE ServerNo;
	if (1 <= DataType && 5 >= DataType)
		ServerNo = dfMONITOR_SERVER_TYPE_GAME;		//	�ϵ���� ���� ���� ��ȣ ����
	else if (6 <= DataType && 12 >= DataType)
		ServerNo = dfMONITOR_SERVER_TYPE_GAME;		//	��ġ����ŷ ���� ��ȣ ����
	else if (13 <= DataType && 23 >= DataType)
		ServerNo = dfMONITOR_SERVER_TYPE_GAME;		//	������ ���� ��ȣ ����
	else if (24 <= DataType && 35 >= DataType)
		ServerNo = dfMONITOR_SERVER_TYPE_GAME;		//	��Ʋ ���� ��ȣ ����
	else if (36 <= DataType && 42 >= DataType)
		ServerNo = dfMONITOR_SERVER_TYPE_CHAT;		//	ä�� ���� ��ȣ ����

	BYTE _DataType = DataType;
	int DataValue = _pLanServer->_Monitor[DataType - 1].Value;
	int TimeStamp = _pLanServer->_Monitor[DataType - 1].TimeStamp;
	_pLanServer->_Monitor[DataType - 1].Recv = false;

	*pPacket << Type << ServerNo << _DataType << DataValue << TimeStamp;
	return true;
}