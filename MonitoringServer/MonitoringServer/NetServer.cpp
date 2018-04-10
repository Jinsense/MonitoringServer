#include <WinSock2.h>
#include <windows.h>
#include <wchar.h>
#include <Ws2tcpip.h>
#include <process.h>
#include <time.h>
#include <iostream>

#include "NetServer.h"

using namespace std;

CNetServer::CNetServer()
{
	pSessionArray = nullptr;
	m_listensock = INVALID_SOCKET;

	m_bWhiteIPMode = false;
	m_bShutdown = false;
	m_bMonitorFlag = true;
	m_iAllThreadCnt = 0;
	pIndex = nullptr;
	m_iSessionKeyCnt = 1;
	m_iAcceptTPS = 0;
	m_iAcceptTotal = 0;
	m_iRecvPacketTPS = 0;
	m_iSendPacketTPS = 0;
	m_iConnectClient = 0;

	m_Log = m_Log->GetInstance();
	InitializeSRWLock(&m_srw);
}

CNetServer::~CNetServer()
{

}

bool CNetServer::ServerStart(WCHAR *pOpenIP, int iPort, int iMaxWorkerThread,
	bool bNodelay, int iMaxSession)
{
	wprintf(L"[Server :: Server_Start]	Start\n");

	int _iRetval = 0;
	setlocale(LC_ALL, "Korean");
	InitializeCriticalSection(&m_SessionCS);

	CPacket::MemoryPoolInit();

	pSessionArray = new st_Session[iMaxSession];

	for (int i = 0; i < iMaxSession; i++)
	{
		pSessionArray[i].Compare = (st_IO_RELEASE_COMPARE*)_aligned_malloc(sizeof(st_IO_RELEASE_COMPARE), 16);
		pSessionArray[i].Compare->iIOCount = 0;
		pSessionArray[i].Compare->iRelease = true;
		pSessionArray[i].sock = INVALID_SOCKET;
		pSessionArray[i].lSendCount = 0;
		pSessionArray[i].iClientID = NULL;
		pSessionArray[i].lSendFlag = false;
		pSessionArray[i].lDisConnect = false;
	}

	pIndex = new unsigned __int64[iMaxSession];
	for (unsigned __int64 i = 0; i < iMaxSession; i++)
	{
		pIndex[i] = i;
		SessionStack.Push(&pIndex[i]);
	}

	if (false == (_iRetval = ServerInit()))
	{
		return false;
	}

	struct sockaddr_in _server_addr;
	ZeroMemory(&_server_addr, sizeof(_server_addr));
	_server_addr.sin_family = AF_INET;
	InetPton(AF_INET, (PCWSTR)pOpenIP, &_server_addr.sin_addr);
	_server_addr.sin_port = htons(iPort);
	setsockopt(m_listensock, IPPROTO_TCP, TCP_NODELAY, (const char*)&bNodelay, sizeof(bNodelay));

	_iRetval = ::bind(m_listensock, (sockaddr *)&_server_addr, sizeof(_server_addr));
	if (_iRetval == SOCKET_ERROR)
	{
		return false;
	}

	_iRetval = listen(m_listensock, SOMAXCONN);
	if (_iRetval == SOCKET_ERROR)
	{
		return false;
	}

	m_hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, &AcceptThread,
		(LPVOID)this, 0, NULL);
	m_hAllthread[m_iAllThreadCnt++] = m_hAcceptThread;
	wprintf(L"[Server :: Server_Start]	AcceptThread Create\n");

	for (int i = 0; i < iMaxWorkerThread; i++)
	{
		m_hWorkerThread[i] = (HANDLE)_beginthreadex(NULL, 0, &WorkerThread,
			(LPVOID)this, 0, NULL);
		m_hAllthread[m_iAllThreadCnt++] = m_hWorkerThread[i];
	}
	wprintf(L"[Server :: Server_Start]	WorkerThread Create\n");

	wprintf(L"[Server :: Server_Start]	MonitorThread Create\n");
	wprintf(L"[Server :: Server_Start]	Complete\n");
	return true;
}

bool CNetServer::ServerStop()
{
	wprintf(L"[Server :: Server_Stop]	Start\n");

	m_bWhiteIPMode = true;

	delete pSessionArray;
	delete pIndex;

	wprintf(L"([Server :: Server_Stop]	Complete\n");
	return true;
}

