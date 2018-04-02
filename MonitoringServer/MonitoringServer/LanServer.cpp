#include <WinSock2.h>
#include <windows.h>
#include <wchar.h>
#include <Ws2tcpip.h>
#include <process.h>
#include <time.h>
#include <iostream>

#include "LanServer.h"

using namespace std;

CLanServer::CLanServer()
{
	_pIOCompare = nullptr;
	_pSessionArray = nullptr;
	_listensock = INVALID_SOCKET;

	_bWhiteIPMode = false;
	_bShutdown = false;
	_iAllThreadCnt = 0;
	_pIndex = nullptr;
	_iClientIDCnt = 1;
	_iAcceptTPS = 0;
	_iAcceptTotal = 0;
	_iRecvPacketTPS = 0;
	_iSendPacketTPS = 0;
	_iConnectClient = 0;

	for (int i = 0; i < LAN_MONITOR_NUM; i++)
	{
		_Monitor[i].Type = i + 1;
		_Monitor[i].Value = 0;
		_Monitor[i].TimeStamp = 0;
		_Monitor[i].Min = 0xffffffff;
		_Monitor[i].Max = 0;
		_Monitor[i].Avr = 0;
		_Monitor[i].Recv = false;
	}	
}

CLanServer::~CLanServer()
{

}

bool CLanServer::Set(CMonitoringServer *pMonitoringServer)
{
	_pMonitor = pMonitoringServer;
	return true;
}

bool CLanServer::ServerStart(WCHAR *pOpenIP, int iPort, int iMaxWorkerThread,
	bool bNodelay, int iMaxSession)
{
	wprintf(L"[Server :: Server_Start]	Start\n");

	int _iRetval = 0;
	setlocale(LC_ALL, "Korean");

	CPacket::MemoryPoolInit();

	_pIOCompare = (LANCOMPARE*)_aligned_malloc(sizeof(LANCOMPARE), 16);
	_pIOCompare->iIOCount = 0;
	_pIOCompare->iReleaseFlag = false;

	_pSessionArray = new LANSESSION[iMaxSession];

	for (int i = 0; i < iMaxSession; i++)
	{
		_pSessionArray[i].lIOCount = 0;
		_pSessionArray[i].sock = INVALID_SOCKET;
		_pSessionArray[i].lSendCount = 0;
		_pSessionArray[i].iClientID = NULL;
		_pSessionArray[i].lSendFlag = true;
		_pSessionArray[i].bLoginFlag = false;
		_pSessionArray[i].bRelease = false;
	}

	_pIndex = new unsigned __int64[iMaxSession];
	for (unsigned __int64 i = 0; i < iMaxSession; i++)
	{
		_pIndex[i] = i;
		_SessionStack.Push(&_pIndex[i]);
	}

	if ((_iRetval = ServerInit()) == false)
	{
		return false;
	}

	struct sockaddr_in _server_addr;
	ZeroMemory(&_server_addr, sizeof(_server_addr));
	_server_addr.sin_family = AF_INET;
	InetPton(AF_INET, (PCWSTR)pOpenIP, &_server_addr.sin_addr);
	_server_addr.sin_port = htons(iPort);

	_iRetval = ::bind(_listensock, (sockaddr *)&_server_addr, sizeof(_server_addr));
	if (_iRetval == SOCKET_ERROR)
	{
		return false;
	}

	_iRetval = listen(_listensock, SOMAXCONN);
	if (_iRetval == SOCKET_ERROR)
	{
		return false;
	}

	_hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, &AcceptThread,
		(LPVOID)this, 0, NULL);
	_hAllthread[_iAllThreadCnt++] = _hAcceptThread;
	wprintf(L"[Server :: Server_Start]	AcceptThread Create\n");

	for (int i = 0; i < iMaxWorkerThread; i++)
	{
		_hWorkerThread[i] = (HANDLE)_beginthreadex(NULL, 0, &WorkerThread,
			(LPVOID)this, 0, NULL);
		_hAllthread[_iAllThreadCnt++] = _hWorkerThread[i];
	}
	wprintf(L"[Server :: Server_Start]	WorkerThread Create\n");
	wprintf(L"[Server :: Server_Start]	Complete\n");
	return true;
}

