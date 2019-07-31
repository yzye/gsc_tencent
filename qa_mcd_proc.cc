
#include <algorithm.h>
#include <sys/time.h>
#include "common/file/file.h"
#include "common/file/local_file.h"
#include "common/file/file_define.h"
#include "qa_mcd_proc.h"
#include "question_analyzer.h"
#include "comm_db.h"
#include "common/encoding/charset_converter.h"
#include "common/file/file_change_notifier.h"
//#include "dqa_rootmerge.proto.h"

using namespace std;
using namespace tfc::cache;
using namespace tfc::base;
using namespace pb::dqa_gsc;
using namespace pb::dqa_rootmerge;
using namespace common;
using namespace common::internal;

class common::FileChangeNotifier;

const string basic_select_string = "SELECT pid,title,tb_author_info.name as author,tb_poem.dynasty,\
content,translation,appreciation,style,score,rhesis,rhesis_tag,popularity,tb_author_info.cv as author_info,\
tb_author_info.authority as authority FROM tb_poem, tb_author_info WHERE tb_poem.author_id = tb_author_info.id";

CQAServ::CQAServ()
{
	m_allow_regex = 0;
	m_qa_stat = CQAStat::Instance();
}

string CQAServ::ConvUtf8ToGbk(string utf8_str)
{
	string gbk_str;
	if (!ConvertUtf8ToGbk(utf8_str, &gbk_str))
	{
		LOG_PRINT(LOG_NORMAL, "convert failed\n");
		gbk_str = utf8_str;
	}
	return gbk_str;
}

string CQAServ::ConvGbkToUtf8(string gbk_str)
{
	string utf8_str;
	if (!ConvertGbkToUtf8(gbk_str, &utf8_str))
	{
		LOG_PRINT(LOG_NORMAL, "convert failed\n");
		utf8_str = gbk_str;
	}
	return utf8_str;
}

void CQAServ::DebugPB(GscResponse *p)
{
	LOG_PRINT(LOG_NORMAL, "title:%s\n", ConvGbkToUtf8(p->title()).c_str());
	LOG_PRINT(LOG_NORMAL, "author:%s\n", ConvGbkToUtf8(p->author().name()).c_str());
	LOG_PRINT(LOG_NORMAL, "author_authority:%.5f\n", p->author().authority());
	LOG_PRINT(LOG_NORMAL, "author_cv:%s\n", ConvGbkToUtf8(p->author().cv_info()).c_str());
	LOG_PRINT(LOG_NORMAL, "dynasty:%s\n", ConvGbkToUtf8(p->dynasty()).c_str());
	LOG_PRINT(LOG_NORMAL, "content:%s\n", ConvGbkToUtf8(p->content()).c_str());
	LOG_PRINT(LOG_NORMAL, "translation:%s\n", ConvGbkToUtf8(p->annotation()).c_str());
	LOG_PRINT(LOG_NORMAL, "appreciation:%s\n", ConvGbkToUtf8(p->appreciation()).c_str());
	LOG_PRINT(LOG_NORMAL, "style:%s\n", ConvGbkToUtf8(p->style()).c_str());
	LOG_PRINT(LOG_NORMAL, "complexity:%d\n", p->complexity());
	LOG_PRINT(LOG_NORMAL, "popularity:%.5f\n", p->popularity());
}

int CQAServ::CountWeight(GscAnswer *ans, ResultVec rv, 
				MYSQL_ROW row, MYSQL_FIELD *fields, unsigned fields_num)
{
	float point = 1;
	int i = 0;
	int s = rv.size();
	if (!ans || !fields)
		return 0;
	for (; i < s; i ++)
	{
		int j = 0;
		for (; j < fields_num; j ++)
		{
			if (!rv[i].tag.compare(fields[j].name))
			{
				int r_len = strlen(row[j]);
				int c_len = rv[i].value.length();
				float l_point;
				if (r_len > c_len)
					l_point = (float) c_len / (float) r_len;
				else
					l_point = (float) r_len / (float) c_len;
				point *= l_point;
			}
		}
	}

	ans->point = point;
}

