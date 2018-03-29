#include <windows.h>

#include "Config.h"

CConfig::CConfig()
{
	ZeroMemory(&BIND_IP, sizeof(BIND_IP));
	BIND_IP_SIZE = eNUM_BUF;
	BIND_PORT = NULL;

	ZeroMemory(&LAN_BIND_IP, sizeof(LAN_BIND_IP));
	LAN_BIND_IP_SIZE = eNUM_BUF;
	LAN_BIND_PORT = NULL;

	WORKER_THREAD = NULL;

	CLIENT_MAX = NULL;
	PACKET_CODE = NULL;
	PACKET_KEY1 = NULL;
	PACKET_KEY2 = NULL;
	LOG_LEVEL = NULL;
	ZeroMemory(&SESSIONKEY, sizeof(SESSIONKEY));
	SESSIONKEY_SIZE = eNUM_BUF;

	ZeroMemory(&LOGDB_IP, sizeof(LOGDB_IP));
	LOGDB_IP_SIZE = eNUM_BUF;
	LOGDB_PORT = NULL;
	ZeroMemory(&LOGDB_USER, sizeof(LOGDB_USER));
	LOGDB_USER_SIZE = eNUM_BUF;
	ZeroMemory(&LOGDB_PASSWORD, sizeof(LOGDB_PASSWORD));
	LOGDB_PASSWORD_SIZE = eNUM_BUF;
	ZeroMemory(&LOGDB_DBNAME, sizeof(LOGDB_DBNAME));
	LOGDB_DBNAME_SIZE = eNUM_BUF;

	ZeroMemory(&IP, sizeof(IP));
}

CConfig::~CConfig()
{

}

bool CConfig::Set()
{
	bool res = true;
	res = _Parse.LoadFile(L"MonitoringServer_Config.ini");
	if (false == res)
		return false;
	res = _Parse.ProvideArea("NETWORK");
	if (false == res)
		return false;
	res = _Parse.GetValue("BIND_IP", &IP[0], &BIND_IP_SIZE);
	if (false == res)
		return false;
	_Parse.UTF8toUTF16(IP, BIND_IP, sizeof(BIND_IP));
	res = _Parse.GetValue("BIND_PORT", &BIND_PORT);
	if (false == res)
		return false;
	_Parse.GetValue("LAN_BIND_IP", &IP[0], &LAN_BIND_IP_SIZE);
	_Parse.UTF8toUTF16(IP, LAN_BIND_IP, sizeof(LAN_BIND_IP));
	_Parse.GetValue("LAN_BIND_PORT", &LAN_BIND_PORT);
	res = _Parse.GetValue("WORKER_THREAD", &WORKER_THREAD);
	if (false == res)
		return false;

	_Parse.ProvideArea("SYSTEM");
	_Parse.GetValue("CLIENT_MAX", &CLIENT_MAX);
	_Parse.GetValue("PACKET_CODE", &PACKET_CODE);
	_Parse.GetValue("PACKET_KEY1", &PACKET_KEY1);
	_Parse.GetValue("PACKET_KEY2", &PACKET_KEY2);
	_Parse.GetValue("LOG_LEVEL", &LOG_LEVEL);
	_Parse.GetValue("SESSIONKEY", &SESSIONKEY[0], &SESSIONKEY_SIZE);

	_Parse.ProvideArea("DATABASE");
	_Parse.GetValue("LOGDB_IP", &LOGDB_IP[0], &LOGDB_IP_SIZE);
	//	_Parse.UTF8toUTF16(IP, ACCOUNT_IP, sizeof(ACCOUNT_IP));
	_Parse.GetValue("LOGDB_PORT", &LOGDB_PORT);
	_Parse.GetValue("LOGDB_USER", &LOGDB_USER[0], &LOGDB_USER_SIZE);
	//	_Parse.UTF8toUTF16(IP, ACCOUNT_USER, sizeof(ACCOUNT_USER));
	_Parse.GetValue("LOGDB_PASSWORD", &LOGDB_PASSWORD[0], &LOGDB_PASSWORD_SIZE);
	//	_Parse.UTF8toUTF16(IP, ACCOUNT_PASSWORD, sizeof(ACCOUNT_PASSWORD));
	_Parse.GetValue("LOGDB_DBNAME", &LOGDB_DBNAME[0], &LOGDB_DBNAME_SIZE);
	//	_Parse.UTF8toUTF16(IP, ACCOUNT_DBNAME, sizeof(ACCOUNT_DBNAME));

	return true;
}