bool CLanServer::ServerStop()
{
	wprintf(L"[Server :: Server_Stop]	Start\n");

	_bWhiteIPMode = true;
	_aligned_free(_pIOCompare);
	delete _pSessionArray;
	delete _pIndex;

	wprintf(L"([Server :: Server_Stop]	Complete\n");
	return true;
}

void CLanServer::Disconnect(unsigned __int64 iClientID)
{
	unsigned __int64  _iIndex = iClientID >> 48;
	LANSESSION *_pSession = &_pSessionArray[_iIndex];

	InterlockedIncrement(&_pSession->lIOCount);

	if (true == _pSession->bRelease || iClientID != _pSession->iClientID)
	{
		if (0 == InterlockedDecrement(&_pSession->lIOCount))
			ClientRelease(_pSession);
		return;
	}
	shutdown(_pSession->sock, SD_BOTH);

	if (0 == InterlockedDecrement(&_pSession->lIOCount))
		ClientRelease(_pSession);

	return;
}

unsigned __int64 CLanServer::GetClientCount()
{
	return _iConnectClient;
}

bool CLanServer::SendPacket(unsigned __int64 iClientID, CPacket *pPacket)
{
	unsigned __int64 _iIndex = GET_INDEX(_iIndex, iClientID);

	if (1 == InterlockedIncrement(&_pSessionArray[_iIndex].lIOCount))
	{
		if (0 == InterlockedDecrement(&_pSessionArray[_iIndex].lIOCount))
		{
			ClientRelease(&_pSessionArray[_iIndex]);
		}
		return false;
	}

	if (true == _pSessionArray[_iIndex].bRelease)
	{
		if (0 == InterlockedDecrement(&_pSessionArray[_iIndex].lIOCount))
		{
			ClientRelease(&_pSessionArray[_iIndex]);
		}
		return false;
	}

	if (_pSessionArray[_iIndex].iClientID == iClientID)
	{
		if (_pSessionArray[_iIndex].bLoginFlag != true)
			return false;

		_iSendPacketTPS++;
		pPacket->AddRef();


		pPacket->SetHeader_CustomShort(pPacket->GetDataSize());
		_pSessionArray[_iIndex].SendQ.Enqueue(pPacket);

		if (0 == InterlockedDecrement(&_pSessionArray[_iIndex].lIOCount))
		{
			ClientRelease(&_pSessionArray[_iIndex]);
			return false;
		}
		SendPost(&_pSessionArray[_iIndex]);
		return true;
	}

	if (0 == InterlockedDecrement(&_pSessionArray[_iIndex].lIOCount))
		ClientRelease(&_pSessionArray[_iIndex]);
	return false;
}

bool CLanServer::ServerInit()
{
	WSADATA _Data;
	int _iRetval = WSAStartup(MAKEWORD(2, 2), &_Data);
	if (0 != _iRetval)
	{
		wprintf(L"[Server :: Server_Init]	WSAStartup Error\n");
		return false;
	}

	_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if (_hIOCP == NULL)
	{
		wprintf(L"[Server :: Server_Init]	IOCP Init Error\n");
		return false;
	}

	_listensock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,
		NULL, 0, WSA_FLAG_OVERLAPPED);
	if (_listensock == INVALID_SOCKET)
	{
		wprintf(L"[Server :: Server_Init]	Listen Socket Init Error\n");
		return false;
	}
	wprintf(L"[Server :: Server_Init]		Complete\n");
	return true;
}

bool CLanServer::ClientShutdown(LANSESSION *pSession)
{
	int _iRetval;
	_iRetval = shutdown(pSession->sock, SD_BOTH);
	if (false == _iRetval)
	{
		return false;
	}
	return true;
}