void CNetServer::Disconnect(unsigned __int64 iClientID)
{
	st_Session *_pSession = SessionAcquireLock(iClientID);
	if (nullptr == _pSession)
	{
		return;
	}

	if (true == _pSession->Compare->iRelease || iClientID != _pSession->iClientID)
	{
		SessionAcquireFree(_pSession);
		return;
	}

	if (true != SessionAcquireFree(_pSession))
		shutdown(_pSession->sock, SD_BOTH);
	return;
}

unsigned __int64 CNetServer::GetClientCount()
{
	return m_iConnectClient;
}

bool CNetServer::SendPacket(unsigned __int64 iClientID, CPacket *pPacket)
{
	unsigned __int64 ID = iClientID;
	unsigned __int64 _iIndex = GET_INDEX(_iIndex, ID);

	st_Session *_pSession = SessionAcquireLock(iClientID);
	if (nullptr == _pSession)
	{
		return false;
	}

	if (true == InterlockedCompareExchange64(&pSessionArray[_iIndex].Compare->iRelease, true, true))
	{
		SessionAcquireFree(_pSession);
		return false;
	}

	if (pSessionArray[_iIndex].iClientID == iClientID)
	{
		m_iSendPacketTPS++;
		pPacket->AddRef();
		pPacket->EnCode();
		pSessionArray[_iIndex].SendQ.Enqueue(pPacket);

		SendPost(&pSessionArray[_iIndex]);

		if (true == SessionAcquireFree(_pSession))
			return false;

		return true;
	}

	SessionAcquireFree(_pSession);

	return false;
}

bool CNetServer::SendPacketAndDisConnect(unsigned __int64 iClientID, CPacket *pPacket)
{
	unsigned __int64 ID = iClientID;
	unsigned __int64 _iIndex = GET_INDEX(_iIndex, ID);

	st_Session *_pSession = SessionAcquireLock(iClientID);
	if (nullptr == _pSession)
	{
		return false;
	}

	if (true == InterlockedCompareExchange64(&pSessionArray[_iIndex].Compare->iRelease, true, true))
	{
		SessionAcquireFree(_pSession);
		return false;
	}

	if (pSessionArray[_iIndex].iClientID == iClientID)
	{
		m_iSendPacketTPS++;
		pPacket->AddRef();
		pPacket->EnCode();
		pSessionArray[_iIndex].SendQ.Enqueue(pPacket);
		SendPost(&pSessionArray[_iIndex]);
		InterlockedExchange(&pSessionArray[_iIndex].lDisConnect, true);

		if (true == SessionAcquireFree(_pSession))
			return false;

		return true;
	}

	SessionAcquireFree(_pSession);

	return false;
}

bool CNetServer::ServerInit()
{
	WSADATA _Data;
	int _iRetval = WSAStartup(MAKEWORD(2, 2), &_Data);
	if (0 != _iRetval)
	{
		wprintf(L"[Server :: Server_Init]	WSAStartup Error\n");
		return false;
	}

	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if (m_hIOCP == NULL)
	{
		wprintf(L"[Server :: Server_Init]	IOCP Init Error\n");
		return false;
	}

	m_listensock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,
		NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_listensock == INVALID_SOCKET)
	{
		wprintf(L"[Server :: Server_Init]	Listen Socket Init Error\n");
		return false;
	}
	wprintf(L"[Server :: Server_Init]		Complete\n");
	return true;
}

bool CNetServer::ClientShutdown(st_Session *pSession)
{
	int _iRetval;
	//	m_Log->Log(const_cast<WCHAR*>(L"Debug"), LOG_SYSTEM,
	//		const_cast<WCHAR*>(L"ClientShutdown - shutdown call"));
	_iRetval = shutdown(pSession->sock, SD_BOTH);
	if (false == _iRetval)
	{
		return false;
	}
	return true;
}