int CQAServ::SelectTarget(string sql, vector<GscAnswer> &ans_vec, 
						string field, string context)
{
	MYSQL_RES res;
	MYSQL_ROW row;
	MYSQL_FIELD* fields;
	int i = 0, fields_num = 0;
	//sql += " limit 0, 5";
	sql += " order by popularity desc limit 0, 20";
	LOG_PRINT(LOG_NORMAL, "sql:%s, context:%s\n", sql.c_str(), context.c_str());
	int ret = m_db.SelectData(sql, res);
	if (!ret)
	{
		LOG_PRINT(LOG_NORMAL, "select failed, sql:%s\n", sql.c_str());
		//m_db.Close();
		return ret;
	}

	fields_num = m_db.GetFieldCount(&res);
	fields = m_db.GetFields(&res);
	for (; i < fields_num; i ++)
	{
		//LOG_PRINT(LOG_NORMAL, "field:%s, fields[%d]:%s\n", field.c_str(),
		//	i, fields[i].name);
		if (!field.compare(fields[i].name))
			break;
	}
	LOG_PRINT(LOG_NORMAL, "i = %d\n", i);
	if (i >= fields_num) return 0;
	while ((row = mysql_fetch_row(&res)))
	{
		GscAnswer ans;
		//ans.answer = row[0];
		Poem *p = new Poem();
		p->title = row[1];
		p->author->name = row[2];
		p->author->cv_info = row[12];
		p->author->authority = atof(row[13]);
		p->dynasty = row[3];
		p->content = row[4];
		p->translation = row[5];
		p->appreciation = row[6];
		p->style = row[7];
		p->rhesis = row[9];
		p->rhesis_tag = row[10];
		p->popularity = atof(row[11]);
		unsigned r_len = strlen(row[i]);
		unsigned c_len = context.size();
		if (r_len >= c_len)
			ans.point = (float)c_len / (float)r_len;
		else
			ans.point = (float)r_len / (float)c_len;
		//ans.answer = 1.0;
		struct result fv;
		fv.tag = field;
		fv.value = context;
		p->fv_vec.push_back(fv);
		ans.poem = p;
		ans_vec.push_back(ans);
	}

	m_db.FreeResult(&res);
	return 1;
}

int CQAServ::SelectPoem(string sql, vector<GscAnswer> &ans_vec)
{
	MYSQL_RES res;
	MYSQL_ROW row;
	//CommDB db;
	//m_db.Connect("10.198.30.113", "root", "root113", "qa_test", 3335);
	sql += " order by popularity desc limit 0, ";
	sql += (char)('0' + MAX_RESULT_NUM);
	int ret = m_db.SelectData(sql, res);
	LOG_PRINT(LOG_NORMAL, "sql:%s\n", sql.c_str());
	if (!ret)
	{
		LOG_PRINT(LOG_NORMAL, "select failed, sql:%s\n", sql.c_str());
		return ret;
	}

	if (mysql_num_fields(&res) < 12)
	{
		LOG_PRINT(LOG_NORMAL, "Wrong Fields Num(should be 6), But Now It Is %u",mysql_num_fields(&res));
		m_db.FreeResult(&res);
		return 0;
	}

	while ((row = mysql_fetch_row(&res)))
	{
		//LOG_PRINT(LOG_NORMAL, "enter while\n");
		GscAnswer ans;
		//GscResponse *p = new GscResponse();
		Poem *p = new Poem();
		p->title = row[1];
		p->author->name = row[2];
		p->author->cv_info = row[12];
		p->author->authority = atof(row[13]);
		p->dynasty = row[3];
		p->content = row[4];
		p->translation = row[5];
		p->appreciation = row[6];
		p->style = row[7];
		p->rhesis = row[9];
		p->rhesis_tag = row[10];
		p->popularity = atof(row[11]);
		ans.point = atof(row[8]) / 10.0;
		ans.poem = p;
		//ans.poem.CopyFrom(gsc_response);
		ans_vec.push_back(ans);
		//LOG_PRINT(LOG_NORMAL, "exit while\n");
	}

//	LOG_PRINT(LOG_NORMAL, "end of select poem\n");
	
	//m_db.Close();
	m_db.FreeResult(&res);
	return 1;
}

int CQAServ::RandNGamePoem(int num, int complexity, vector<GscAnswer> &ans_vec)
{
	MYSQL_RES res;
	MYSQL_ROW row;
	int ret = 0;
	int count = 0;
	string sql = "select * from tb_game_poem";
	if (complexity > 0 && complexity < 4)
	{
		sql += " where complexity = ";
		sql += (char)('0' + complexity);
	}
	
	LOG_PRINT(LOG_NORMAL, "num:%d, complexity:%d\n", num, complexity);
	ret = m_db.SelectData(sql, res);
	LOG_PRINT(LOG_NORMAL, "sql:%s\n", sql.c_str());
	if (!ret) 
	{
		LOG_PRINT(LOG_NORMAL, "select failed, sql:%s\n", sql.c_str());
		return ret;
	}

	if (m_db.GetFieldCount(&res) < 5)
	{
		LOG_PRINT(LOG_NORMAL, "Wrong Fields Num(should be 5), But Now It Is %u",mysql_num_fields(&res));
		m_db.FreeResult(&res);
		return 0;
	}

	count = m_db.GetRowCount(&res);
	if (num < count)
	{
		srand(time(NULL));
		int i = 0;
		vector<int> used_index_vec;
		for (; i < num; i ++)
		{
			GscAnswer ans;
			int index = rand() % count;
			int times = 0;
			while (find(used_index_vec.begin(), used_index_vec.end(), index) !=
				   used_index_vec.end() && times < num)
			{
				index = (index + 1) % count;
				times ++;
			}

			mysql_data_seek(&res, index);
			row = mysql_fetch_row(&res);
			ans.poem = new Poem();

			ans.poem->complexity = atoi(row[1]);
			ans.poem->title = row[2];
			ans.poem->author->name = row[3];
			ans.poem->content = row[4];
			//StringTrim(&ans.poem->content, "\r");
			//StringTrim(&ans.poem->content, "\n");
			ans.point = 1.0;
			ans.answer = ans.poem->content;
			LOG_PRINT(LOG_NORMAL, "index:%d,content:%s\n", index, ans.poem->content.c_str());
			ans_vec.push_back(ans);
		}
	}
	else
	{
		while ((row = mysql_fetch_row(&res)))
		{
			GscAnswer ans;
			ans.poem = new Poem();

			ans.poem->complexity = atoi(row[1]);
			ans.poem->title = row[2];
			ans.poem->author->name = row[3];
			ans.poem->content = row[4];
			//StringTrim(&ans.poem->content, "\r");
			//StringTrim(&ans.poem->content, "\n");
			ans.point = 1.0;
			LOG_PRINT(LOG_NORMAL, "content:%s\n", ans.poem->content.c_str());
			ans_vec.push_back(ans);
		}
	}

	return 1;
}

