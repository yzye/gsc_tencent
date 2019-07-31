
#include "qa_stat.h"

CQAStat *CQAStat::m_qa_stat_instance = NULL;

CQAStat::CQAStat()
{
}

CQAStat *CQAStat::Instance()
{
	if (!CQAStat::m_qa_stat_instance)
		CQAStat::m_qa_stat_instance = new CQAStat();
	return CQAStat::m_qa_stat_instance;
}

int CQAStat::Init(std::string str_name_prefix, int max_file_size, 
				int max_file_index, int timeout_1, 
				int timeout_2, int timeout_3)
{
    //m_last_report_time = GetTimeStampInUs();

	m_comm_stat.Initialize((char*)str_name_prefix.c_str(),max_file_size, max_file_index, timeout_1, timeout_2, timeout_3);
    
	InitTimers();

    return 0;
}

void CQAStat::AddStat(string str_stat_name, int ret_code, 
		struct timeval& start_time, struct timeval& end_time)
{
	m_comm_stat.AddStat(const_cast<char*>(str_stat_name.c_str()), 
			ret_code, &start_time, &end_time);
}

void CQAStat::AddStatWithNoTime(string str_stat_name, 
		int ret_code, int count)
{
	m_comm_stat.AddStat(const_cast<char*>(str_stat_name.c_str()), 
			ret_code, NULL, NULL,NULL,count);
}

void CQAStat::ReportCommonStat(const uint64_t id)
{
	m_comm_stat.WriteToFile();
	m_comm_stat.ClearStat();
}

void CQAStat::InitTimers()
{
	TimerManager::CallbackFunction common_stat_func = \
			Bind(&CQAStat::ReportCommonStat,this);
	m_timer_manager.AddPeriodTimer(60000,\
			common_stat_func);
}

void CQAStat::Destroy()
{
	if (m_qa_stat_instance)
	{
		delete m_qa_stat_instance;
		m_qa_stat_instance = NULL;
	}
}
