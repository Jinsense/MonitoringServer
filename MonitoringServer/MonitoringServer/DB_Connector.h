#ifndef _SERVER_DB_DBCONNECTOR_H_
#define _SERVER_DB_DBCONNECTOR_H_

#include "../mysql/include/mysql.h"
#include "../mysql/include/errmsg.h"

#include "Log.h"

#pragma comment(lib, "libmysql.lib")
#pragma comment(lib, "/vs14/mysqlclient.lib")

/*


/////////////////////////////////////////////////////////
// MySQL DB ���� Ŭ����
//
// �ܼ��ϰ� MySQL Connector �� ���� DB ���Ḹ �����Ѵ�.
//
// �����忡 �������� �����Ƿ� ���� �ؾ� ��.
// ���� �����忡�� ���ÿ� �̸� ����Ѵٸ� ������ ��.
//
/////////////////////////////////////////////////////////

namespace procademy
{

class CDBConnector
{
public:

enum en_DB_CONNECTOR
{
eQUERY_MAX_LEN	= 2048

};

CDBConnector(WCHAR *szDBIP, WCHAR *szUser, WCHAR *szPassword, WCHAR *szDBName, int iDBPort);
virtual		~CDBConnector();

//////////////////////////////////////////////////////////////////////
// MySQL DB ����
//////////////////////////////////////////////////////////////////////
bool		Connect(void);

//////////////////////////////////////////////////////////////////////
// MySQL DB ����
//////////////////////////////////////////////////////////////////////
bool		Disconnect(void);


//////////////////////////////////////////////////////////////////////
// ���� ������ ����� �ӽ� ����
//
//////////////////////////////////////////////////////////////////////
bool		Query(WCHAR *szStringFormat, ...);
bool		Query_Save(WCHAR *szStringFormat, ...);	// DBWriter �������� Save ���� ����
// ������� �������� ����.

//////////////////////////////////////////////////////////////////////
// ������ ���� �ڿ� ��� �̾ƿ���.
//
// ����� ���ٸ� NULL ����.
//////////////////////////////////////////////////////////////////////
MYSQL_ROW	FetchRow(void);

//////////////////////////////////////////////////////////////////////
// �� ������ ���� ��� ��� ��� �� ����.
//////////////////////////////////////////////////////////////////////
void		FreeResult(void);


//////////////////////////////////////////////////////////////////////
// Error ���.�� ������ ���� ��� ��� ��� �� ����.
//////////////////////////////////////////////////////////////////////
int			GetLastError(void) { return _iLastError; };
WCHAR		*GetLastErrorMsg(void) { return _szLastErrorMsg; }


private:

//////////////////////////////////////////////////////////////////////
// mysql �� LastError �� �ɹ������� �����Ѵ�.
//////////////////////////////////////////////////////////////////////
void		SaveLastError(void);

private:



//-------------------------------------------------------------
// MySQL ���ᰴü ��ü
//-------------------------------------------------------------
MYSQL		_MySQL;

//-------------------------------------------------------------
// MySQL ���ᰴü ������. �� ������ ��������.
// �� �������� null ���η� ������� Ȯ��.
//-------------------------------------------------------------
MYSQL		*_pMySQL;

//-------------------------------------------------------------
// ������ ���� �� Result �����.
//
//-------------------------------------------------------------
MYSQL_RES	*_pSqlResult;

WCHAR		_szDBIP[16];
WCHAR		_szDBUser[64];
WCHAR		_szDBPassword[64];
WCHAR		_szDBName[64];
int			_iDBPort;


WCHAR		_szQuery[eQUERY_MAX_LEN];
char		_szQueryUTF8[eQUERY_MAX_LEN];

int			_iLastError;
WCHAR		_szLastErrorMsg[128];

};
}


#endif





// Query �Լ� ������ ������� ���� üũ

#include "mysql/include/errmsg.h"

_iLastError = mysql_errno(&_MySQL);

if ( _iLastError == CR_SOCKET_CREATE_ERROR ||
_iLastError == CR_CONNECTION_ERROR ||
_iLastError == CR_CONN_HOST_ERROR ||
_iLastError == CR_SERVER_GONE_ERROR ||
_iLastError == CR_TCP_CONNECTION ||
_iLastError == CR_SERVER_HANDSHAKE_ERR ||
_iLastError == CR_SERVER_LOST ||
_iLastError == CR_INVALID_CONN_HANDLE)
{
*/