bool CNetServer::ClientRelease(st_Session *pSession)
{
	unsigned __int64 iSessionKey = pSession->iClientID;

	if (0 == iSessionKey)
	{
		return false;
	}

	st_IO_RELEASE_COMPARE Compare;
	Compare.iIOCount = 0;
	Compare.iRelease = false;

	if (false == InterlockedCompareExchange128((LONG64*)pSession->Compare, true,
		0, (LONG64*)&Compare))
	{
		return false;
	}

	if (iSessionKey != pSession->iClientID)
	{
		m_Log->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM,
			const_cast<WCHAR*>(L"ClientRelease - SessionKey Not Same [SessionKey : %d, pSession->SessionKey :%d]"), iSessionKey, pSession->iClientID);
		return false;
	}

	while (0 < pSession->PacketQ.GetUseSize())
	{
		CPacket *_pPacket;
		pSession->PacketQ.Dequeue((char*)&_pPacket, sizeof(CPacket*));
		if (nullptr != _pPacket)
			_pPacket->Free();
	}
	while (0 < pSession->SendQ.GetUseCount())
	{
		CPacket *_pPacket;
		pSession->SendQ.Dequeue(_pPacket);
		if (nullptr != _pPacket)
			_pPacket->Free();
	}

	/*if (0 == iSessionKey)
	{
	m_Log->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM,
	const_cast<WCHAR*>(L"ClientRelease - SessionKey is 0 "));
	return false;
	}*/

	OnClientLeave(iSessionKey);
	unsigned __int64 Key = iSessionKey;
	unsigned __int64 iIndex = GET_INDEX(iIndex, Key);

	InterlockedDecrement(&m_iConnectClient);
	closesocket(pSession->sock);
	pSession->iClientID = 0;
	PutIndex(iIndex);
	return true;
}

void CNetServer::WorkerThread_Update()
{
	DWORD _dwRetval;

	while (1)
	{
		OVERLAPPED * _pOver = NULL;
		st_Session * _pSession = NULL;
		DWORD _dwTrans = 0;

		_dwRetval = GetQueuedCompletionStatus(m_hIOCP, &_dwTrans, (PULONG_PTR)&_pSession,
			(LPWSAOVERLAPPED*)&_pOver, INFINITE);
		if (nullptr == _pOver)
		{
			if (nullptr == _pSession && 0 == _dwTrans)
			{
				PostQueuedCompletionStatus(m_hIOCP, 0, 0, 0);
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

		SessionAcquireFree(_pSession);
	}
}

void CNetServer::AcceptThread_Update()
{
	DWORD _dwRetval = 0;
	SOCKADDR_IN _ClientAddr;

	while (1)
	{
		int addrSize = sizeof(_ClientAddr);
		SOCKET clientSock = accept(m_listensock, (SOCKADDR*)&_ClientAddr, &addrSize);
		if (INVALID_SOCKET == clientSock)
			break;

		InterlockedIncrement(&m_iAcceptTPS);
		InterlockedIncrement(&m_iAcceptTotal);

		OnConnectionRequest((WCHAR*)&_ClientAddr.sin_addr, _ClientAddr.sin_port);

		unsigned __int64 * _iSessionNum = GetIndex();
		if (_iSessionNum == nullptr)
		{
			m_Log->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM,
				const_cast<WCHAR*>(L"AcceptThread - Stack Not Index"));
			closesocket(clientSock);
			continue;
		}
		if (false == InterlockedCompareExchange64(&pSessionArray[*_iSessionNum].Compare->iRelease, false, true))
		{
			m_Log->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM,
				const_cast<WCHAR*>(L"AcceptThread - ReleaseFlag is FALSE"));
			closesocket(clientSock);
			continue;
		}
		if (0 != pSessionArray[*_iSessionNum].iClientID)
		{
			m_Log->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM,
				const_cast<WCHAR*>(L"AcceptThread - SessionKey is Not 0"));
			closesocket(clientSock);
			continue;
		}

		InterlockedIncrement(&m_iConnectClient);

		InterlockedIncrement64(&pSessionArray[*_iSessionNum].Compare->iIOCount);

		unsigned __int64 iIndex = *_iSessionNum;

		pSessionArray[*_iSessionNum].iClientID = m_iSessionKeyCnt++;
		SET_INDEX(iIndex, pSessionArray[*_iSessionNum].iClientID);

		pSessionArray[*_iSessionNum].sock = clientSock;
		pSessionArray[*_iSessionNum].RecvQ.Clear();
		if (0 != pSessionArray[*_iSessionNum].PacketQ.GetUseSize())
		{
			while (0 < pSessionArray[*_iSessionNum].PacketQ.GetUseSize())
			{
				CPacket *_pPacket;
				pSessionArray[*_iSessionNum].PacketQ.Dequeue((char*)&_pPacket, sizeof(CPacket*));
				if (nullptr != _pPacket)
					_pPacket->Free();
			}
		}

		if (0 != pSessionArray[*_iSessionNum].SendQ.GetUseCount())
		{
			while (0 < pSessionArray[*_iSessionNum].SendQ.GetUseCount())
			{
				CPacket *_pPacket;
				pSessionArray[*_iSessionNum].SendQ.Dequeue(_pPacket);
				if (nullptr != _pPacket)
					_pPacket->Free();
			}
		}

		InterlockedExchange(&pSessionArray[*_iSessionNum].lDisConnect, false);

		if (false != pSessionArray[*_iSessionNum].lSendFlag)
			InterlockedExchange(&pSessionArray[*_iSessionNum].lSendFlag, false);

		if (0 != pSessionArray[*_iSessionNum].lSendCount)
			InterlockedExchange(&pSessionArray[*_iSessionNum].lSendCount, 0);
		pSessionArray[*_iSessionNum].Info.iClientID =
			pSessionArray[*_iSessionNum].iClientID;
		inet_ntop(AF_INET, &_ClientAddr.sin_addr, pSessionArray[*_iSessionNum].Info.SessionIP, sizeof(pSessionArray[*_iSessionNum].Info.SessionIP));

		//	�ӽ� ���� ���� IP�� UTF8�� UTF16���� ��ȯ���Ѽ� ����
		pSessionArray[*_iSessionNum].Info.SessionPort = _ClientAddr.sin_port;

		CreateIoCompletionPort((HANDLE)clientSock, m_hIOCP,
			(ULONG_PTR)&pSessionArray[*_iSessionNum], 0);
		OnClientJoin(pSessionArray[*_iSessionNum].Info);
		StartRecvPost(&pSessionArray[*_iSessionNum]);
	}
}

