#include <conio.h>

#include "MonitoringServer.h"

CConfig Config;

int main()
{
	Config.Set();

	CMonitoringServer Server;
	if (false == Server._MonitorDB.Set(Config.LOGDB_IP, Config.LOGDB_USER, Config.LOGDB_PASSWORD, Config.LOGDB_DBNAME, Config.LOGDB_PORT))
	{
		printf("DB Connect Error\n");
		return 0;
	}
	Server._pLanServer->ServerStart(Config.LAN_BIND_IP, Config.LAN_BIND_PORT, Config.WORKER_THREAD, true, 10);
	Server.ServerStart(Config.BIND_IP, Config.BIND_PORT, Config.WORKER_THREAD, true, 100);
	Server.ThreadInit();
	int In;
	while (1)
	{
		In = _getch();
		switch (In)
		{
		case 'q': case 'Q':
		{
			return 0;
		}
		break;
		default:
			break;
		}
	}

	return 0;
}