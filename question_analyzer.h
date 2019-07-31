#ifndef _QUESTION_ANALYZER_H_
#define _QUESTION_ANALYZER_H_

#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <vector>
#include <list>
#include <map>
#include "common/text/regex/regex.h"
#include "common/base/string/algorithm.h"
#include "common/base/stdext/hash_map.h"
//#include "qa.h"

using namespace std;
using namespace stdext;
/*
typedef enum e_type {
	TARGET_AUTHOR = 0x0001,
	TARGET_TITLE = 0x0002,
	TARGET_DYNASTY = 0x0004,
	TARGET_STYLE = 0x0008,
	TARGET_CONTENT = 0x0010,
	TARGET_CONTENT_N = 0x0020,
	TARGET_CONTENT_L = 0x0040,
	TARGET_ANNOTATION = 0x0080,
} TargetType;
*/
#define TARGET_AUTHOR 	"author"
#define TARGET_TITLE  	"title"
#define TARGET_DYNASTY 	"dynasty"
#define TARGET_STYLE 	"style"
#define TARGET_CONTENT 	"content"
#define TARGET_CONTENT_N 	"content_next"
#define TARGET_CONTENT_L 	"content_last"
#define TARGET_TRANSLATION 	"translation"
#define TARGET_APPRECIATION "appreciation"
#define TARGET_RHESIS 	"rhesis" //名句
#define TARGET_AUTHOR_INFO 	"author_info"

const uint8_t MAX_RESULT_NUM = 5;
static int m_allow_token = 0;

typedef struct s_pattern {
	string pattern;
	Regex *reg;
	//unsigned ftype;
	vector<string> fields;
	string target;
	//unsigned ttype;
} Pattern;

class QuestionAnalyzer {
	public:
		static QuestionAnalyzer * GetInstance();
		int Tokenizer(string question, vector<string> & fields, 
				string & target, string & context);
		int CheckPid(string f1, string f2,
				vector<int> &match_pid, int &match_point);
		int SplitAndCheck(string qu, vector<int> &pids);

		void AddPattern(Pattern pattern);
		void ClearPattern();
		void TranslateTarget(unsigned ttype, std::vector<string> &vf);
	private:
		QuestionAnalyzer();
		~QuestionAnalyzer();
		int LoadRind();//Load Reverse Index
	private:
		static QuestionAnalyzer * instance;
		vector<Pattern> pattern_vec;
		hash_map<string, list<int> > rind_map;
};

#endif
