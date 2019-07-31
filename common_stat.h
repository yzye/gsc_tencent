/**************************************************************************

DESC: 统计类:TypeName为大类,IDName为小类,大类对小类有统计

AUTHOR: nekeyzhong

DATE: 2007年9月

PROJ: Part of MCP Project

 **************************************************************************/
#ifndef __STATISTIC_H__
#define __STATISTIC_H__

#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <pthread.h>

#define TYPE_NUM 256
#define TYPE_NAME_LEN	64
#define MAX_TIMEOUT_LEVEL	3

#define STAT_RESULT_OK	0
#define STAT_RESULT_FAILED	(-1)

typedef struct
{
	unsigned long long tv_sec;
	unsigned long long tv_usec;
}ULLTimeVal;

typedef struct
{
	char m_szTypeName[TYPE_NAME_LEN];
	unsigned int m_unAllCount;
	unsigned int m_unFailedCount;
	unsigned int m_unMaxTime;
	unsigned int m_unMinTime;
	ULLTimeVal m_stTime;
	unsigned int m_unTimeOut[MAX_TIMEOUT_LEVEL];
	char m_szRecordAtMax[TYPE_NAME_LEN];
}TypeInfo;

class CStatistic
{
public:
	static CStatistic* Instance();
	static CStatistic* m_pInstance;

	CStatistic(bool bUseMutex=false);
	~CStatistic();

	//iTimeOutUs :时间计数标尺,单位us
	int Initialize(char *pszLogBaseFile=(char *)"./Statistic",
		int iMaxSize=20000000,
		int iMaxNum=10,
		int iTimeOutUs1=100000,
		int iTimeOutUs2=500000,
		int iTimeOutUs3=1000000);

	//pTypeName: 统计名称
	//szRecordAtMax: 最多延时产生时记录的数据
	//iStatCount统计个数
	int AddStat(char* szTypeName,
				int iResultID=STAT_RESULT_OK,
				struct timeval *pstBegin=NULL,
				struct timeval *pstEnd=NULL,
				char* szRecordAtMax=NULL,
				int iStatCount=1);

	int GetStat(char* szTypeName,TypeInfo &stTypeInfo);

	int WriteToFile();
	void ClearStat();

private:
	static void GetDateString(char *szTimeStr);

	int _AddStat(char* pszTypeName,
				int iResultID,
				struct timeval *pstBegin,
				struct timeval *pstEnd,
				char* szRecordAtMax,
				int iStatCount);
	int _GetStat(char* szTypeName,TypeInfo &stTypeInfo);

	int _ShiftFiles();
	void _WriteLog(const char *sFormat, ...);
	void _AddTime(TypeInfo* pTypeInfo,
				struct timeval *pstBegin,
				struct timeval *pstEnd,
				char* szRecordAtMax);

	//互斥使用
	bool m_bUseMutex;
	pthread_mutex_t m_stMutex;

	int m_iTypeNum;
	TypeInfo m_astTypeInfo[TYPE_NUM];

	char m_szLogBase[256];
	int m_iLogMaxSize;
	int m_iLogMaxNum;

	unsigned int m_iTimeOutUs[MAX_TIMEOUT_LEVEL];

	int m_iLastClearTime;
};
#endif