int CQAServ::InitLog()
{
    LOG_PRINT(LOG_NORMAL, "init log\n");
    int log_level = tfc::base::from_str<int>((*m_mcd_config)["root\\log\\log_level"]);
    int log_type = tfc::base::from_str<int>((*m_mcd_config)["root\\log\\log_type"]);
    string path = (*m_mcd_config)["root\\log\\path"];
    string name_prefix =(*m_mcd_config)["root\\log\\name_prefix"];
    int max_file_size = tfc::base::from_str<int>((*m_mcd_config)["root\\log\\max_file_size"]);
    int max_file_no = tfc::base::from_str<int>((*m_mcd_config)["root\\log\\max_file_no"]);
    
    int ret = 0;
    ret = DEBUG_OPEN(log_level, log_type, path, 
                     name_prefix,max_file_size, max_file_no);
    if(ret)
    {
        LOG_PRINT(LOG_NORMAL, "init log error\n");
        exit(1);
    }
   	LOG_PRINT(LOG_NORMAL, "init log ok\n");
    return 0;
}

int CQAServ::Init(const string & conf_file)
{
	m_mcd_config.reset(new CFileConfig());
	m_mcd_config->Init(conf_file);

	InitLog();
	m_allow_regex = from_str<int>((*m_mcd_config)["root\\allow_regex"]);	
	m_mq_ccd_2_mcd = _mqs["mq_ccd_2_mcd"];
	m_mq_mcd_2_ccd = _mqs["mq_mcd_2_ccd"];

	std::string str_name_prefix = (*m_mcd_config)["root\\stat\\name_prefix"];
	int max_file_size = tfc::base::from_str<int>((*m_mcd_config)["root\\stat\\max_file_size"]);
	int max_file_index =tfc::base::from_str<int>((*m_mcd_config)["root\\stat\\max_file_index"]);
	int timeout_1 = tfc::base::from_str<int>((*m_mcd_config)["root\\stat\\timeout_1"]);
	int timeout_2 = tfc::base::from_str<int>((*m_mcd_config)["root\\stat\\timeout_2"]);
	int timeout_3 = tfc::base::from_str<int>((*m_mcd_config)["root\\stat\\timeout_3"]);

	m_qa_stat->Init(str_name_prefix, max_file_size, max_file_index, 
					timeout_1, timeout_2, timeout_3);

	m_db.Connect("10.198.30.113", "root", "root113", "qa_test", 3335);
}

void CQAServ::Run(const string & conf_file)
{
	//TODO: load configure file
	//No configure file at this time
	Init(conf_file);
	m_qa = QuestionAnalyzer::GetInstance();
	if (m_allow_regex)
	{
		m_template_file = (*m_mcd_config)["root\\template_file"];
		LoadTemplates(m_template_file);
		Function<void ()> check_regex_func = Bind(LoadTemplates, m_template_file);
		FileChangeNotifier::Subscribe(m_template_file, check_regex_func, 60);
	}
	string nlp_pattern_file = (*m_mcd_config)["root\\nlp_pattern_file"];
	string nlp_type_file = (*m_mcd_config)["root\\nlp_type_file"];
	string nlp_target_file = (*m_mcd_config)["root\\nlp_target_file"];
	m_nlp_ptn = new PatternCmpnt(nlp_pattern_file, nlp_type_file, nlp_target_file);
	m_nlp_ptn->init();

	Function<void ()> check_nlp_func = Bind(ReloadNlpPattern, m_nlp_ptn);
	FileChangeNotifier::Subscribe(nlp_pattern_file, check_nlp_func, 60);

	m_longconn_pack = new xfs::base::CBaseProtocolPack();
	m_longconn_unpack = new xfs::base::CBaseProtocolUnpack();
	LOG_PRINT(LOG_NORMAL, "CQAServ Has Been Launched\n");
	if (-1 == add_mq_2_epoll(m_mq_ccd_2_mcd, QADispCCD, this))
		LOG_PRINT(LOG_ERROR, "Add input mq m_mq_ccd_2_mcd to EPOLL fail!\n");

	while (!stop)
	{
		run_epoll_4_mq();
		CheckFlags();
	}
}

