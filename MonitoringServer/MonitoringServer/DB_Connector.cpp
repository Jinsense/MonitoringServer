#include <windows.h>
#include <wchar.h>

//#include "../mysql/include/mysql.h"
//#include "../mysql/include/errmsg.h"
#include "DB_Connector.h"

//#pragma comment(lib, "libmysql.lib")
//#pragma comment(lib, "/vs14/mysqlclient.lib")

CDBConnector::CDBConnector()
{

}

CDBConnector::~CDBConnector()
{

}

CDBConnector::CDBConnector(char *szDBIP, char *szUser, char *szPassword, char *szDBName, int iDBPort)
{
	memcpy_s(_szDBIP, sizeof(_szDBIP), szDBIP, sizeof(_szDBIP));
	memcpy_s(_szDBUser, sizeof(_szDBUser), szUser, sizeof(_szDBUser));
	memcpy_s(_szDBPassword, sizeof(_szDBPassword), szPassword, sizeof(_szDBPassword));
	memcpy_s(_szDBName, sizeof(_szDBName), szDBName, sizeof(_szDBName));
	_iDBPort = iDBPort;
	_Log = _Log->GetInstance();
}

bool CDBConnector::Set(char *szDBIP, char *szUser, char *szPassword, char *szDBName, int iDBPort)
{
	memcpy_s(_szDBIP, sizeof(_szDBIP), szDBIP, sizeof(_szDBIP));
	memcpy_s(_szDBUser, sizeof(_szDBUser), szUser, sizeof(_szDBUser));
	memcpy_s(_szDBPassword, sizeof(_szDBPassword), szPassword, sizeof(_szDBPassword));
	memcpy_s(_szDBName, sizeof(_szDBName), szDBName, sizeof(_szDBName));
	_iDBPort = iDBPort;
	_Log = _Log->GetInstance();

	if (false == Connect())
		return false;

	return true;
}

bool CDBConnector::Connect()
{
	mysql_init(&_MySQL);
	_pMySQL = mysql_real_connect(&_MySQL, (char*)_szDBIP, (char*)_szDBUser,
		(char*)_szDBPassword, (char*)_szDBName, 3306, (char*)NULL, 0);
	if (NULL == _pMySQL)
	{
		wprintf(L"DB Connect Error : %s\n", mysql_error(&_MySQL));
		return false;
	}
	mysql_set_character_set(_pMySQL, "utf8");
	return true;
}

bool CDBConnector::Disconnect()
{
	mysql_close(&_MySQL);
	return true;
}

bool CDBConnector::Query(WCHAR * szStringFormat, ...)
{
	//	result가 필요한 경우
	HRESULT hResult;
	va_list vList;
	int	iError;
	int iSpinCount = 0;

	va_start(vList, szStringFormat);
	hResult = StringCchVPrintf(_szQuery, sizeof(_szQuery), szStringFormat, vList);
	va_end(vList);

	if (FAILED(hResult))
	{
		//	에러가 떴을 경우 예외처리
		return false;
	}

	//	WCHAR를 CHAR로
	UTF16toUTF8(_szQuery, _szQueryUTF8, sizeof(_szQuery));

	while (10 > iSpinCount)
	{
		iError = mysql_query(_pMySQL, _szQueryUTF8);
		if (0 != iError)
		{
			if (10 > iSpinCount)
			{
				iSpinCount++;
				continue;
			}
			else
			{
				SaveLastError();
				return false;
			}
		}
		else
		{
			break;
		}

	}
	_pSqlResult = mysql_store_result(&_MySQL);
	return true;
}

bool CDBConnector::Query_Save(WCHAR * szStringFormat, ...)
{
	//	result가 필요없는 경우
	HRESULT hResult;
	va_list vList;
	int	iError;
	int iSpinCount = 0;

	va_start(vList, szStringFormat);
	hResult = StringCchVPrintf(_szQuery, sizeof(_szQuery), szStringFormat, vList);
	va_end(vList);

	if (FAILED(hResult))
	{
		//	에러가 떴을 경우 예외처리
		return false;
	}

	//	WCHAR를 CHAR로
	UTF16toUTF8(_szQuery, _szQueryUTF8, sizeof(_szQuery));

	while (10 > iSpinCount)
	{
		iError = mysql_query(_pMySQL, _szQueryUTF8);
		if (0 != iError)
		{
			if (10 > iSpinCount)
			{
				iSpinCount++;
				continue;
			}
			else
			{
				SaveLastError();
				return false;
			}
		}
		else
		{
			break;
		}
	}
	FreeResult();
	return true;
}

MYSQL_ROW CDBConnector::FetchRow()
{
	return mysql_fetch_row(_pSqlResult);
}

void CDBConnector::FreeResult()
{
	//	쿼리에 대한 결과를 모두 정리, 피드백이 필요없는 경우도 항상 호출해서 정리
	mysql_free_result(_pSqlResult);
	return;
}

int CDBConnector::FetchNum()
{
	int totalrows = mysql_num_rows(_pSqlResult);
	return mysql_num_fields(_pSqlResult);
}

void CDBConnector::SaveLastError()
{
	//	실패한 에러 코드와 에러번호 저장
	_iLastError = mysql_errno(&_MySQL);
	memcpy_s(_szLastErrorMsg, sizeof(_szLastErrorMsg),
		mysql_error(&_MySQL), sizeof(_szLastErrorMsg));
	return;
}

void CDBConnector::UTF8toUTF16(const char *szText, WCHAR *szBuf, int iBufLen)
{
	int iRe = MultiByteToWideChar(CP_UTF8, 0, szText, strlen(szText), szBuf, iBufLen);
	if (iRe < iBufLen)
		szBuf[iRe] = L'\0';
	return;
}

void CDBConnector::UTF16toUTF8(WCHAR *szText, char *szBuf, int iBufLen)
{
	int iRe = WideCharToMultiByte(CP_UTF8, 0, szText, lstrlenW(szText), szBuf, iBufLen, NULL, NULL);
	return;
}