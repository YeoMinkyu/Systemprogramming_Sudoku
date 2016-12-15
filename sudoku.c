#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "sudoku.h"

bool is_valid_puzzle(char puzzle[STREAM_LENGTH])
{
	int rowEntry, colEntry, squareEntry;

	int rowEntryCounter[9] = {0};
	int colEntryCounter[9] = {0};
	int squareEntryCounter[9] = {0};

	for(int row=0; row<9; row++)
	{
		for (int col=0; col<9; col++)
		{
			rowEntry = puzzle[row * 9 + col];
			colEntry = puzzle[col * 9 + row];
			squareEntry = puzzle[(((row % 3) + (row / 3) * 9) * 3 ) + (col % 3 + (col / 3) * 9)];
			if (rowEntry != '.')
			{
				rowEntryCounter[rowEntry - 49]++;
			}
			if (colEntry!= '.')
			{
				colEntryCounter[colEntry - 49]++;
			}
			if (squareEntry != '.')
			{
				squareEntryCounter[squareEntry - 49]++;
			}
		}
		for (int i = 0; i < 9; i++)
		{
			if (rowEntryCounter[i] > 1 || colEntryCounter[i] > 1 || squareEntryCounter[i] > 1)
				return false;
			rowEntryCounter[i] = 0;
			colEntryCounter[i] = 0;
			squareEntryCounter[i] = 0;
		}
	}
	return true;
}

static bool is_available(char puzzle[STREAM_LENGTH], int row, int col, int num)
{
	int i;
	int rowStart = (row/3) * 3;
	int colStart = (col/3) * 3;

	for (i=0; i<9; i++)
	{
		if (puzzle[row * 9 + i] - 48 == num)
			return false;
		if (puzzle[i * 9 + col] - 48 == num)
			return false;
		if (puzzle[(rowStart + (i % 3)) * 9 + (colStart + (i / 3))] - 48 == num)
			return false;
	}
	return true;
}

static int solve_recursively(char puzzle[STREAM_LENGTH], int row, int col)
{
	if (row < 9 && col < 9)
	{
		if (puzzle[row * 9 + col] != '.')
		{
			if ((col + 1) < 9)
				return solve_recursively(puzzle, row, col + 1);
			else if ((row + 1) < 9)
				return solve_recursively(puzzle, row + 1, 0);
			else
				return 1;
		}
		else
		{
			for (int i = 0; i < 9; i++)
			{
				if(is_available(puzzle, row, col, i + 1))
				{
					puzzle[row * 9 + col] = i + 1 + 48;

					if(solve_recursively(puzzle, row, col))
						return 1;
					else
						puzzle[row * 9 + col] = '.';
				}
			}
		}
		return 0;
	}
	else
		return 1;
}

int solve(char puzzle[STREAM_LENGTH])
{
	if (is_valid_puzzle(puzzle))
		return solve_recursively(puzzle, 0, 0);
	else
		return 0;
}

static int rand_int(int n) {
	int rnd;
	int limit = RAND_MAX - RAND_MAX % n;

	do {
		rnd = rand();
	} while (rnd >= limit);
	return (rnd % n);
}

static void shuffle(char *array, int n) {
	int i, j;
	char tmp;

	for (i = n - 1; i > 0; i--)
	{
		j = rand_int(i + 1);
		tmp = array[j];
		array[j] = array[i];
		array[i] = tmp;
	}
}

static char* create_random_numbers()
{
	char numbers[10] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '\0'};
	shuffle(numbers, 9);
	return strdup(numbers);
}

static char* generate_seed()
{
	char *stream = (char*)malloc(STREAM_LENGTH);
	int index = 0;
	int iSquare = 0;

	srand(time(NULL));

	char* upperleft = create_random_numbers();
	char* center = create_random_numbers();
	char* lowerright = create_random_numbers();

	for (int i = 0; i < 3; i++)
	{
		for(int j = 0; j < 3; j++)
			stream[index++] = upperleft[iSquare++];
		for(int j = 0; j < 6 ; j++)
			stream[index++] = '.';
	}
	iSquare = 0;

	for (int i = 0; i < 3; i++)
	{
		for(int j = 0; j < 3 ; j++)
			stream[index++] = '.';
		for(int j = 0; j < 3; j++)
			stream[index++] = center[iSquare++];
		for(int j = 0; j < 3 ; j++)
			stream[index++] = '.';
	}
	iSquare = 0;
	
	for (int i = 0; i < 3; i++)
	{
		for(int j = 0; j < 6 ; j++)
			stream[index++] = '.';
		for(int j = 0; j < 3; j++)
			stream[index++] = upperleft[iSquare++];
	}

	stream[81] = '\0';

	free(upperleft);
	free(center);
	free(lowerright);

	return stream;
}

static void punch_holes(char *stream, int count)
{
	int i = 0;
	while(i < count)
	{
		int random = rand() % 80 + 1;
		if (stream[random] != '.')
		{
			stream[random] = '.';
			i++;
		}
	}
}

char* difficulty_to_str(DIFFICULTY level)
{
	switch(level)
	{
		case D_HARD:
			return "hard";
		case D_NORMAL:
			return "normal";
		case D_EASY:
		default:
			return "easy";
	}
}

int get_holes(DIFFICULTY level)
{
	int holes;
	switch (level)
	{
		case D_HARD:
			holes = 50;
			break;
		case D_NORMAL:
			holes = 40;
			break;
		case D_EASY:
		default:
			holes = 30;
			break;
	}
	return holes;
}

char* generate_puzzle(int holes)
{
	char* stream;

	stream = generate_seed();
	solve(stream);
	punch_holes(stream, holes);
	return stream;
}