int CQAServ::DecodeQuery(char *rbuf, int buf_len, RootServerRequest &request)
{
	m_longconn_unpack->UntachPackage();
	m_longconn_unpack->AttachPackage((unsigned char*)(rbuf), buf_len);
	m_service_type = m_longconn_unpack->GetServiceType();
	m_seq_no = m_longconn_unpack->GetSeq();
	if (!m_longconn_unpack->Unpack())
	{
		LOG_PRINT(LOG_NORMAL, "Unpack Failed,buf_len is %d\n", buf_len);
		return 0;
	}

	uint16_t key = 0;
	char * content;
	uint32_t content_len;
	if (!m_longconn_unpack->GetVal(key, &content, &content_len))
	{
		LOG_PRINT(LOG_NORMAL, "GetVal Failed\n");
		return 0;
	}

	string data(content, content_len);
	/*
	string output_str;
	if (!ConvertUtf8ToGbk(data, &output_str))
	{
		LOG_PRINT(LOG_NORMAL, "Convert Utf8 To Gbk Failed\n");
		return 0;
	}
	*/
	if(!request.ParseFromString(data))
	{
		LOG_PRINT(LOG_NORMAL, "ParseFromString Failed\n");
		return 0;
	}
	return 1;
}

int CQAServ::FindRelativeContent(string context, string target, GscAnswer &ans)
{
	string poem_content(ans.poem->content);
	vector<string> sen_vec;
	string symbol_arr[] = {"。","！","？","；"};
	int s, i, ret;
	int symbol_size = 4;
	i = 0;
	for(; i < symbol_size; i ++) 
	{
		ReplaceAll(&poem_content, symbol_arr[i], "，");
	}
	SplitStringByDelimiter(poem_content, "，", &sen_vec);
	s = sen_vec.size();
	i = 0;
	ret = 0;
	LOG_PRINT(LOG_NORMAL, "sec_vec.size is %d,context is %s, target is %s\n", 
				s, context.c_str(), target.c_str());
	for (; i < s; i ++)
	{
		LOG_PRINT(LOG_NORMAL, "sen_vec[%d] is %s\n", i, sen_vec[i].c_str());
		if (!context.compare(sen_vec[i]))
		{
			if (i < s - 1 && !target.compare(TARGET_CONTENT_N))
			{
				ans.answer = sen_vec[i + 1];
				ret = 1;
			}
			else if (i > 0 && !target.compare(TARGET_CONTENT_L))
			{
				ans.answer = sen_vec[i - 1];
				ret = 1;
			}
			else
			{
				ans.answer = "";
				if (ans.poem)
					ans.poem->feedback = "no_next_content";
				ret = 0;
			}
		}
	}
	
	return ret;
}

int CQAServ::GuessTarget(string query, string &target)
{
	if (!query.size())
		return 0;
	if (query.find("作者") != std::string::npos)
	{
		target = TARGET_AUTHOR;
		return 1;
	}

	if (query.find("标题") != std::string::npos)
	{
		target = TARGET_TITLE;
		return 1;
	}
	if (query.find("朝代") != std::string::npos ||
		query.find("年代") != std::string::npos)
	{
		target = TARGET_DYNASTY;
		return 1;
	}
	if (query.find("内容") != std::string::npos)
	{
		target = TARGET_CONTENT;
		return 1;
	}
	if (query.find("注释") != std::string::npos ||
		query.find("翻译") != std::string::npos)
	{
		target = TARGET_TRANSLATION;
		return 1;
	}
	if (query.find("风格") != std::string::npos)
	{
		target = TARGET_STYLE;
		return 1;
	}

	target = TARGET_CONTENT;
	LOG_PRINT(LOG_NORMAL, "Guess Result:%s\n", target.c_str());
	return 0;
}

void CQAServ::UpdateAnsByTarget(GscAnswer &ans, string target, string context)
{
	//string converted_target;
	//if (ConvertUtf8ToGbk(target, &converted_target))
	//	converted_target = target;
	if (!ans.poem)
	{
		LOG_PRINT(LOG_ERROR, "ans.poem is NULL\n");
		return;
	}
	
	//LOG_PRINT(LOG_NORMAL, "target is %s, poem title is %s, author is %s\n", 
	//	target.c_str(), ans.poem->title.c_str(), ans.poem->author.c_str());
	
	if (!target.compare(TARGET_TITLE))
		ans.answer = ans.poem->title;
	else if (!target.compare(TARGET_AUTHOR))
		ans.answer = ans.poem->author->name;
	else if (!target.compare(TARGET_AUTHOR_INFO))
		ans.answer = ans.poem->author->cv_info;
	else if (!target.compare(TARGET_DYNASTY))
		ans.answer = ans.poem->dynasty;
	else if (!target.compare(TARGET_CONTENT))
		ans.answer = ans.poem->content;
	else if (!target.compare(TARGET_TRANSLATION))
		ans.answer = ans.poem->translation;
	else if (!target.compare(TARGET_APPRECIATION))
		ans.answer = ans.poem->appreciation;
	else if (!target.compare(TARGET_STYLE))
		ans.answer = ans.poem->style;
	else if (!target.compare(TARGET_CONTENT_N))
		FindRelativeContent(context, target, ans);
	else if (!target.compare(TARGET_CONTENT_L))
		FindRelativeContent(context, target, ans);
	else if (!target.compare(TARGET_RHESIS))
		ans.answer = ans.poem->rhesis;
	else
	{
		LOG_PRINT(LOG_NORMAL, "target %s is not defined target\n",
				target.c_str());
		ans.answer = "";
	}
	
	LOG_PRINT(LOG_NORMAL, "target is %s, ans.answer is %s\n",
			target.c_str(), ans.answer.c_str());
}

