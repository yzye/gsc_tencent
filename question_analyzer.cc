
#include "question_analyzer.h"
#include "comm_db.h"
#include "qa_log.h"
#include "common/encoding/charset_converter.h"

QuestionAnalyzer *QuestionAnalyzer::instance;
QuestionAnalyzer::QuestionAnalyzer(){
	/*
	char *p_str[] = {
		"(.*)写了哪些诗",
		"(.*)诗集",
		"(.*)是哪个朝代的诗人",
		"(.*)(的)?作者(是谁)?",
		"(.*)(的)?完整内容(是什么)?",
		"(.*)(的)?内容(是什么)?",
		"(.*)(的)?风格(是什么)?",
		"(.*)(的)?注释(是什么)?",
		"(.*)是哪个朝代的",
		"(.*)出自哪个朝代",
		"(.*)朝代",
		"(.*)[代朝](的)?诗人(有哪些)?",
		"(.*)[代朝](的)?诗(有哪些)?",
		"(.*)是谁(写)?的(诗)?",
		"(.*)出自哪首诗",
		"(.*)(的)?上一句(是什么)?",
		"(.*)(的)?前一句(是什么)?",
		"(.*)(的)?下一句(是什么)?",
		"(.*)(的)?后一句(是什么)?",
		"(.*)(的)?情感描述(是什么)?",
	};

	unsigned ttypes[][2] = {
        {TARGET_AUTHOR, TARGET_TITLE},
        {TARGET_AUTHOR, TARGET_TITLE},
        {TARGET_AUTHOR, TARGET_DYNASTY},
        {TARGET_TITLE|TARGET_CONTENT, TARGET_AUTHOR},
        {TARGET_TITLE|TARGET_CONTENT, TARGET_CONTENT},
        {TARGET_TITLE|TARGET_CONTENT, TARGET_CONTENT},
        {TARGET_TITLE|TARGET_CONTENT, TARGET_STYLE},
        {TARGET_TITLE|TARGET_CONTENT, TARGET_ANNOTATION},
        {TARGET_TITLE|TARGET_AUTHOR|TARGET_CONTENT, TARGET_DYNASTY},
        {TARGET_TITLE|TARGET_AUTHOR|TARGET_CONTENT, TARGET_DYNASTY},
        {TARGET_TITLE|TARGET_AUTHOR|TARGET_CONTENT, TARGET_DYNASTY},
        //{TARGET_CONTENT|TARGET_TITLE, TARGET_DYNASTY},
        {TARGET_DYNASTY, TARGET_AUTHOR},
        {TARGET_DYNASTY, TARGET_TITLE},
        {TARGET_CONTENT|TARGET_TITLE, TARGET_AUTHOR},
        {TARGET_CONTENT, TARGET_TITLE},
        {TARGET_CONTENT, TARGET_CONTENT_L},
        {TARGET_CONTENT, TARGET_CONTENT_L},
        {TARGET_CONTENT, TARGET_CONTENT_N},
        {TARGET_CONTENT, TARGET_CONTENT_N},
        {TARGET_TITLE|TARGET_CONTENT, TARGET_ANNOTATION},
	};
	int size = sizeof(p_str)/sizeof(char*);
	int i = 0;
	for (; i < size; i ++)
	{
		Pattern pat;
		//regcomp(&pat.reg, p_str[i], REG_EXTENDED);
		pat.reg = new Regex(p_str[i], Regex::Options().SetExtended(true));
		pat.ttype = ttypes[i][1];
        pat.ftype = ttypes[i][0];
		pattern_vec.push_back(pat);
	}
	*/
	if (m_allow_token)
		LoadRind();
	return;
}

QuestionAnalyzer * QuestionAnalyzer::GetInstance(){
	if (!instance)
		instance = new QuestionAnalyzer();

	return instance;
}

void QuestionAnalyzer::AddPattern(Pattern pattern)
{
	pattern.reg = new Regex(pattern.pattern.c_str(), Regex::Options().SetExtended(true));
	pattern_vec.push_back(pattern);
}

void QuestionAnalyzer::ClearPattern()
{
	pattern_vec.clear();
}