st_Session* CNetServer::SessionAcquireLock(unsigned __int64 iClientID)
{
	long _lCount = 0;
	unsigned __int64 _iIndex = iClientID >> 48;
	st_Session *_pSession = &pSessionArray[_iIndex];

	_lCount = InterlockedIncrement64(&_pSession->Compare->iIOCount);
	if (1 == _lCount)
	{
		SessionAcquireFree(_pSession);
		return nullptr;
	}

	if (true == InterlockedCompareExchange64(&_pSession->Compare->iRelease, true, true))
	{
		SessionAcquireFree(_pSession);
		return nullptr;
	}

	if (iClientID != _pSession->iClientID)
	{
		SessionAcquireFree(_pSession);
		return nullptr;
	}

	return _pSession;
}

bool CNetServer::SessionAcquireFree(st_Session *pSession)
{
	if (0 == InterlockedDecrement64(&pSession->Compare->iIOCount))
	{
		ClientRelease(pSession);
		//		shutdown(pSession->sock, SD_BOTH);
		return true;
	}
	return false;
}

void CNetServer::StartRecvPost(st_Session *pSession)
{
	DWORD _dwRetval = 0;
	DWORD _dwFlags = 0;
	ZeroMemory(&pSession->RecvOver, sizeof(pSession->RecvOver));

	WSABUF _Buf[2];
	DWORD _dwFreeSize = pSession->RecvQ.GetFreeSize();
	DWORD _dwNotBrokenPushSize = pSession->RecvQ.GetNotBrokenPushSize();
	if (0 == _dwFreeSize && 0 == _dwNotBrokenPushSize)
	{
		if (true != SessionAcquireFree(pSession))
		{
			shutdown(pSession->sock, SD_BOTH);
		}
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
			if (true != SessionAcquireFree(pSession))
			{
				shutdown(pSession->sock, SD_BOTH);
			}
		}
	}
	return;
}