class CDBConnector
{
public:

	enum en_DB_CONNECTOR
	{
		eQUERY_MAX_LEN = 2048

	};

	CDBConnector();
	CDBConnector(char *szDBIP, char *szUser, char *szPassword, char *szDBName, int iDBPort);
	virtual		~CDBConnector();

	bool		Set(char *szDBIP, char *szUser, char *szPassword, char *szDBName, int iDBPort);

	//////////////////////////////////////////////////////////////////////
	// MySQL DB ����
	//////////////////////////////////////////////////////////////////////
	bool		Connect(void);

	//////////////////////////////////////////////////////////////////////
	// MySQL DB ����
	//////////////////////////////////////////////////////////////////////
	bool		Disconnect(void);


	//////////////////////////////////////////////////////////////////////
	// ���� ������ ����� �ӽ� ����
	//
	//////////////////////////////////////////////////////////////////////
	bool		Query(WCHAR *szStringFormat, ...);
	bool		Query_Save(WCHAR *szStringFormat, ...);
	// DBWriter �������� Save ���� ����
	// ������� �������� ����.

	//////////////////////////////////////////////////////////////////////
	// ������ ���� �ڿ� ��� �̾ƿ���.
	//
	// ����� ���ٸ� NULL ����.
	//////////////////////////////////////////////////////////////////////
	MYSQL_ROW	FetchRow(void);

	//////////////////////////////////////////////////////////////////////
	// �� ������ ���� ��� ��� ��� �� ����.
	//////////////////////////////////////////////////////////////////////
	void		FreeResult(void);

	//////////////////////////////////////////////////////////////////////
	// �� ������ ���� Row ���� ����
	//////////////////////////////////////////////////////////////////////
	int			FetchNum(void);


	//////////////////////////////////////////////////////////////////////
	// Error ���.�� ������ ���� ��� ��� ��� �� ����.
	//////////////////////////////////////////////////////////////////////
	int			GetLastError(void) { return _iLastError; };
	WCHAR		*GetLastErrorMsg(void) { return _szLastErrorMsg; }


private:

	//////////////////////////////////////////////////////////////////////
	// mysql �� LastError �� �ɹ������� �����Ѵ�.
	//////////////////////////////////////////////////////////////////////
	void		SaveLastError(void);

	//////////////////////////////////////////////////////////////////////
	// UTF8�� UTF16����
	//////////////////////////////////////////////////////////////////////
	void		UTF8toUTF16(const char *szText, WCHAR *szBuf, int iBufLen);

	//////////////////////////////////////////////////////////////////////
	// UTF16�� UTF8��
	//////////////////////////////////////////////////////////////////////
	void		UTF16toUTF8(WCHAR *szText, char *szBuf, int iBufLen);

private:

	//-------------------------------------------------------------
	// MySQL ���ᰴü ��ü
	//-------------------------------------------------------------
	MYSQL		_MySQL;

	//-------------------------------------------------------------
	// MySQL ���ᰴü ������. �� ������ ��������.
	// �� �������� null ���η� ������� Ȯ��.
	//-------------------------------------------------------------
	MYSQL		*_pMySQL;

	//-------------------------------------------------------------
	// ������ ���� �� Result �����.
	//
	//-------------------------------------------------------------
	MYSQL_RES	*_pSqlResult;

	char		_szDBIP[16];
	char		_szDBUser[64];
	char		_szDBPassword[64];
	char		_szDBName[64];
	int			_iDBPort;


	WCHAR		_szQuery[eQUERY_MAX_LEN];
	char		_szQueryUTF8[eQUERY_MAX_LEN];

	int			_iLastError;
	WCHAR		_szLastErrorMsg[128];

	CSystemLog	*_Log;
};


#endif