#include "boggle.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#if defined(_WIN32)
#define FORCEINLINE __forceinline
#include <Windows.h>
#undef min
#else
#define FORCEINLINE inline __attribute__((always_inline))
#include <sys/mman.h>
#endif
// 500 mb buffer.
#define MEM_BUFFER_SIZE (500 * 1024 * 1024)
#define BOARD_AT(board, width, x, y) board[(y) * (width) + (x)]
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define UNUSED_VARIABLE(variable) ((void)&(variable))

static char* dictionary_buffer = NULL;
static void* mem_buffer_head = NULL;
static void* mem_buffer = NULL;
static void* mem_buffer_curr = NULL;
static uintptr_t mem_buffer_tail = 0;
// enough space for a big string.
static char temp_string[256]; 
static int temp_string_len = 0;
static int score_table[] = { 0, 0, 0, 1, 1, 2, 3, 5, 11 };

struct PrefixTree
{
	char value;
	const char* word;
	PrefixTree* children[26];
} trie;

FORCEINLINE static void* MemAlloc(size_t size)
{
	// This is just a simple bump pointer allocator.
	void* return_ptr = mem_buffer_curr;
	mem_buffer_curr = ((char*)mem_buffer_curr + size);
	return return_ptr;
}
FORCEINLINE static int PrefixTreeAddWord(PrefixTree* node, const char* word, int index = 0)
{
	int len = (int)strlen(word);
	PrefixTree* child = NULL;
	for (int i = 0; i < len; ++i)
	{
		// Force every word to be lower case
		char value = (char)tolower(word[i]);
		int child_index = value - 'a';
		PrefixTree** children = node->children;
		// If node doesn't exist, we create it.
		if ((child = children[child_index]) == NULL)
		{
			child = children[child_index] = (PrefixTree*)MemAlloc(sizeof(PrefixTree));
			child->value = value;
			child->word = NULL;
			memset(child->children, 0, sizeof(PrefixTree*) * 26);
		}
		node = child;
	}
	// Save dictionary word on child as a reference
	if (child != NULL)
		child->word = word;
	return len;
}
FORCEINLINE static PrefixTree* PrefixTreeHasPrefix(PrefixTree* node, const char* word, int word_len, int* qu_addition)
{
	for (int index = 0; index < word_len; ++index)
	{
		char letter = word[index];
		int letter_index = letter - 'a';
		if ((node = node->children[letter_index]) != NULL)
		{
			// Handling Q as QU
			if (letter_index == 16)
			{
				if ((node = node->children[20]) != NULL)
				{
					++(*qu_addition);
				}
				else
				{
					return NULL;
				}
			}
		}
		else
		{
			return NULL;
		}
	}
	return node;
}
static int ProbeForWords(Results* results, const char* board, int width, int height, char* visited_board, int x, int y, int score, const char*** word_buffer)
{
	int found_qu = 0;
	const char* word = NULL;
	PrefixTree* node = NULL;
	bool test_xlt, test_xgt, test_ylt, test_ygt;

	BOARD_AT(visited_board, width, x, y) = 1;

	// Append character to temporal string
	temp_string[temp_string_len++] = BOARD_AT(board, width, x, y);

	// Check if prefix can be found on the dictionary	
	// If not we stop searching.
	if (!(node = PrefixTreeHasPrefix(&trie, temp_string, temp_string_len, &found_qu)))
	{
		--temp_string_len;
		BOARD_AT(visited_board, width, x, y) = 0;
		return score;
	}

	// Check if we found a word.
	// If we do we add it to the found word array and add score
	word = node->word;
	if (word != NULL)
	{
		++results->Count;
		score += score_table[MIN(temp_string_len + found_qu, 8)];
		**word_buffer = word;
		*word_buffer = (const char**)MemAlloc(sizeof(void*));
	}


	test_xlt = x >= 1;
	test_xgt = x + 1 < width;
	test_ylt = y >= 1;
	test_ygt = y + 1 < height;

	// Recursively probe through each adjacent neighbour.
	if (test_xlt && test_ylt && BOARD_AT(visited_board, width, x - 1, y - 1) == 0)
		score = ProbeForWords(results, board, width, height, visited_board, x - 1, y - 1, score, word_buffer);
	if (test_ylt && BOARD_AT(visited_board, width, x, y - 1) == 0)
		score = ProbeForWords(results, board, width, height, visited_board, x, y - 1, score, word_buffer);
	if (test_xgt && test_ylt && BOARD_AT(visited_board, width, x + 1, y - 1) == 0)
		score = ProbeForWords(results, board, width, height, visited_board, x + 1, y - 1, score, word_buffer);
	if (test_xlt && BOARD_AT(visited_board, width, x - 1, y) == 0)
		score = ProbeForWords(results, board, width, height, visited_board, x - 1, y, score, word_buffer);
	if (test_xgt && BOARD_AT(visited_board, width, x + 1, y) == 0)
		score = ProbeForWords(results, board, width, height, visited_board, x + 1, y, score, word_buffer);
	if (test_xlt && test_ygt && BOARD_AT(visited_board, width, x - 1, y + 1) == 0)
		score = ProbeForWords(results, board, width, height, visited_board, x - 1, y + 1, score, word_buffer);
	if (test_ygt && BOARD_AT(visited_board, width, x, y + 1) == 0)
		score = ProbeForWords(results, board, width, height, visited_board, x, y + 1, score, word_buffer);
	if (test_xgt && test_ygt && BOARD_AT(visited_board, width, x + 1, y + 1) == 0)
		score = ProbeForWords(results, board, width, height, visited_board, x + 1, y + 1, score, word_buffer);

	--temp_string_len;
	BOARD_AT(visited_board, width, x, y) = 0;
	return score;
}

