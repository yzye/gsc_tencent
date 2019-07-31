#ifndef _QA_MCD_PROC_H_
#define _QA_MCD_PROC_H_

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>
#include <iostream>

#include "tfc_cache_proc.h"
#include "tfc_net_ccd_define.h"
#include "tfc_base_config_file.h"
#include "tfc_base_str.h"
#include "tfc_base_http.h"
#include "myalloc.h"
#include "qa_log.h"
#include "question_analyzer.h"
#include "common/base/scoped_ptr.h"
#include "gsc.pb.h"
#include "dqa_rootmerge.pb.h"
#include "poem.h"
#include "longconn/protocol/base_protocol.h"
#include "comm_db.h"
#include "pattern_component.h"
#include "common/system/concurrency/mutex.h"
#include "qa_stat.h"

using namespace std;
using namespace tfc::cache;
using namespace tfc::base;
using namespace pb::dqa_rootmerge;
class PatternCmpnt;

static const unsigned kDataBufMaxSize = (128<<20);

extern void QADispCCD(void * priv);
static Mutex m_regex_lock;
static Mutex m_nlp_lock;
static void LoadTemplates(string tfile);
static void ReloadNlpPattern(PatternCmpnt *nlp_ptn);

class CQAServ : public CacheProc
{
public:
	CQAServ();
	~CQAServ(){};
	int SelectTarget(string sql, vector<GscAnswer> &ans_vec,
					string field, string context);
	int SelectPoem(string sql, vector<GscAnswer> &ans_vec);
	int RandNGamePoem(int num, int complexity, vector<GscAnswer> &ans_vec);
	int InitLog();
	int Init(const string & conf_file);
	virtual void run(const std::string & conf_file) { Run(conf_file); }
	// 统一风格(首字母大写)
	virtual void Run(const std::string & conf_file);
	int DecodeQuery(char *rbuf, int buf_len, RootServerRequest &request);
	int FindRelativeContent(string context, string target, GscAnswer &ans);
	void UpdateAnsByTarget(GscAnswer &ans, string target, string context);
	int TokenByRegex(string query,  vector<GscAnswer> &ans_vec);
	int TokenBySpace(string query, vector<GscAnswer> &ans_vec);
	int TokenByNlpTool(string query, vector<GscAnswer> &ans_vec);
	int HandlePoemGameRequest(string query, vector<GscAnswer> &ans_vec);
	int DoQuery(string query, vector<GscAnswer> &ans_vec);
	void ConvertPoemToPb(Poem *p, GscResponse *resp);
	char *EncodeResult(vector<GscAnswer> &ans_vec, 
						RootServerRequest request, uint32_t & pack_len);
	int HandleCcdRequest();
	int Enqueue2Ccd(const char * data, int data_len, 
					unsigned long long flow);
	void CheckFlags();
private:
	int CountWeight(GscAnswer *ans, ResultVec rv,
		MYSQL_ROW row, MYSQL_FIELD *fields, unsigned fields_num);
	string ConvUtf8ToGbk(string utf8_str);
	string ConvGbkToUtf8(string gbk_str);
	void DebugPB(GscResponse *p);
	int GuessTarget(string query, string &target);
private:
	QuestionAnalyzer    *m_qa;
	CFifoSyncMQ			*m_mq_ccd_2_mcd;
	CFifoSyncMQ			*m_mq_mcd_2_ccd;
	int 				m_allow_regex;
	TCCDHeader			*m_ccdheader;

	char m_recv_buf[kDataBufMaxSize];
	char m_send_buf[kDataBufMaxSize];
	scoped_ptr<CFileConfig> m_mcd_config;
	xfs::base::CBaseProtocolPack	*m_longconn_pack;
	xfs::base::CBaseProtocolUnpack	*m_longconn_unpack;
	uint16_t m_service_type;
	unsigned m_seq_no;
	CommDB m_db;
	PatternCmpnt *m_nlp_ptn;
	string m_template_file;
	CQAStat *m_qa_stat;
};

#endif