int CQAServ::TokenByRegex(string query,  vector<GscAnswer> &ans_vec)
{
	int ret = 0;
	vector<string> field_vec;
	string target;
	string context;
	//string converted_query;
    ReplaceAll(&query, " ", ""); 
	/*	
	if (!ConvertGbkToUtf8(query, &converted_query))
	{
		LOG_PRINT(LOG_NORMAL, "ConvertGbkToUtf8 Failed\n");
		converted_query = query;
	}
	query = converted_query;
	*/
	{
		MutexLocker locker(m_regex_lock);
		ret = m_qa->Tokenizer(query, field_vec, target, context);
		if (!ret) return ret;
	}

	LOG_PRINT(LOG_NORMAL, "target is %s, context is %s\n", target.c_str(), context.c_str());
    if(!target.compare(TARGET_CONTENT_N) || !target.compare(TARGET_CONTENT_L))
    {
		string sql = basic_select_string + " and content LIKE \"%" 
				   	 + context + "%\"";
		ret = SelectPoem(sql, ans_vec);
		if (ret)
		{
			vector<GscAnswer>::iterator it = ans_vec.begin();
			for (; it != ans_vec.end(); it ++)
			{
				ret = FindRelativeContent(context, target, *it);
				struct result fv;
				fv.tag = "content";
				fv.value = context;
				it->poem->fv_vec.push_back(fv);
			}
		}		
		return 1;
    }
    if(field_vec.size()>0)
   	{ 
		vector<GscAnswer>::iterator it;
    	for(int i=0;i<field_vec.size();++i)
    	{
    		string condition;
			string link_prefix;
			if (!field_vec[i].compare("title"))
				//!field_vec[i].compare("author"))
				//link_prefix = " = ";
				condition = field_vec[i] + " = \"" + context + "\"";
			else if (!field_vec[i].compare("author"))
				condition = "tb_author_info.name = \"" + context + "\"";
			else if (!field_vec[i].compare("dynasty"))
				condition = "tb_poem.dynasty like \"%" + context + "%\"";
			else
				//link_prefix = " like ";
				condition = field_vec[i] + " like \"%" + context + "%\"";
        	//condition = field_vec[i]+ link_prefix + "\"%"+context+"%\"";
			string sql = basic_select_string + " and " + condition;
			ret = SelectTarget(sql, ans_vec, field_vec[i], context);
		}
		if (!ans_vec.size()) return 0;
	
		sort(ans_vec.begin(), ans_vec.end(), compare);
		if (ans_vec.size() > MAX_RESULT_NUM)
			ans_vec.erase(ans_vec.begin()+MAX_RESULT_NUM, ans_vec.end());
		it = ans_vec.begin();
		for (; it != ans_vec.end(); it ++)
		{
			UpdateAnsByTarget(*it, target, context);
		}
    }
    else
    {
		return 0;
    }
	if (ans_vec.size() != 0)
		ret = 1;

	return ret;
}

int CQAServ::TokenBySpace(string query, vector<GscAnswer> &ans_vec)
{
	vector<string> qa_vec;
	vector<int> pids;
	string target;

	SplitStringByDelimiter(query, " ", &qa_vec);
	int ret = 0;
	int q_size = qa_vec.size();
	if (q_size == 0)
		return 0;
	else if (q_size == 1)
	{
		// 禁用倒排分词
		if (!m_allow_token) return 0;

		ret = m_qa->SplitAndCheck(query, pids);
		if (!ret) return 0;
		if (pids.size() == 0)
			return 0;
		string sql = basic_select_string + " and pid IN (";
		vector<int>::iterator it = pids.begin();
		for (; it != pids.end(); it ++)
		{
			char tmp[8];
			sprintf(tmp, "%d", *it);
			string tmp_str(tmp);
			sql += (const char *)tmp;
			if ((it + 1) != pids.end())
				sql += ",";
		}
		sql += ")";
		ret = SelectPoem(sql, ans_vec);
		if (!ret) return 0;
		GuessTarget(query, target);
		vector<GscAnswer>::iterator ans_it = ans_vec.begin();
		for (; ans_it != ans_vec.end(); ans_it ++)
		{
			UpdateAnsByTarget(*ans_it, target, "");
		}
	}
	else if (q_size == 2)
	{
        string sql = basic_select_string + " and (title =\""
				   + qa_vec[1] + "\" or content like \"%"
				   + qa_vec[1] + "%\") and tb_author_info.name=\"" 
				   + qa_vec[0] + "\" or (title =\"" 
				   + qa_vec[0] + "\" or content like \"%" 
				   + qa_vec[0] + "%\") and tb_author_info.name =\""
				   + qa_vec[1]+"\"";
		ret = SelectPoem(sql, ans_vec);
		if (!ret) return ret;
		vector<GscAnswer>::iterator it = ans_vec.begin();
		for (; it != ans_vec.end(); it ++)
		{
			UpdateAnsByTarget(*it, TARGET_CONTENT, "");
		}
	}
	else
		ret = 0;
	
	return ret;
}

