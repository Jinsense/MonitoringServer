#ifndef _SERVER_PARSE_CONFIG_H_
#define _SERVER_PARSE_CONFIG_H_

#include "Parse.h"

class CConfig
{
	enum eNumConfig
	{
		eNUM_BUF = 20,
	};
public:
	CConfig();
	~CConfig();

	bool Set();

public:
	//	NETWORK
	WCHAR BIND_IP[20];
	int BIND_IP_SIZE;
	int BIND_PORT;

	WCHAR LAN_BIND_IP[20];
	int LAN_BIND_IP_SIZE;
	int LAN_BIND_PORT;

	int WORKER_THREAD;

	//	SYSTEM
	int CLIENT_MAX;
	int PACKET_CODE;
	int PACKET_KEY1;
	int PACKET_KEY2;
	int LOG_LEVEL;
	char SESSIONKEY[32];
	int SESSIONKEY_SIZE;

	//	DATABASE
	char ACCOUNT_IP[20];
	int ACCOUNT_IP_SIZE;
	int ACCOUNT_PORT;
	char ACCOUNT_USER[20];
	int ACCOUNT_USER_SIZE;
	char ACCOUNT_PASSWORD[20];
	int ACCOUNT_PASSWORD_SIZE;
	char ACCOUNT_DBNAME[20];
	int ACCOUNT_DBNAME_SIZE;

	CINIParse _Parse;

private:
	char IP[20];
};

#endif _SERVER_PARSE_CONFIG_H_
