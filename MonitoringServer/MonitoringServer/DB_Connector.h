#ifndef _SERVER_DB_DBCONNECTOR_H_
#define _SERVER_DB_DBCONNECTOR_H_

#include "../mysql/include/mysql.h"
#include "../mysql/include/errmsg.h"

#include "Log.h"

#pragma comment(lib, "libmysql.lib")
#pragma comment(lib, "/vs14/mysqlclient.lib")

/*


/////////////////////////////////////////////////////////
// MySQL DB 연결 클래스
//
// 단순하게 MySQL Connector 를 통한 DB 연결만 관리한다.
//
// 스레드에 안전하지 않으므로 주의 해야 함.
// 여러 스레드에서 동시에 이를 사용한다면 개판이 됨.
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
// MySQL DB 연결
//////////////////////////////////////////////////////////////////////
bool		Connect(void);

//////////////////////////////////////////////////////////////////////
// MySQL DB 끊기
//////////////////////////////////////////////////////////////////////
bool		Disconnect(void);


//////////////////////////////////////////////////////////////////////
// 쿼리 날리고 결과셋 임시 보관
//
//////////////////////////////////////////////////////////////////////
bool		Query(WCHAR *szStringFormat, ...);
bool		Query_Save(WCHAR *szStringFormat, ...);	// DBWriter 스레드의 Save 쿼리 전용
// 결과셋을 저장하지 않음.

//////////////////////////////////////////////////////////////////////
// 쿼리를 날린 뒤에 결과 뽑아오기.
//
// 결과가 없다면 NULL 리턴.
//////////////////////////////////////////////////////////////////////
MYSQL_ROW	FetchRow(void);

//////////////////////////////////////////////////////////////////////
// 한 쿼리에 대한 결과 모두 사용 후 정리.
//////////////////////////////////////////////////////////////////////
void		FreeResult(void);


//////////////////////////////////////////////////////////////////////
// Error 얻기.한 쿼리에 대한 결과 모두 사용 후 정리.
//////////////////////////////////////////////////////////////////////
int			GetLastError(void) { return _iLastError; };
WCHAR		*GetLastErrorMsg(void) { return _szLastErrorMsg; }


private:

//////////////////////////////////////////////////////////////////////
// mysql 의 LastError 를 맴버변수로 저장한다.
//////////////////////////////////////////////////////////////////////
void		SaveLastError(void);

private:



//-------------------------------------------------------------
// MySQL 연결객체 본체
//-------------------------------------------------------------
MYSQL		_MySQL;

//-------------------------------------------------------------
// MySQL 연결객체 포인터. 위 변수의 포인터임.
// 이 포인터의 null 여부로 연결상태 확인.
//-------------------------------------------------------------
MYSQL		*_pMySQL;

//-------------------------------------------------------------
// 쿼리를 날린 뒤 Result 저장소.
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





// Query 함수 내에서 연결관련 에러 체크

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
	// MySQL DB 연결
	//////////////////////////////////////////////////////////////////////
	bool		Connect(void);

	//////////////////////////////////////////////////////////////////////
	// MySQL DB 끊기
	//////////////////////////////////////////////////////////////////////
	bool		Disconnect(void);


	//////////////////////////////////////////////////////////////////////
	// 쿼리 날리고 결과셋 임시 보관
	//
	//////////////////////////////////////////////////////////////////////
	bool		Query(WCHAR *szStringFormat, ...);
	bool		Query_Save(WCHAR *szStringFormat, ...);
	// DBWriter 스레드의 Save 쿼리 전용
	// 결과셋을 저장하지 않음.

	//////////////////////////////////////////////////////////////////////
	// 쿼리를 날린 뒤에 결과 뽑아오기.
	//
	// 결과가 없다면 NULL 리턴.
	//////////////////////////////////////////////////////////////////////
	MYSQL_ROW	FetchRow(void);

	//////////////////////////////////////////////////////////////////////
	// 한 쿼리에 대한 결과 모두 사용 후 정리.
	//////////////////////////////////////////////////////////////////////
	void		FreeResult(void);

	//////////////////////////////////////////////////////////////////////
	// 한 쿼리에 대한 Row 갯수 리턴
	//////////////////////////////////////////////////////////////////////
	int			FetchNum(void);


	//////////////////////////////////////////////////////////////////////
	// Error 얻기.한 쿼리에 대한 결과 모두 사용 후 정리.
	//////////////////////////////////////////////////////////////////////
	int			GetLastError(void) { return _iLastError; };
	WCHAR		*GetLastErrorMsg(void) { return _szLastErrorMsg; }


private:

	//////////////////////////////////////////////////////////////////////
	// mysql 의 LastError 를 맴버변수로 저장한다.
	//////////////////////////////////////////////////////////////////////
	void		SaveLastError(void);

	//////////////////////////////////////////////////////////////////////
	// UTF8을 UTF16으로
	//////////////////////////////////////////////////////////////////////
	void		UTF8toUTF16(const char *szText, WCHAR *szBuf, int iBufLen);

	//////////////////////////////////////////////////////////////////////
	// UTF16을 UTF8로
	//////////////////////////////////////////////////////////////////////
	void		UTF16toUTF8(WCHAR *szText, char *szBuf, int iBufLen);

private:

	//-------------------------------------------------------------
	// MySQL 연결객체 본체
	//-------------------------------------------------------------
	MYSQL		_MySQL;

	//-------------------------------------------------------------
	// MySQL 연결객체 포인터. 위 변수의 포인터임.
	// 이 포인터의 null 여부로 연결상태 확인.
	//-------------------------------------------------------------
	MYSQL		*_pMySQL;

	//-------------------------------------------------------------
	// 쿼리를 날린 뒤 Result 저장소.
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