int CQAServ::TokenByNlpTool(string query, vector<GscAnswer> &ans_vec)
{
	ResultVec rv;
	TargetVec tv;
	ReplaceAll(&query, " ", "");
	string query_gbk = ConvUtf8ToGbk(query);
	int ret = 0;
	{
		MutexLocker locker(m_nlp_lock);
		ret = m_nlp_ptn->match_query(query_gbk, rv, tv);
	}
	if (ret == -1 || !rv.size() || !tv.size())
	{
		LOG_PRINT(LOG_NORMAL, "match query failed, ret is %d, \
				  rv.size() is %d, tv.size() is %d\n", 
				  ret, rv.size(), tv.size());
		return 0;
	}

	LOG_PRINT(LOG_NORMAL, "match query success, ret is %d, \
			  rv.size() is %d, tv.size() is %d\n", 
			  ret, rv.size(), tv.size());

	int target_size = 1;//tv.size();
	int i = 0;
	for (; i < target_size; i ++)
	{
		string target = ConvGbkToUtf8(tv[0]);
		int j = 0;
		int result_size = rv.size();
		string condition = "";
		for (; j < result_size; j ++)
		{
			string link;
			string like_fix;
			string c_tag = ConvGbkToUtf8(rv[j].tag);
			if (!c_tag.compare(TARGET_CONTENT) ||
				!c_tag.compare(TARGET_TRANSLATION) ||
				!c_tag.compare(TARGET_APPRECIATION) ||
				/*!c_tag.compare(TARGET_TITLE) ||*/
				!c_tag.compare(TARGET_STYLE) ||
				!c_tag.compare(TARGET_DYNASTY))
			{
				like_fix = "%";
				link = " like ";
			}
			else
			{
				like_fix = "";
				link = " = ";
			}
			if (!c_tag.compare(TARGET_DYNASTY))
				c_tag = "tb_poem.dynasty";
			else if (!c_tag.compare(TARGET_AUTHOR))
				c_tag = "tb_author_info.name";
			condition += c_tag + link + "\"" + like_fix
					   + ConvGbkToUtf8(rv[j].value) + like_fix + "\" ";
			if ((j + 1) != result_size)
				condition += "and ";
		}

		string sql = basic_select_string + " and " + condition;
		int ret = SelectPoem(sql, ans_vec);

		if (!ans_vec.size()) continue;
		
		vector<GscAnswer>::iterator it = ans_vec.begin();
		for (; it != ans_vec.end(); it ++)
		{
			UpdateAnsByTarget(*it, target, ConvGbkToUtf8(rv[0].value));
			it->poem->fv_vec = rv;
			/*
			if (it->poem)
			{
				delete it->poem;
				it->poem = NULL;
			}
			*/
		}
	}

	if (ans_vec.size()) return 1;

	return 0;
}

int CQAServ::HandlePoemGameRequest(string query, vector<GscAnswer> &ans_vec)
{
	if (strstr(query.c_str(), "option=getpoem"))
	{
		vector<string> kv_vec;
		vector<string>::iterator it;
		int num = 0;
		int complexity = 0;
		SplitStringByDelimiter(query, "&", &kv_vec);
		it = kv_vec.begin();
		for (; it != kv_vec.end(); it ++)
		{
			vector<string> v;
			SplitStringByDelimiter(*it, "=", &v);
			if (!v[0].compare("num") && v.size() >= 2)
				num = atoi(v[1].c_str());
			else if (!v[0].compare("difficulty") && v.size() >= 2)
				complexity = atoi(v[1].c_str());
		}

		if (num <= 0) return 1;

		RandNGamePoem(num, complexity, ans_vec);

		return 1;
	}

	return 0;
}

int CQAServ::DoQuery(string query, vector<GscAnswer> &ans_vec)
{
	int ret = 0;
	//vector<GscAnswer> ans_vec;
	string query_utf8;
	if (query.length() == 0)
	{
		LOG_PRINT(LOG_NORMAL, "query string is null\n");
		return ret;
	}
	query_utf8 = ConvGbkToUtf8(query);
	StringTrim(&query_utf8, " ");
	LOG_PRINT(LOG_NORMAL, "query is %s\n", query_utf8.c_str());
	//query_utf8 = query;
	//处理诗词游戏的请求
	if (HandlePoemGameRequest(query, ans_vec))
		return 1;

	if (m_allow_regex)
	{
		ret = TokenByRegex(query_utf8, ans_vec);
	}

	if (ret) return 1;

	ret = TokenByNlpTool(query_utf8, ans_vec);
	if (!ret)
	{
		ret = TokenBySpace(query_utf8, ans_vec);
	}
	return ret;
}

