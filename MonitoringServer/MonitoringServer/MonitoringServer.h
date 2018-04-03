#ifndef _SERVER_SERVER_MONITORINGSERVER_H_
#define _SERVER_SERVER_MONITORINGSERVER_H_

#include <list>
#include <time.h>

#include "Config.h"
#include "MonitorProtocol.h"
#include "LanServer.h"
#include "NetServer.h"
#include "DB_Connector.h"

typedef struct st_MonitorInfo
{
	unsigned __int64 iClientID;
	char			IP[20];
	unsigned short	Port;
}MONITORINGINFO;

extern CConfig Config;
class CLanServer;

class CMonitoringServer : public CNetServer
{
public:
	CMonitoringServer();
	~CMonitoringServer();

	bool				MakePacket(BYTE DataType, CPacket *pPacket);
protected:
	virtual void		OnClientJoin(st_SessionInfo Info);
	virtual void		OnClientLeave(unsigned __int64 iClientID);
	virtual void		OnConnectionRequest(WCHAR * pClientIP, int iPort);
	virtual void		OnError(int iErrorCode, WCHAR *pError);
	virtual bool		OnRecv(unsigned __int64 iClientID, CPacket *pPacket);


private:
//	static unsigned __stdcall UpdateThread(void *pParam)
	//{
	//	CMonitoringServer *pUpdateThread = (CMonitoringServer*)pParam;
	//	if (NULL == pUpdateThread)
	//	{
	//		wprintf(L"[MonitoringServer :: UpdateThread] Init Error\n");
	//		return false;
	//	}
	//	pUpdateThread->UpdateThread_Update();
	//	return true;
	//}
//	void		UpdateThread_Update();

	static unsigned __stdcall DBWriteThread(void *pParam)
	{
		CMonitoringServer *pDBWriteThread = (CMonitoringServer*)pParam;
		if (NULL == pDBWriteThread)
		{
			wprintf(L"[MonitoringServer :: DBWriteThread] Init Error\n");
			return false;
		}
		pDBWriteThread->DBWriteThread_Update();
		return true;
	}
	void		DBWriteThread_Update();

public:
	CLanServer			*_pLanServer;
	std::list<MONITORINGINFO*>	_ClientList;
	SRWLOCK				_ClientList_srwlock;
	CDBConnector		_MonitorDB;
private:
	SRWLOCK				_DB_srwlock;

	HANDLE				_DBThread;
//	HANDLE				_Updatethread;
	CSystemLog			*_pLog;

	struct tm			*_pTime;
	time_t				_Timer;
	SRWLOCK				_Timer_srwlock;
};

#endif