#ifndef _POEM_H_
#define _POEM_H_

#include <iostream>
#include <string>
#include "qa_log.h"
#include "pattern_component.h"
#include "gsc.pb.h"
using namespace pb::dqa_gsc;
/*
typedef struct tag_fv {
	string field;
	string value;
}FV;
*/
typedef struct tag_author {
	string name;
	float authority;
	string cv_info;

	tag_author()
	{
		name = "";
		authority = 0.0;
		cv_info = "";
	}
} Author, *PAuthor;

typedef struct tag_poem {
	string title;
	PAuthor author;
	string dynasty;
	string content;
	string translation;
	string appreciation;
	string style;
	string rhesis;
	string rhesis_tag;
	int complexity;
	float popularity;
	ResultVec fv_vec;
	string feedback;
	tag_poem ()
	{
		title = "";
		author = new Author();
		dynasty = "";
		content = "";
		translation = "";
		appreciation = "";
		style = "";
		rhesis = "";
		rhesis_tag = "";
		complexity = 0;
		popularity = 0.0;
		fv_vec.clear();
	}

	~tag_poem ()
	{
		if (author)
		{
			delete author;
			author = NULL;
		}
	}

	void print()
	{
		LOG_PRINT(LOG_NORMAL, "title:%s\n", title.c_str());
		LOG_PRINT(LOG_NORMAL, "author:%s\n", author->name.c_str());
		LOG_PRINT(LOG_NORMAL, "dynasty:%s\n", dynasty.c_str());
		LOG_PRINT(LOG_NORMAL, "content:%s\n", content.c_str());
		LOG_PRINT(LOG_NORMAL, "translation:%s\n", translation.c_str());
		LOG_PRINT(LOG_NORMAL, "appreciation:%s\n", appreciation.c_str());
		LOG_PRINT(LOG_NORMAL, "style:%s\n", style.c_str());
		LOG_PRINT(LOG_NORMAL, "rhesis:%s\n", rhesis.c_str());
		LOG_PRINT(LOG_NORMAL, "rhesis_tag:%s\n", rhesis_tag.c_str());
		LOG_PRINT(LOG_NORMAL, "complexity:%d\n", complexity);
	}
} Poem, *PPoem;

typedef struct tag_gsc_answer {
	string answer;//直接答案
	float point;  //置信度
	Poem *poem;

	tag_gsc_answer ()
	{
		poem = NULL;
	};

	~tag_gsc_answer ()
	{
	};
} GscAnswer, *PGscAnswer;

bool compare(const GscAnswer &first, const GscAnswer &second)
{
	return first.point > second.point;
}


#endif