void CQAServ::ConvertPoemToPb(Poem *p, GscResponse *resp)
{
	if (p == NULL)
		return;
	//p->print();
	ResultVecIter it;
	GscAuthor gsc_author;
	resp->set_title(ConvUtf8ToGbk(p->title));
	gsc_author.set_name(ConvUtf8ToGbk(p->author->name));
	gsc_author.set_authority(p->author->authority);
	gsc_author.set_cv_info(ConvUtf8ToGbk(p->author->cv_info));
	resp->mutable_author()->CopyFrom(gsc_author);
//	resp->author.CopyFrom(gsc_author);
	resp->set_dynasty(ConvUtf8ToGbk(p->dynasty));
	resp->set_content(ConvUtf8ToGbk(p->content));
	resp->set_annotation(ConvUtf8ToGbk(p->translation));
	resp->set_appreciation(ConvUtf8ToGbk(p->appreciation));
	resp->set_style(ConvUtf8ToGbk(p->style));
	resp->set_complexity(p->complexity);
	resp->set_popularity(p->popularity);
	for (it = p->fv_vec.begin(); it != p->fv_vec.end(); it ++)
	{
		GscFieldValue *gsc_fv = resp->add_match_info();
		gsc_fv->set_field(it->tag);
		gsc_fv->set_value(it->value);
	}
	resp->set_feedback(ConvUtf8ToGbk(p->feedback));
	//LOG_PRINT(LOG_NORMAL, "p->content:%s, resp->content:%s\n",
	//	p->content.c_str(), ConvGbkToUtf8(resp->content()).c_str());
}

char *CQAServ::EncodeResult(vector<GscAnswer> &ans_vec, 
	  RootServerRequest request, uint32_t & pack_len)
{
	char *pack = NULL;
	RootServerResponse response;
	AnswerItem *answer;
	AnswerSourceMask *ans_source_mask;
	string response_str;
	uint64_t i = 0;
	response.set_ret_code(0);
	response.set_bid(request.bid());
	vector<GscAnswer>::iterator it = ans_vec.begin();
	for (; it != ans_vec.end(); it ++)
	{
		if (!it->poem || 
			(it->answer.empty() && 
			it->poem->feedback.empty()))
			continue;
		string serialized_ans;
		GscResponse resp;
		LOG_PRINT(LOG_NORMAL, "answer is %s, point is %f\n", 
				it->answer.c_str(), it->point);
		answer = response.add_result_list();
		if (it->poem)
		{
			ConvertPoemToPb(it->poem, &resp);
			//DebugPB(&resp);
			if (!resp.SerializeToString(&serialized_ans))
			{
				LOG_PRINT(LOG_NORMAL, "Response Serialize Failed\n");
				serialized_ans = "";
			}
			else
			{
				/*
				GscResponse r;
				r.ParseFromString(serialized_ans);
				LOG_PRINT(LOG_NORMAL, "r.content:%s\n", ConvGbkToUtf8(r.content()).c_str());
				*/
				answer->set_merge_info(serialized_ans);
			}
			
		}
		answer->set_ans_type(DQA_POETRY);
		answer->set_ans_id(i ++);
		answer->set_ans_text(ConvUtf8ToGbk(it->answer));
		answer->set_ans_weight(it->point);
		//answer.question_idx = 
		ans_source_mask = answer->add_ans_source_mask();
		ans_source_mask->set_question_idx(0);
		ans_source_mask->set_source_idx(0);
		
		if (it->poem)
		{
			delete it->poem;
			it->poem = NULL;
		}
	}

	LOG_PRINT(LOG_NORMAL, "result size is %u\n", 
				response.result_list_size());

	if (!response.SerializeToString(&response_str))
	{
		LOG_PRINT(LOG_NORMAL, "Response Serialize Failed\n");
		return NULL;
	}

	m_longconn_pack->Init();
	m_longconn_pack->ResetContent();
	m_longconn_pack->SetServiceType(m_service_type);
	m_longconn_pack->SetSeq(m_seq_no);
	m_longconn_pack->SetKey((uint16_t)0,
							reinterpret_cast<uint8_t*>(&(*(response_str.begin()))),
							response_str.size());
	m_longconn_pack->GetPackage((unsigned char **)&pack, &pack_len);
	LOG_PRINT(LOG_NORMAL, "Response Str Is %d, pack_len is %d\n", 
			response_str.size(), pack_len);
	/*
	i = 0;
	const char *response_chars = response_str.c_str();
	for (i; i < response_str.size(); i ++)
	{
		printf("%d ", response_chars[i]);
	}
	printf("\n");
	*/
	return pack;
}

