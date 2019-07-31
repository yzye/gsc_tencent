#ifndef _QA_STAT_H_
#define _QA_STAT_H_

#include <iostream>
#include <string>
//#include "common/base/singleton.h"
#include "common/system/timer/timer_manager.h"
#include "common_stat.h"

using namespace std;

class CQAStat
{
public:
	static CQAStat *Instance();
    int Init(string str_name_prefix, int max_file_size, 
			 int max_file_index, int timeout_1, 
			 int timeout_2, int timeout_3);
	void AddStat(std::string str_stat_name, int ret_code, 
			 struct timeval& start_time, struct timeval& end_time);
	void AddStatWithNoTime(std::string str_stat_name, 
			 int ret_code, int count=1);
    void ReportCommonStat(const uint64_t id);
    void InitTimers();
	void Destroy();
private:
	CQAStat();
	~CQAStat();
private:
	static CQAStat *m_qa_stat_instance;
	CStatistic m_comm_stat;
    TimerManager m_timer_manager;
};

#endif