bool CLanServer::ClientRelease(LANSESSION *pSession)
{
	LANCOMPARE _CheckFlag;
	_CheckFlag.iIOCount = pSession->lIOCount;
	_CheckFlag.iReleaseFlag = pSession->bRelease;

	if (FALSE == InterlockedCompareExchange128((LONG64*)_pIOCompare, _CheckFlag.iIOCount,
		_CheckFlag.iReleaseFlag, (LONG64*)&_CheckFlag))
		return FALSE;

	pSession->bRelease = TRUE;
	closesocket(pSession->sock);

	while (0 < pSession->SendQ.GetUseCount())
	{
		CPacket *_pPacket;
		pSession->SendQ.Dequeue(_pPacket);
		_pPacket->Free();
	}

	while (0 < pSession->PacketQ.GetUseSize())
	{
		CPacket *_pPacket;
		pSession->PacketQ.Peek((char*)&_pPacket, sizeof(CPacket*));
		_pPacket->Free();
		pSession->PacketQ.Dequeue(sizeof(CPacket*));
	}

	pSession->bLoginFlag = false;

	unsigned __int64 iClientID = pSession->iClientID;
	unsigned __int64 iIndex = GET_INDEX(iIndex, iClientID);

	
	printf("[LanRelease] IP : %s\n", _ServerInfo[pSession->Index].IP);
	ZeroMemory(&_ServerInfo[pSession->Index].IP, sizeof(_ServerInfo[pSession->Index].IP));
	_ServerInfo[pSession->Index].Port = NULL;
	_ServerInfo[pSession->Index].Login = false;

	InterlockedDecrement(&_iConnectClient);
	PutIndex(iIndex);
	return true;
}

void CLanServer::WorkerThread_Update()
{
	DWORD _dwRetval;

	while (1)
	{
		OVERLAPPED * _pOver = NULL;
		LANSESSION * _pSession = NULL;
		DWORD _dwTrans = 0;

		_dwRetval = GetQueuedCompletionStatus(_hIOCP, &_dwTrans, (PULONG_PTR)&_pSession,
			(LPWSAOVERLAPPED*)&_pOver, INFINITE);
		if (nullptr == _pOver)
		{
			if (nullptr == _pSession && 0 == _dwTrans)
			{
				PostQueuedCompletionStatus(_hIOCP, 0, 0, 0);
			}
		}

		if (0 == _dwTrans)
		{
			shutdown(_pSession->sock, SD_BOTH);
		}
		else if (_pOver == &_pSession->RecvOver)
		{
			CompleteRecv(_pSession, _dwTrans);
		}
		else if (_pOver == &_pSession->SendOver)
		{
			CompleteSend(_pSession, _dwTrans);
		}

		if (0 >= (_dwRetval = InterlockedDecrement(&_pSession->lIOCount)))
		{
			if (0 == _dwRetval)
				ClientRelease(_pSession);
		}
	}
}

void CLanServer::AcceptThread_Update()
{
	DWORD _dwRetval = 0;
	SOCKADDR_IN _ClientAddr;

	while (1)
	{
		int addrSize = sizeof(_ClientAddr);
		SOCKET clientSock = accept(_listensock, (SOCKADDR*)&_ClientAddr, &addrSize);
		if (INVALID_SOCKET == clientSock)
			break;

		InterlockedIncrement(&_iAcceptTPS);
		InterlockedIncrement(&_iAcceptTotal);

		//		OnConnectionRequest((WCHAR*)&_ClientAddr.sin_addr, _ClientAddr.sin_port);

		unsigned __int64 * _iSessionNum = GetIndex();
		if (_iSessionNum == nullptr)
		{
			closesocket(clientSock);
			continue;
		}

		if (_pSessionArray[*_iSessionNum].bLoginFlag == TRUE)
		{
			ClientRelease(&_pSessionArray[*_iSessionNum]);
			continue;
		}

		unsigned __int64 iIndex = *_iSessionNum;

		_pSessionArray[*_iSessionNum].iClientID = _iClientIDCnt++;
		SET_INDEX(iIndex, _pSessionArray[*_iSessionNum].iClientID);
		_pSessionArray[*_iSessionNum].sock = clientSock;
		_pSessionArray[*_iSessionNum].RecvQ.Clear();
		_pSessionArray[*_iSessionNum].PacketQ.Clear();
		_pSessionArray[*_iSessionNum].lSendFlag = TRUE;
		_pSessionArray[*_iSessionNum].lSendCount = 0;
		InterlockedIncrement(&_iConnectClient);
		_pSessionArray[*_iSessionNum].bLoginFlag = TRUE;

		inet_ntop(AF_INET, &_ClientAddr.sin_addr, _pSessionArray[*_iSessionNum].IP, sizeof(_pSessionArray[*_iSessionNum].IP));
		_pSessionArray[*_iSessionNum].Port = _ClientAddr.sin_port;

		_pSessionArray[*_iSessionNum].bRelease = FALSE;

		InterlockedIncrement(&_pSessionArray[*_iSessionNum].lIOCount);
		CreateIoCompletionPort((HANDLE)clientSock, _hIOCP,
			(ULONG_PTR)&_pSessionArray[*_iSessionNum], 0);
		//		OnClientJoin(&pSessionArray[*_iSessionNum].Info);
		StartRecvPost(&_pSessionArray[*_iSessionNum]);
	}
}