int CQAServ::HandleCcdRequest()
{
	int ret = 0;
	unsigned data_len, i = 0;
	unsigned long long cur_flow;
	uint32_t pack_len = 0;
	unsigned ccd_header_len = sizeof(TCCDHeader);
	struct timeval start_tv, end_tv;
	TCCDHeader *ccd_header = (TCCDHeader*) m_recv_buf;
	vector<GscAnswer> ans_vec;
	RootServerRequest request;
	try{
		do {
			string query;
			ans_vec.clear();

			if (++i > 1000) break;
			ret = m_mq_ccd_2_mcd->try_dequeue(m_recv_buf, sizeof(m_recv_buf),
									data_len, cur_flow);
			if (ret < 0)
				LOG_PRINT(LOG_ERROR, "try_dequeue m_mq_ccd_2_mcd failed!\n");

			if (data_len <= 0)
				continue;

			if (data_len < ccd_header_len)
			{
				LOG_PRINT(LOG_NORMAL, "invalid pkt\n");
				continue;
			}
			LOG_PRINT(LOG_NORMAL, "------------new request pkt--------------\n");
			
			m_qa_stat->AddStatWithNoTime("Request", 0);
			gettimeofday(&start_tv, NULL);
			if (ccd_header->_type != ccd_rsp_data) {
				LOG_PRINT(LOG_NORMAL, "Invalid CCD Header Type\n");
				continue;
			}
			LOG_PRINT(LOG_NORMAL, "data_len:%u, ccd_header_len:%u\n",
					data_len, ccd_header_len);
			if (!DecodeQuery(m_recv_buf + ccd_header_len, 
						data_len - ccd_header_len, request))
			{
				LOG_PRINT(LOG_NORMAL, "Decode Query Failed\n");
				continue;
			}

			ret = DoQuery(request.question(), ans_vec);
			//if (ret)
			//{
			char *data = EncodeResult(ans_vec, request, pack_len);
				/*
				int i = 0;
				for (; i < pack_len; i ++)
					printf("%d ", data[i]);
				printf("\n");
				*/
			if (Enqueue2Ccd(data, pack_len, cur_flow))
			{
				LOG_PRINT(LOG_NORMAL, "Failed To Enqueue Data To CCD\n");
				m_qa_stat->AddStatWithNoTime("Enqueue2Ccd", -1);
			}
			//}
			gettimeofday(&end_tv, NULL);
			m_qa_stat->AddStatWithNoTime("Enqueue2Ccd", 0);
			m_qa_stat->AddStatWithNoTime("QueryMatched", ret ? 0 : -1);
			m_qa_stat->AddStat("ProcessTime", 0, start_tv, end_tv);
			long time_gap = (end_tv.tv_sec - start_tv.tv_sec)*1000 + 
							(end_tv.tv_usec - start_tv.tv_usec)%1000;
			LOG_PRINT(LOG_NORMAL, "time used is %lu ms\n",time_gap); 
		}
		while(!stop && !ret && data_len > 0);
	} catch (exception & ex) {
		LOG_PRINT(LOG_NORMAL, "Exception in dispatch CCD! exception:%s\n", ex.what());
	}
}

int CQAServ::Enqueue2Ccd(const char * data, int data_len,
		unsigned long long flow)
{
    TCCDHeader stCcdHeader;
    stCcdHeader._ip   = 0;
    stCcdHeader._port = 0;
    stCcdHeader._type = ccd_req_data;
    stCcdHeader._arg  = 0;

    memcpy(m_send_buf, &stCcdHeader, sizeof(TCCDHeader));
    memmove(m_send_buf + sizeof(TCCDHeader), data, data_len);

    int ret = -1;
    ret = m_mq_mcd_2_ccd->enqueue(m_send_buf, \
        sizeof(TCCDHeader)+data_len, flow);

    return 0;
}

void CQAServ::CheckFlags()
{
	return;
}

void QADispCCD(void * priv)
{
	CQAServ *qa_serv = (CQAServ*)priv;
	qa_serv->HandleCcdRequest();
}

void LoadTemplates(string t_file)
{
	unsigned rsize = 0;
	string buf;

	MutexLocker locker(m_regex_lock);
	File *file_obj = File::Open(t_file, "r");
	if (!file_obj)
	{
		LOG_PRINT(LOG_NORMAL, "Load Regex Template Failed: File %s Not Exsit!\n", 
					t_file.c_str());
		//return -1;
		//exit(1);
		return;
	}
	
	QuestionAnalyzer::GetInstance()->ClearPattern();
	while ((rsize = file_obj->ReadLine(&buf)) > 5)
	{
		vector<string> tmp_vec;
		vector<string> f_vec;
		Pattern p;
		SplitStringByDelimiter(buf, "$$", &tmp_vec);
		if (tmp_vec.size() != 3)
		{
			LOG_PRINT(LOG_NORMAL, "fields not matched, line is %s\n",buf.c_str());
			continue;
		}
		p.pattern = tmp_vec[0];
		p.target = tmp_vec[2];
		StringTrim(&(p.target));
		SplitStringByDelimiter(tmp_vec[1], ",", &f_vec);
		p.fields = f_vec;
		QuestionAnalyzer::GetInstance()->AddPattern(p);
	}
	LOG_PRINT(LOG_NORMAL, "Load Regex Template Success\n");
	file_obj->Close();
	delete file_obj;
}

void ReloadNlpPattern(PatternCmpnt *nlp_ptn)
{
	if (!nlp_ptn) return;

	MutexLocker locker(m_nlp_lock);
	if (!nlp_ptn->reload_dict())
		LOG_PRINT(LOG_NORMAL, "Reload NLP Pattern Success\n");
	else
		LOG_PRINT(LOG_NORMAL, "Reload NLP Pattern Failed\n");
}

extern "C"
{
	CacheProc* create_app()
	{
		return new CQAServ();
	}
}
