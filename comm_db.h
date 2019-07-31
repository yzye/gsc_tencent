#ifndef _COMM_DB_H_
#define _COMM_DB_H_

#include <unistd.h>
#include <iostream>
#include <string>
#include "thirdparty/mysql/mysql.h"
#include "thirdparty/mysql/errmsg.h"
#include "common/base/string/algorithm.h"

class CommDB 
{
public:
	CommDB();
	~CommDB();
	int Connect(std::string server, std::string usr, 
				std::string pwd, std::string db, int port = 3306);
	int ReConnect();
	int GetFieldCount(MYSQL_RES *result);
	MYSQL_FIELD* GetFields(MYSQL_RES *result);
	unsigned long long GetRowCount(MYSQL_RES *result);
	int SelectData(std::string sql, MYSQL_RES &res);
	long UpdateData(std::string sql);
	void FreeResult(MYSQL_RES *result);
	//void **TraverseResult(MYSQL_RES *res, int &row_count, int &col_count);
	int Commit();
	int Close();
	std::string GetErr();
private:
	std::string m_server;
	std::string m_user;
	std::string m_pwd;
	std::string m_db;
	std::string m_err;
	int m_port;
	MYSQL *mysql;
};

#endif
