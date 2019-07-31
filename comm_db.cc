
#include "spider/code/spider/gsc_qa/comm_db.h"

using namespace std;

CommDB::CommDB()
{
}

CommDB::~CommDB()
{
}


int CommDB::Connect(string server, string usr, string pwd, 
					string db, int port)
{
	m_server = server;
	m_user = usr;
	m_pwd = pwd;
	m_db = db;
	m_port = port;
	mysql = NULL;
	if (!(mysql = mysql_init(mysql)))
	{
		m_err = "mysql init failed";
		//std::cout << "mysql init failed" << std::endl;
		return 0;
	}

	if (!mysql_real_connect(mysql, m_server.c_str(), m_user.c_str(), 
				m_pwd.c_str(), m_db.c_str(), m_port, NULL, 0))
	{
		m_err = "mysql connect failed";
		//std::cout << "mysql connect failed" << std::endl;
		return 0;
	}
	if (!mysql_set_character_set(mysql, "UTF-8"))
	{
		m_err = "mysql fail to set character";
		//std::cout << "mysql fail to set character" << std::endl;
	}
	mysql_autocommit(mysql, false);

	return 1;
}

int CommDB::ReConnect()
{
	Close();
	mysql = NULL;
	if (!(mysql = mysql_init(mysql)))
	{
		m_err = "mysql init failed";
		//std::cout << "mysql init failed" << std::endl;
		return 0;
	}

	if (!mysql_real_connect(mysql, m_server.c_str(), m_user.c_str(), 
				m_pwd.c_str(), m_db.c_str(), m_port, NULL, 0))
	{
		m_err = "mysql connect failed";
		//std::cout << "mysql connect failed" << std::endl;
		return 0;
	}
	if (!mysql_set_character_set(mysql, "UTF-8"))
	{
		m_err = "mysql fail to set character";
		//std::cout << "mysql fail to set character" << std::endl;
	}
	mysql_autocommit(mysql, false);

	return 1;
}

int CommDB::GetFieldCount(MYSQL_RES *result)
{
	return mysql_num_fields(result);
}

MYSQL_FIELD* CommDB::GetFields(MYSQL_RES *result)
{
	return mysql_fetch_fields(result);
}

unsigned long long CommDB::GetRowCount(MYSQL_RES *result)
{
	return mysql_num_rows(result);
}

int CommDB::SelectData(string sql, MYSQL_RES &res)
{
	MYSQL_RES *l_res;
	
	if (mysql_query(mysql,sql.c_str()))
	{
		int err_no = mysql_errno(mysql);
		m_err = "mysql query failed";
		std::cout << "errno:" << err_no << std::endl;
		if ((err_no == CR_SERVER_LOST || 
			 err_no == CR_SERVER_GONE_ERROR) && 
			 !ReConnect())
			return 0;

		//重试
		if (mysql_query(mysql,sql.c_str()))
			return 0;
	}

	l_res = mysql_store_result(mysql);
	if (!l_res)
	{
		return 0;
	}

	//std::cout << "actual fields num :" << mysql_num_fields(l_res) << std::endl;
	res = *l_res;
	return 1;
}

long CommDB::UpdateData(string sql)
{
	if (mysql_query(mysql, sql.c_str()))
	{
		int err_no = mysql_errno(mysql);
		m_err = "mysql query failed";
		std::cout << "errno:" << err_no << std::endl;
		if ((err_no == CR_SERVER_LOST || 
			 err_no == CR_SERVER_GONE_ERROR) && 
			 !ReConnect())
			return 0;

		//重试
		if (mysql_query(mysql,sql.c_str()))
			return 0;
	}

	return (long)mysql_affected_rows(mysql);
}
/*
void **CommDB::TraverseResult(MYSQL_RES *res, 
		int &row_count, int &col_count)
{
	MYSQL_ROW *row;
	row_count = 0;
	col_count = 0;
	
	row_count = mysql_num_rows(res);
	col_count = mysql_num_fields(res);
	char ***res_arr = new (char*)[row_count][col_count];
	while ((row = mysql_fetch_row(res)))
	{
		
	}
}
*/
void CommDB::FreeResult(MYSQL_RES *result)
{
	//if (result)
	//	mysql_free_result(result);
}

int CommDB::Commit()
{
	if (mysql_commit(mysql))
		mysql_rollback(mysql);
}

int CommDB::Close()
{
	mysql_close(mysql);
}

string CommDB::GetErr()
{
	return m_err; 
}
