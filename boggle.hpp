#pragma once

struct Results
{
	const char* const* Words;    // pointers to unique found words, each terminated by a non-alpha char
	unsigned           Count;    // number of words found
	unsigned           Score;    // total score
	void*              UserData; // ignored
};

// input dictionary is a file with one word per line
void LoadDictionary(const char* path); // << TODO
void FreeDictionary(); // << TODO

					   // `board` will be exactly `width` * `height` chars, and char 'q' represents the 'qu' Boggle cube
Results FindWords(const char* board, unsigned width, unsigned height); // << TODO
																	   // `results` is identical to what was returned from `FindWords`
void FreeWords(Results results); // << TODO