// Test Specific
void LoadDictionary(const char* path)
{
	if (dictionary_buffer == NULL)
	{
		FILE* file = NULL;
#if defined(_MSC_VER) && defined(_WIN32)
		fopen_s(&file, path, "r");
#else
		file = fopen(path, "r");
#endif
		if (file == NULL)
		{
			exit(-1);
			return;
		}
		{
#if defined(_WIN32)
			// Reserve memory buffer
			mem_buffer_head = VirtualAlloc(NULL, MEM_BUFFER_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else 
			// assume "other" is linux
			mem_buffer_head = mmap(NULL, MEM_BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
#endif
			mem_buffer = mem_buffer_head;
			mem_buffer_curr = mem_buffer;
			mem_buffer_tail = (uintptr_t)mem_buffer + MEM_BUFFER_SIZE;
		}
		fseek(file, 0, SEEK_END);
		long file_size = ftell(file);
		fseek(file, 0, SEEK_SET);
		dictionary_buffer = (char*)MemAlloc(file_size);
		size_t n = fread((void*)dictionary_buffer, file_size, 1, file);
		UNUSED_VARIABLE(n); // mingw complains.
		fclose(file);
		{
			trie.value = 0;
			trie.word = NULL;
			memset(trie.children, 0, sizeof(PrefixTree*) * 26);
			long index = 0;
			int word_count = 0;
			while (index < file_size)
			{
				long last_index = index;
				while (index < file_size && dictionary_buffer[index++] != '\n');
				dictionary_buffer[index - 1] = 0;
				PrefixTreeAddWord(&trie, &dictionary_buffer[last_index]);
				++word_count;
			}
		}
		// We'll use the rest of the space for generating words
		mem_buffer = mem_buffer_curr;
		memset(temp_string, 0, sizeof(temp_string));
	}
}
void FreeDictionary()
{
	if (dictionary_buffer != NULL)
	{
#if defined(_WIN32)
		VirtualFree(mem_buffer_head, MEM_BUFFER_SIZE, MEM_RELEASE);
#else
		munmap(mem_buffer_head, MEM_BUFFER_SIZE);
#endif
		dictionary_buffer = NULL;
		mem_buffer_head = NULL;
		mem_buffer = NULL;
		mem_buffer_curr = NULL;
		mem_buffer_tail = 0;
	}
}
Results FindWords(const char* board, unsigned width, unsigned height)
{
	Results results = {};
	char* visited_board = (char*)MemAlloc(width * height);
	const char** word_buffer = (const char**)MemAlloc(sizeof(void*));
	memset(visited_board, 0, width * height);
	results.Words = (const char* const*)word_buffer;
	// We force the board to be lower case.
	char* tboard = (char*)board;
	for (unsigned i = 0; i < (width * height); ++i)
	{
		tboard[i] = tolower(board[i]);
	}
	// We iterate through each cell and probe for words.
	for (int y = 0; y < (int)height; ++y)
	{
		for (int x = 0; x < (int)width; ++x)
		{
			results.Score = ProbeForWords(&results, board, width, height, visited_board, x, y, results.Score, &word_buffer);
		}
	}
	return results;
}
void FreeWords(Results results)
{
	mem_buffer_curr = mem_buffer;
}