void QuestionAnalyzer::TranslateTarget(unsigned ttype, std::vector<string> &vf)
{
	char *fields[] = {"author", "title", "dynasty", "style", "content",
					  "content_next", "content_last", "annotation"};
	int size = sizeof(fields)/sizeof(char *);
	int i;
	for (i = 0; i < size; i++)
	{
		unsigned mask = ((unsigned)1) << i;
		if (ttype & mask)
		{
			string tmp(fields[i]);
			vf.push_back(tmp);
		}
	}
}

int QuestionAnalyzer::Tokenizer(string question, vector<string> & fields,
				string & target, string & context)
{
	if (question.length() == 0)
		return 0;

	//const size_t nmatch = 10;
	//regmatch_t pm[10];
	int i = 0;
	vector<Pattern>::iterator it = pattern_vec.begin();
	while (it != pattern_vec.end())
	{
		//int ret = regexec(&it->reg, question.c_str(),nmatch, pm, REG_NOTEOL);
		//string cap1;
		bool ret = it->reg->FullMatch(question.c_str(), &context);
		if (ret)
		{
			fields = it->fields;
			target = it->target;
			StringTrimRight(&context, "的");
			StringTrim(&context, '\n');
			StringTrim(&context, '\t');
			return 1;
		}
		it ++;
		i ++;
	}
	return 0;
}

QuestionAnalyzer::~QuestionAnalyzer()
{
	vector<Pattern>::iterator it = pattern_vec.begin();
	while (it != pattern_vec.end())
	{
		if (it->reg)
		{
			delete it->reg;
			it->reg = NULL;
		}
	}
}

int QuestionAnalyzer::CheckPid(string f1, string f2, 
			vector<int> &match_pid, int &match_point)
{
	match_point = 0;
	list<int> tmp_list_1;
	list<int> tmp_list_2;
	hash_map<string, list<int> >::iterator it;
	if ((it = rind_map.find(f1)) != rind_map.end())
	{
		match_point = f1.length();;
		tmp_list_1 = it->second;
	}
	else if (f1.length() == 0)
		match_point ++;

	if ((it = rind_map.find(f2)) != rind_map.end())
	{
		match_point = f2.length();
		tmp_list_2 = it->second;
	}
	else if (f2.length() == 0)
		match_point ++;

	list<int>::iterator it2;
	if (!tmp_list_1.size() && !tmp_list_2.size())
	{	
		LOG_PRINT(LOG_NORMAL,"no list has element\n");
		return 0;
	}
	else if (tmp_list_1.size() && !tmp_list_2.size())
	{
		for (it2 = tmp_list_1.begin(); it2 != tmp_list_1.end(); it2 ++)
			match_pid.push_back(*it2);
		LOG_PRINT(LOG_NORMAL, "1.matched pid num is %u", match_pid.size());
	}
	else if (tmp_list_2.size() && !tmp_list_1.size())
	{
		for (it2 = tmp_list_1.begin(); it2 != tmp_list_1.end(); it2 ++)
			match_pid.push_back(*it2);
		LOG_PRINT(LOG_NORMAL, "2.matched pid num is %u", match_pid.size());
	}
	else
	{
		for (it2 = tmp_list_1.begin(); it2 != tmp_list_1.end(); it2 ++)
		{
			if (find(tmp_list_2.begin(), tmp_list_2.end(), *it2) != tmp_list_2.end())
				match_pid.push_back(*it2);
		}
		LOG_PRINT(LOG_NORMAL, "3.matched pid num is %u", match_pid.size());
	}
	LOG_PRINT(LOG_NORMAL, "match point is %u", match_point);
	return 1;
}