void CNetServer::RecvPost(st_Session *pSession)
{
	st_Session *_pSession = SessionAcquireLock(pSession->iClientID);
	if (nullptr == _pSession)
	{
		return;
	}

	DWORD _dwRetval = 0;
	DWORD _dwFlags = 0;
	ZeroMemory(&pSession->RecvOver, sizeof(pSession->RecvOver));

	WSABUF _Buf[2];
	DWORD _dwFreeSize = pSession->RecvQ.GetFreeSize();
	DWORD _dwNotBrokenPushSize = pSession->RecvQ.GetNotBrokenPushSize();
	if (0 == _dwFreeSize && 0 == _dwNotBrokenPushSize)
	{
		if (true != SessionAcquireFree(pSession))
		{
			shutdown(pSession->sock, SD_BOTH);
		}
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
			if (true != SessionAcquireFree(pSession))
			{
				shutdown(pSession->sock, SD_BOTH);
			}
		}
	}
	return;
}

void CNetServer::SendPost(st_Session *pSession)
{
	//if (true == InterlockedCompareExchange(&pSession->lSendFlag, true, false))
	//	return;
	do
	{
		if (true == InterlockedCompareExchange(&pSession->lSendFlag, true, false))
			return;

		if (0 == pSession->SendQ.GetUseCount())
		{
			InterlockedExchange(&pSession->lSendFlag, false);
			return;
		}

		WSABUF _Buf[MAX_WSABUF_NUMBER];
		CPacket *_pPacket;
		long _lBufNum = 0;
		int _iUseSize = (pSession->SendQ.GetUseCount());
		if (_iUseSize > MAX_WSABUF_NUMBER)
		{
			_lBufNum = MAX_WSABUF_NUMBER;
			pSession->lSendCount += MAX_WSABUF_NUMBER;

			for (int i = 0; i < MAX_WSABUF_NUMBER; i++)
			{
				bool _bCheck;
				_bCheck = pSession->SendQ.Dequeue(_pPacket);
				if (nullptr == _pPacket)
				{
					g_CrashDump->Crash();
					return;
				}
				pSession->PacketQ.Enqueue((char*)&_pPacket, sizeof(CPacket*));
				_Buf[i].buf = _pPacket->GetBufferPtr();
				_Buf[i].len = _pPacket->GetPacketSize();
			}
		}
		else
		{
			_lBufNum = _iUseSize;
			pSession->lSendCount += _iUseSize;
			for (int i = 0; i < _iUseSize; i++)
			{
				bool _bCheck;
				_bCheck = pSession->SendQ.Dequeue(_pPacket);
				if (nullptr == _pPacket)
				{
					g_CrashDump->Crash();
					return;
				}
				pSession->PacketQ.Enqueue((char*)&_pPacket, sizeof(CPacket*));
				_Buf[i].buf = _pPacket->GetBufferPtr();
				_Buf[i].len = _pPacket->GetPacketSize();
			}
		}
		st_Session *_pSession = SessionAcquireLock(pSession->iClientID);
		if (nullptr == _pSession)
		{
			return;
		}
		ZeroMemory(&pSession->SendOver, sizeof(pSession->SendOver));
		if (SOCKET_ERROR == WSASend(pSession->sock, _Buf, _lBufNum,
			NULL, 0, &pSession->SendOver, NULL))
		{
			int _LastError = WSAGetLastError();
			if (ERROR_IO_PENDING != _LastError)
			{
				if (true != SessionAcquireFree(pSession))
				{
					shutdown(pSession->sock, SD_BOTH);
					break;
				}
			}
		}
	} while (0 != pSession->SendQ.GetUseCount());
}

