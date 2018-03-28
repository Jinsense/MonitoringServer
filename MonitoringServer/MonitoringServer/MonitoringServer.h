#ifndef _SERVER_SERVER_MONITORINGSERVER_H_
#define _SERVER_SERVER_MONITORINGSERVER_H_

#include <list>
#include <time.h>

#include "Config.h"
#include "MonitorProtocol.h"
#include "LanServer.h"
#include "NetServer.h"
#include "DB_Connector.h"

typedef struct st_ClientInfo
{
	unsigned __int64 iClientID;
	char			IP[20];
	unsigned short	Port;
}INFO;

extern CConfig Config;
class CLanServer;

class CMonitoringServer : public CNetServer
{
	CMonitoringServer();
	~CMonitoringServer();

protected:
	virtual void		OnClientJoin(st_SessionInfo Info);
	virtual void		OnClientLeave(unsigned __int64 iClientID);
	virtual void		OnConnectionRequest(WCHAR * pClientIP, int iPort);
	virtual void		OnError(int iErrorCode, WCHAR *pError) = 0;
	virtual bool		OnRecv(unsigned __int64 iClientID, CPacket *pPacket);

public:
	static unsigned __stdcall	UpdateThread(void *pParam)
	{
		CMonitoringServer *pUpdateThread = (CMonitoringServer*)pParam;
		if (NULL == pUpdateThread)
		{
			wprintf(L"[MonitoringServer :: UpdateThread] Init Error\n");
			return false;
		}
		pUpdateThread->UpdateThread_Update();
		return true;
	}
	void		UpdateThread_Update();
	bool		MakePacket(BYTE Type, CPacket *pPacket);

public:
	CLanServer			*_pLanServer;
	CDBConnector		_MonitorDB;
private:
	SRWLOCK				_DB_srwlock;
	std::list<INFO*>	_ClientList;
	SRWLOCK				_ClientList_srwlock;

	CSystemLog			*_pLog;

	struct tm			*_pTime;
	time_t				_Timer;
	SRWLOCK				_Timer_srwlock;
};

#endif