int QuestionAnalyzer::LoadRind()
{
	char * tables[] = {"tb_title", "tb_author", "tb_dynasty", \
					   "tb_content", "tb_style"};
	CommDB db;
	//db.Connect("10.173.13.40", "root", "root40", "qa_test");
	db.Connect("10.198.30.113", "root", "root113", "qa_test", 3335);

	LOG_PRINT(LOG_NORMAL, "LoadRind\n");
	int tb_size = sizeof(tables)/sizeof(char*);
	for (int i = 0; i < tb_size; i++)
	{
		string sql = "SELECT * FROM ";
		MYSQL_RES res;
		MYSQL_ROW row;

		sql += tables[i];
		if (!db.SelectData(sql, res))
		{
			LOG_PRINT(LOG_NORMAL, "sql: %s\n", sql);
			continue;
		}
		int num = 0;
		if ((num = mysql_num_fields(&res)) != 3)
		{
			LOG_PRINT(LOG_NORMAL, "Wrong Fields Num(should be 3), But Not It is %u\n", num);
			db.FreeResult(&res);
			continue;
		}
		while ((row = mysql_fetch_row(&res)))
		{
			//std::cout << "num fields:" << mysql_num_fields(&res) << std::endl;
			hash_map<string, list<int> >::iterator it = 
							rind_map.find(row[1]);
			if (it == rind_map.end())
			{
				list<int> q;
				vector<string> vs;
				SplitStringByDelimiter(row[2], ",", &vs);
				for (vector<string>::iterator vit = vs.begin();
					 vit != vs.end(); vit ++)
				{
					q.push_back(atoi(vit->c_str()));
				}
				rind_map.insert(pair<string, list<int> >(row[1],q));
			}
			else
			{
				list<int> q = it->second;
				vector<string> vs;
				SplitStringByDelimiter(row[2], ",", &vs);
				for (vector<string>::iterator vit = vs.begin();
					vit != vs.end(); vit ++)
				{
					if (find(q.begin(), q.end(),atoi(vit->c_str())) == q.end())
					{
						q.push_back(atoi(vit->c_str()));
					}
				}
			}
		}
		db.FreeResult(&res);
	}
	db.Close();
}

int QuestionAnalyzer::SplitAndCheck(string qu, vector<int> &pids)
{
		//vector<int> opt_vec;
		int max_match_point = 0;
		ReplaceAll(&qu, " ", "");
		//QuestionAnalyzer * qa = QuestionAnalyzer::GetInstance();
		string rule = "的";
		int u_size;
		int qu_size = qu.length();
		int tmp_match_point = 0;
		vector<int> tmp_match_pid;
		//ConvertGbkToUtf8("的", &rule);
		u_size = rule.length();
		LOG_PRINT(LOG_NORMAL, "qu_size = %d, u_size = %u\n", qu_size, u_size);
		if (qu_size/u_size <= 2)
		{
			if (CheckPid(qu,"",tmp_match_pid,tmp_match_point))
			{
				int i = 0;
				vector<int>::iterator it = tmp_match_pid.begin();
				for (; it != tmp_match_pid.end(); it ++)
					LOG_PRINT(LOG_NORMAL, "pid%u:%d\n", i++,*it);
				return 1;
			}

			return 0;
		}

		for(int i = 0 + u_size; i <= qu_size; i += u_size)
		{

			string f1 = qu.substr(0, i);
			string f2 = qu.substr(i,qu_size-i);
			LOG_PRINT(LOG_NORMAL, "f1:%s,f2:%s\n", f1.c_str(), f2.c_str());
		

			tmp_match_point = 0;
			tmp_match_pid.clear();

			if (!CheckPid(f1,f2,tmp_match_pid,tmp_match_point))
				continue;

			if (tmp_match_point > max_match_point)
			{
				max_match_point = tmp_match_point;
				pids = tmp_match_pid;
			}
		}

		vector<int>::iterator it = pids.begin();
		int i = 0;
		LOG_PRINT(LOG_NORMAL, "Final Result Num Is %u\n", pids.size());
		for (; it != pids.end(); it ++)
		{
			LOG_PRINT(LOG_NORMAL, "pid%u:%d\n",i++, *it);
		}
		return 1;
}

extern "C" {
	int tokenizer(string qu, unsigned & field_type, unsigned & target_type, string & context)
	{
		//unsigned tt, tt2;
		ReplaceAll(&qu, " ", "");
		//std::cout << "question is " << qu << std::endl;
		QuestionAnalyzer * qa = QuestionAnalyzer::GetInstance();
		//int ret = qa->Tokenizer(qu, field_type,target_type, context);
        //field_type = (int)tt;
		//target_type = (int)tt2;

		//return ret;
	}

	int tokenizer2(string qu, vector<int> &pids)
	{
		QuestionAnalyzer * qa = QuestionAnalyzer::GetInstance();
		return qa->SplitAndCheck(qu, pids);
	}
}