void CNetServer::CompleteRecv(st_Session *pSession, DWORD dwTransfered)
{
	pSession->RecvQ.Enqueue(dwTransfered);

	while (0 < pSession->RecvQ.GetUseSize())
	{
		CPacket::st_PACKET_HEADER _Header;

		if (sizeof(CPacket::st_PACKET_HEADER) > pSession->RecvQ.GetUseSize())
		{
			RecvPost(pSession);
			return;
		}
		pSession->RecvQ.Peek((char*)&_Header, sizeof(CPacket::st_PACKET_HEADER));

		if (MAX_QUEUE_SIZE < _Header.shLen)
		{
			shutdown(pSession->sock, SD_BOTH);
			return;
		}

		if (sizeof(CPacket::st_PACKET_HEADER) + _Header.shLen > pSession->RecvQ.GetUseSize())
		{
			RecvPost(pSession);
			return;
		}

		//		if (static_cast<int>(CPacket::en_PACKETDEFINE::PACKET_CODE) != _Header.byCode)
		if (Config.PACKET_CODE != _Header.byCode)
		{
			shutdown(pSession->sock, SD_BOTH);
			return;
		}

		if (200 < _Header.shLen + sizeof(CPacket::st_PACKET_HEADER))
		{
			shutdown(pSession->sock, SD_BOTH);
			return;
		}

		CPacket *_pPacket = CPacket::Alloc();

		if (1 != _pPacket->GetRefCount())
		{
			g_CrashDump->Crash();
		}

		pSession->RecvQ.Dequeue((char*)_pPacket->GetBufferPtr(), _Header.shLen + sizeof(CPacket::st_PACKET_HEADER));

		_pPacket->PushData(_Header.shLen + sizeof(CPacket::st_PACKET_HEADER));

		if (false == _pPacket->DeCode(&_Header))
		{
			shutdown(pSession->sock, SD_BOTH);
			_pPacket->Free();
			return;
		}

		_pPacket->m_header.byCode = _Header.byCode;
		_pPacket->m_header.shLen = _Header.shLen;
		_pPacket->m_header.RandKey = _Header.RandKey;
		_pPacket->m_header.CheckSum = _Header.CheckSum;

		_pPacket->PopData(sizeof(CPacket::st_PACKET_HEADER));

		OnRecv(pSession->iClientID, _pPacket);
		_pPacket->Free();
	}
	RecvPost(pSession);
	return;
}

void CNetServer::CompleteSend(st_Session *pSession, DWORD dwTransfered)
{
	CPacket *_pPacket[MAX_WSABUF_NUMBER];
	long _lSendCount = pSession->lSendCount;

	pSession->PacketQ.Peek((char*)&_pPacket, sizeof(CPacket*) *pSession->lSendCount);
	for (int i = 0; i < _lSendCount; i++)
	{
		_pPacket[i]->Free();
		pSession->PacketQ.Dequeue(sizeof(CPacket*));
	}
	pSession->lSendCount -= _lSendCount;

	if (true == pSession->lDisConnect && 0 == pSession->SendQ.GetUseCount())
	{
		ClientShutdown(pSession);
		return;
	}

	if (false == pSession->lDisConnect)
		InterlockedExchange(&pSession->lSendFlag, false);

	SendPost(pSession);
}

bool CNetServer::SetShutDownMode(bool bFlag)
{
	m_bShutdown = bFlag;
	PostQueuedCompletionStatus(m_hIOCP, 0, 0, 0);
	WaitForMultipleObjects(m_iAllThreadCnt, m_hAllthread, true, INFINITE);

	ServerStop();

	return m_bShutdown;
}

bool CNetServer::SetWhiteIPMode(bool bFlag)
{
	m_bWhiteIPMode = bFlag;
	return m_bWhiteIPMode;
}

bool CNetServer::SetMonitorMode(bool bFlag)
{
	m_bMonitorFlag = bFlag;
	return m_bMonitorFlag;
}

unsigned __int64* CNetServer::GetIndex()
{
	unsigned __int64 *_iIndex = nullptr;
	AcquireSRWLockExclusive(&m_srw);
	SessionStack.Pop(&_iIndex);
	ReleaseSRWLockExclusive(&m_srw);
	return _iIndex;
}

void CNetServer::PutIndex(unsigned __int64 iIndex)
{
	AcquireSRWLockExclusive(&m_srw);
	SessionStack.Push(&pIndex[iIndex]);
	ReleaseSRWLockExclusive(&m_srw);
}

void CNetServer::UTF8toUTF16(const char *szText, WCHAR *szBuf, int iBufLen)
{
	int iRe = MultiByteToWideChar(CP_UTF8, 0, szText, strlen(szText), szBuf, iBufLen);
	if (iRe < iBufLen)
		szBuf[iRe] = L'\0';
	return;
}

void CNetServer::UTF16toUTF8(WCHAR *szText, char *szBuf, int iBufLen)
{
	int iRe = WideCharToMultiByte(CP_UTF8, 0, szText, lstrlenW(szText), szBuf, iBufLen, NULL, NULL);
	return;
}