LANSESSION* CLanServer::SessionAcquireLock(unsigned __int64 iClientID)
{
	unsigned __int64 _iIndex = iClientID >> 48;
	LANSESSION *_pSession = &_pSessionArray[_iIndex];
	InterlockedIncrement(&_pSession->lIOCount);

	return _pSession;
}

void CLanServer::SessionAcquireFree(LANSESSION *pSession)
{
	DWORD _dwRetval;
	if (0 >= (_dwRetval = InterlockedDecrement(&pSession->lIOCount)))
	{
		if (0 == _dwRetval)
			ClientRelease(pSession);
	}
}

void CLanServer::StartRecvPost(LANSESSION *pSession)
{
	DWORD _dwRetval = 0;
	DWORD _dwFlags = 0;
	ZeroMemory(&pSession->RecvOver, sizeof(pSession->RecvOver));

	WSABUF _Buf[2];
	DWORD _dwFreeSize = pSession->RecvQ.GetFreeSize();
	DWORD _dwNotBrokenPushSize = pSession->RecvQ.GetNotBrokenPushSize();
	if (0 == _dwFreeSize && 0 == _dwNotBrokenPushSize)
	{
		if (0 >= (_dwRetval = InterlockedDecrement(&pSession->lIOCount)))
		{
			if (0 == _dwRetval)
				ClientRelease(pSession);
		}
		else
			shutdown(pSession->sock, SD_BOTH);
		return;
	}
	int _iNumOfBuf = (_dwNotBrokenPushSize < _dwFreeSize) ? 2 : 1;

	_Buf[0].buf = pSession->RecvQ.GetWriteBufferPtr();
	_Buf[0].len = _dwNotBrokenPushSize;

	if (2 == _iNumOfBuf)
	{
		_Buf[1].buf = pSession->RecvQ.GetBufferPtr();
		_Buf[1].len = _dwFreeSize - _dwNotBrokenPushSize;
	}
	if (SOCKET_ERROR == WSARecv(pSession->sock, _Buf, _iNumOfBuf,
		NULL, &_dwFlags, &pSession->RecvOver, NULL))
	{
		int _iLastError = WSAGetLastError();
		if (ERROR_IO_PENDING != _iLastError)
		{
			if (0 >= (_dwRetval = InterlockedDecrement(&pSession->lIOCount)))
			{
				if (0 == _dwRetval)
					ClientRelease(pSession);
			}
			else
				shutdown(pSession->sock, SD_BOTH);
		}
	}
}

