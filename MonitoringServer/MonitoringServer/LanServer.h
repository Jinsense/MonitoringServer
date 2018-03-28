#ifndef _SERVER_SERVER_LANSERVER_H_
#define _SERVER_SERVER_LANSERVER_H_

#include <map>

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "winmm.lib")

#include "Packet.h"
#include "RingBuffer.h"
#include "MemoryPool.h"
#include "LockFreeStack.h"
#include "LockFreeQueue.h"
#include "MonitoringServer.h"


#define		LAN_WSABUF_NUMBER		100
#define		LAN_QUEUE_SIZE			10000
#define		LAN_HEADER_SIZE			2
#define		LAN_MONITOR_NUM			42

#define		SET_INDEX(Index, SessionKey)		Index = Index << 48; SessionKey = Index | SessionKey;
#define		GET_INDEX(Index, SessionKey)		Index = SessionKey >> 48;

typedef struct st_MONITOR
{
	BYTE Type;
	int Value;
	int TimeStamp;
	int Min;
	int Max;
	float Avr;
	bool Recv;
}MONITOR;

typedef struct st_ServerInfo
{
	int Index;
	WCHAR IP[16];
	USHORT Port;
	WCHAR ServerName[32];
}SERVERINFO;

typedef struct st_ClientInfo
{
	unsigned __int64 iClientID;
	SOCKADDR_IN Addr;
	BYTE byServerType;

	st_ClientInfo() :
		iClientID(NULL) {}
}LANINFO;

typedef struct st_RELEASE_COMPARE
{
	__int64	iIOCount;
	__int64	iReleaseFlag;

	st_RELEASE_COMPARE() :
		iIOCount(0),
		iReleaseFlag(false) {}
}LANCOMPARE;

typedef struct st_Client
{
	bool				bLoginFlag;
	bool				bRelease;
	long				lIOCount;
	long				lSendFlag;
	long				lSendCount;
	unsigned __int64	iClientID;
	SOCKET				sock;
	OVERLAPPED			SendOver;
	OVERLAPPED			RecvOver;
	CRingBuffer			RecvQ;
	CRingBuffer			PacketQ;
	CLockFreeQueue<CPacket*> SendQ;
	LANINFO				Info;

	st_Client() :
		RecvQ(LAN_QUEUE_SIZE),
		PacketQ(LAN_QUEUE_SIZE),
		lIOCount(0),
		lSendFlag(true) {}
}LANSESSION;

class CMonitoringServer;

class CLanServer
{
public:
	CLanServer();
	~CLanServer();

	void				Disconnect(unsigned __int64 iClientID);
	/*virtual void		OnClientJoin(st_SessionInfo *pInfo) = 0;
	virtual void		OnClientLeave(unsigned __int64 iClientID) = 0;
	virtual void		OnConnectionRequest(WCHAR * pClientIP, int iPort) = 0;
	virtual void		OnError(int iErrorCode, WCHAR *pError) = 0;*/
	unsigned __int64	GetClientCount();

	bool				Set(CMonitoringServer *pMonitoringServer);
	bool				ServerStart(WCHAR *pOpenIP, int iPort, int iMaxWorkerThread,
		bool bNodelay, int iMaxSession);
	bool				ServerStop();
	bool				SendPacket(unsigned __int64 iClientID, CPacket *pPacket);
	bool				ChatReqLoginSendPacket(CPacket *pPacket);
	bool				GameReqLoginSendPacket(CPacket *pPacket);
	bool				GetShutDownMode() { return _bShutdown; }
	bool				GetWhiteIPMode() { return _bWhiteIPMode; }
	bool				SetShutDownMode(bool bFlag);
	bool				SetWhiteIPMode(bool bFlag);

	LANSESSION*			SessionAcquireLock(unsigned __int64 iClientID);
	void				SessionAcquireFree(LANSESSION *pSession);

private:
	bool				ServerInit();
	bool				ClientShutdown(LANSESSION *pSession);
	bool				ClientRelease(LANSESSION *pSession);

	static unsigned int WINAPI WorkerThread(LPVOID arg)
	{
		CLanServer *_pWorkerThread = (CLanServer *)arg;
		if (_pWorkerThread == NULL)
		{
			wprintf(L"[Server :: WorkerThread]	Init Error\n");
			return false;
		}
		_pWorkerThread->WorkerThread_Update();
		return true;
	}

	static unsigned int WINAPI AcceptThread(LPVOID arg)
	{
		CLanServer *_pAcceptThread = (CLanServer*)arg;
		if (_pAcceptThread == NULL)
		{
			wprintf(L"[Server :: AcceptThread]	Init Error\n");
			return false;
		}
		_pAcceptThread->AcceptThread_Update();
		return true;
	}

	void				PutIndex(unsigned __int64 iIndex);
	void				WorkerThread_Update();
	void				AcceptThread_Update();
	void				StartRecvPost(LANSESSION *pSession);
	void				RecvPost(LANSESSION *pSession);
	void				SendPost(LANSESSION *pSession);
	void				CompleteRecv(LANSESSION *pSession, DWORD dwTransfered);
	void				CompleteSend(LANSESSION *pSession, DWORD dwTransfered);
	bool				OnRecv(LANSESSION *pSession, CPacket *pPacket);
	unsigned __int64*	GetIndex();

public:
	unsigned __int64		_iAcceptTPS;
	unsigned __int64		_iAcceptTotal;
	unsigned __int64		_iRecvPacketTPS;
	unsigned __int64		_iSendPacketTPS;
	unsigned __int64		_iConnectClient;

	SERVERINFO			_ChatServerInfo;
	SERVERINFO			_GameServerInfo;

private:
	CLockFreeStack<UINT64*>	_SessionStack;
	LANCOMPARE				*_pIOCompare;
	LANSESSION				*_pSessionArray;
	SOCKET					_listensock;

	HANDLE					_hIOCP;
	HANDLE					_hWorkerThread[100];
	HANDLE					_hAcceptThread;
	HANDLE					_hMonitorThread;
	HANDLE					_hAllthread[200];

	bool					_bWhiteIPMode;
	bool					_bShutdown;

	unsigned __int64		_iAllThreadCnt;
	unsigned __int64		*_pIndex;
	unsigned __int64		_iClientIDCnt;

	CMonitoringServer			*_pMonitor;

	//	모니터링 측정 데이터 변수	//
	MONITOR					_Monitor[LAN_MONITOR_NUM];
};

#endif 