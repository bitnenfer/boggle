#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "boggle.hpp"

#if defined(_WIN32) && defined(_MSC_VER)
#include <Windows.h>
#define LOG(fmt, ...) {\
	char log_buffer[128] = {};\
	snprintf(log_buffer, sizeof(log_buffer), fmt, ##__VA_ARGS__);\
	OutputDebugString(log_buffer);\
}
#else
#define LOG(fmt, ...) printf(fmt, ##__VA_ARGS__);
#endif
#define UNUSED_VARIABLE(variable) ((void)&(variable))
void test0()
{
	LoadDictionary("dictionary-zingarelli2005.txt");
	const char board[] = "rasvnlsdddesntew";
	Results res = FindWords(board, 4, 4);
	UNUSED_VARIABLE(res);
	LOG("word count = %u score = %u\n", res.Count, res.Score);
	/*for (unsigned i = 0; i < res.Count; ++i)
	{
		LOG("%s ", res.Words[i]);
	}*/
	FreeDictionary();
}

void test1()
{
	LOG("\n--------\nTest 1\n--------\n\n")
	LoadDictionary("dictionary.txt");
	// Randomly generated string.
	const char board[] = "ezbaxefjsxjkkgdltdnebcgrarnztgnszxgp";
	Results res = FindWords(board, 6, 6);
	LOG("word count = %u score = %u\n", res.Count, res.Score);
	// Too many words to print out.
	//for (unsigned i = 0; i < res.Count; ++i)
	//{
	//	LOG("found: %s\n", res.Words[i]);
	//}
	FreeDictionary();
}

int main(int argc, const char** argv)
{
	test0();
	//test1();
	//getchar();
	return 0;
}