void CLanServer::RecvPost(LANSESSION *pSession)
{
	InterlockedIncrement(&pSession->lIOCount);

	DWORD _dwRetval = 0;
	DWORD _dwFlags = 0;
	ZeroMemory(&pSession->RecvOver, sizeof(pSession->RecvOver));

	WSABUF _Buf[2];
	DWORD _dwFreeSize = pSession->RecvQ.GetFreeSize();
	DWORD _dwNotBrokenPushSize = pSession->RecvQ.GetNotBrokenPushSize();
	if (0 == _dwFreeSize && 0 == _dwNotBrokenPushSize)
	{
		if (0 >= (_dwRetval = InterlockedDecrement(&pSession->lIOCount)))
		{
			if (0 == _dwRetval)
				ClientRelease(pSession);
		}
		else
			shutdown(pSession->sock, SD_BOTH);
		return;
	}

	int _iNumOfBuf = (_dwNotBrokenPushSize < _dwFreeSize) ? 2 : 1;

	_Buf[0].buf = pSession->RecvQ.GetWriteBufferPtr();
	_Buf[0].len = _dwNotBrokenPushSize;

	if (2 == _iNumOfBuf)
	{
		_Buf[1].buf = pSession->RecvQ.GetBufferPtr();
		_Buf[1].len = _dwFreeSize - _dwNotBrokenPushSize;
	}

	if (SOCKET_ERROR == WSARecv(pSession->sock, _Buf, _iNumOfBuf,
		NULL, &_dwFlags, &pSession->RecvOver, NULL))
	{
		int _iLastError = WSAGetLastError();
		if (ERROR_IO_PENDING != _iLastError)
		{
			if (0 >= (_dwRetval = InterlockedDecrement(&pSession->lIOCount)))
			{
				if (0 == _dwRetval)
					ClientRelease(pSession);
			}
			else
				shutdown(pSession->sock, SD_BOTH);
		}
	}
}

void CLanServer::SendPost(LANSESSION *pSession)
{
	DWORD _dwRetval;
	do
	{
		if (false == InterlockedCompareExchange(&pSession->lSendFlag, false, true))
			return;

		if (0 == pSession->SendQ.GetUseCount())
		{
			InterlockedExchange(&pSession->lSendFlag, true);
			continue;
		}
		ZeroMemory(&pSession->SendOver, sizeof(pSession->SendOver));

		WSABUF _Buf[LAN_WSABUF_NUMBER];
		CPacket *_pPacket;
		long _lBufNum = 0;
		int _iUseSize = (pSession->SendQ.GetUseCount());
		if (_iUseSize > LAN_WSABUF_NUMBER)
		{
			_lBufNum = LAN_WSABUF_NUMBER;
			pSession->lSendCount = LAN_WSABUF_NUMBER;
			for (int i = 0; i < LAN_WSABUF_NUMBER; i++)
			{
				pSession->SendQ.Dequeue(_pPacket);
				pSession->PacketQ.Enqueue((char*)&_pPacket, sizeof(CPacket*));
				_Buf[i].buf = _pPacket->GetReadPtr();
				_Buf[i].len = _pPacket->GetPacketSize_CustomHeader(static_cast<int>(CPacket::en_PACKETDEFINE::SHORT_HEADER_SIZE));
			}
		}
		else
		{
			_lBufNum = _iUseSize;
			pSession->lSendCount = _iUseSize;
			for (int i = 0; i < _iUseSize; i++)
			{
				pSession->SendQ.Dequeue(_pPacket);
				pSession->PacketQ.Enqueue((char*)&_pPacket, sizeof(CPacket*));
				_Buf[i].buf = _pPacket->GetReadPtr();
				_Buf[i].len = _pPacket->GetPacketSize_CustomHeader(static_cast<int>(CPacket::en_PACKETDEFINE::SHORT_HEADER_SIZE));
			}
		}
		InterlockedIncrement(&pSession->lIOCount);
		ZeroMemory(&pSession->SendOver, sizeof(pSession->SendOver));
		if (SOCKET_ERROR == WSASend(pSession->sock, _Buf, _lBufNum,
			NULL, 0, &pSession->SendOver, NULL))
		{
			int _LastError = WSAGetLastError();
			if (ERROR_IO_PENDING != _LastError)
			{
				if (0 >= (_dwRetval = InterlockedDecrement(&pSession->lIOCount)))
				{
					if (0 == _dwRetval)
						ClientRelease(pSession);
				}
				else
					shutdown(pSession->sock, SD_BOTH);
			}
		}
	} while (0 != pSession->SendQ.GetUseCount());
}

void CLanServer::CompleteRecv(LANSESSION *pSession, DWORD dwTransfered)
{
	pSession->RecvQ.Enqueue(dwTransfered);
	WORD _wPayloadSize = 0;

	while (LAN_HEADER_SIZE == pSession->RecvQ.Peek((char*)&_wPayloadSize, LAN_HEADER_SIZE))
	{
		CPacket *_pPacket = CPacket::Alloc();
		if (pSession->RecvQ.GetUseSize() < LAN_HEADER_SIZE + _wPayloadSize)
			break;

		pSession->RecvQ.Dequeue(LAN_HEADER_SIZE);

		if (_pPacket->GetFreeSize() < _wPayloadSize)
		{
			shutdown(pSession->sock, SD_BOTH);
			return;
		}
		pSession->RecvQ.Dequeue(_pPacket->GetWritePtr(), _wPayloadSize);
		_pPacket->PushData(_wPayloadSize + sizeof(CPacket::st_PACKET_HEADER));
		_pPacket->PopData(sizeof(CPacket::st_PACKET_HEADER));

		if (false == OnRecv(pSession, _pPacket))
			return;

		_pPacket->Free();
	}
	RecvPost(pSession);
}

void CLanServer::CompleteSend(LANSESSION *pSession, DWORD dwTransfered)
{
	CPacket *_pPacket[LAN_WSABUF_NUMBER];
	pSession->PacketQ.Peek((char*)&_pPacket, sizeof(CPacket*) *pSession->lSendCount);
	for (int i = 0; i < pSession->lSendCount; i++)
	{
		_pPacket[i]->Free();
		pSession->PacketQ.Dequeue(sizeof(CPacket*));
	}

	InterlockedExchange(&pSession->lSendFlag, true);

	SendPost(pSession);
}

bool CLanServer::OnRecv(LANSESSION *pSession, CPacket *pPacket)
{
	_iRecvPacketTPS++;

	WORD Type;
	*pPacket >> Type;

	if (en_PACKET_SS_MONITOR_LOGIN == Type)
	{
		int ServerNo;
		*pPacket >> ServerNo;

		pSession->Index = ServerNo;
		strcpy_s(_ServerInfo[ServerNo].IP, sizeof(_ServerInfo[ServerNo].IP), pSession->IP);
		_ServerInfo[ServerNo].Port = pSession->Port;
		_ServerInfo[ServerNo].Login = true;

		printf("[LanConnect] ServerNo : %d, IP : %s\n", ServerNo, _ServerInfo[ServerNo].IP);

	}
	else if (en_PACKET_SS_MONITOR_DATA_UPDATE == Type)
	{
		BYTE DataType;
		int DataValue;
		int TimeStamp;
		*pPacket >> DataType >> DataValue >> TimeStamp;

		BYTE Type = DataType - 1;
		_Monitor[Type].TimeStamp = TimeStamp;
		_Monitor[Type].Value = DataValue;
		if (DataValue < _Monitor[Type].Min)
			_Monitor[Type].Min;
		if (DataValue > _Monitor[Type].Max)
			_Monitor[Type].Max;

		_Monitor[Type].Avr = (_Monitor[Type].Avr + DataValue) / 2;
		_Monitor[Type].Recv = true;
		return true;
	}
	return true;
}

bool CLanServer::SetShutDownMode(bool bFlag)
{
	_bShutdown = bFlag;
	PostQueuedCompletionStatus(_hIOCP, 0, 0, 0);
	WaitForMultipleObjects(_iAllThreadCnt, _hAllthread, true, INFINITE);

	ServerStop();

	return _bShutdown;
}

bool CLanServer::SetWhiteIPMode(bool bFlag)
{
	_bWhiteIPMode = bFlag;
	return _bWhiteIPMode;
}

unsigned __int64* CLanServer::GetIndex()
{
	unsigned __int64 *_iIndex = nullptr;
	_SessionStack.Pop(&_iIndex);
	return _iIndex;
}

void CLanServer::PutIndex(unsigned __int64 iIndex)
{
	_SessionStack.Push(&_pIndex[iIndex]);
}