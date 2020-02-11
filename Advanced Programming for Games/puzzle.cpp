#include "puzzle.h"
#include "bigInt.h"
#include <iostream>
#include <time.h>
#include <vector>
#include <algorithm>
#include <fstream>
#include <string>
#include <functional>

using namespace std;


puzzle::puzzle(int size) :
	rowSize(size),
	boardSize(size* size)
{
	board = new int[boardSize];
	memset(board, 0, (boardSize * sizeof(int)));//creating the board on the heap and initialising all values to 0
}

puzzle::puzzle() :
	rowSize(4),
	boardSize(16)
{
	board = new int[boardSize];
	memset(board, 0, (boardSize * sizeof(int)));//creating the board on the heap and initialising all values to 0
}


puzzle::~puzzle() {
	delete board;
	board = NULL;
}


int* puzzle::getBoard() {
	return board;
}

int puzzle::getBoardSize() {
	return boardSize;
}
int puzzle::getRowSize() {
	return rowSize;
}

bigint factFunc(int num) {
	bigint factorial = 1;

	for (int i = 1; i < num + 1; i++) {
		factorial *= i;
	}
	return factorial;
}

int countContCombinations(puzzle puzz, int length) {
	int contCombinations=0;
	for (int i = 0; i < (puzz.getBoardSize() - length); i++) {
		if ((*(puzz.getBoard()+i + length - 1) - *(puzz.getBoard()+i)) == (length - 1))
			contCombinations++;
	}
	return contCombinations;
}

string puzzle::printBoard() {
	string formatBoard;

	for (int i = 0; i < boardSize - 1; i++) {
		formatBoard.append(to_string(board[i]) + "	");
		if ((i + 1) % rowSize == 0) {
			formatBoard.append("\n");
		}
	}
	formatBoard.append("\n");
	return formatBoard;
}

void puzzle::solvePartial(const int &partial) {
	int contCombinations = 0;
	int remainingTiles = boardSize - partial - 1;
	unsigned long long int possiblePositions = (rowSize - partial + 1) * (rowSize - 1) + rowSize - partial;
	bigint partialTotal;//this needs to change to allow larger numbers, use bigInt struct?

	ofstream mySolutionFile;
	mySolutionFile.open("Solution-File.txt", ios::app);

	sort(board, board + (boardSize - 1));
	contCombinations = countContCombinations(*this, partial);
	partialTotal = factFunc(remainingTiles)* contCombinations * possiblePositions * 2;

	mySolutionFile << partial << " = " << partialTotal << "\n";
	cout << partial <<" = "<<partialTotal << "\n";
	mySolutionFile.close();
}

void puzzle::solvePuzzle() {
	int contCombinations = 0;
	bigint contRows;

	if (rowSize == 2) {
		/* do this stuff */
	}
	else {
		ofstream mySolutionFile;
		mySolutionFile.open("Solution-File.txt", ios::app);
		mySolutionFile << printBoard();
		cout << printBoard();

		sort(board, board + (boardSize - 1));

		contCombinations = countContCombinations(*this, rowSize);

		contRows = factFunc(boardSize - rowSize - 1)*contCombinations * (rowSize - 1) / 2;

		mySolutionFile << "row = " << contRows << endl << "column = " << contRows << endl;
		mySolutionFile << "reverse row = " << contRows << endl << "reverse column = " << contRows << "\n";
		
		cout << "row = "<< contRows << endl << "column = " << contRows <<endl;
		cout << "reverse row = " << contRows << endl <<"reverse column = " << contRows << "\n";
		mySolutionFile.close();
	}
}

puzzle::puzzle(const puzzle& source)
{
	*this = source;
}


puzzle& puzzle::operator=(const puzzle& rhs) {
	if (this == &rhs) {
		return (*this);
	}

	//delete board;
	
	boardSize = rhs.boardSize;
	rowSize = rhs.rowSize;
	board = new int[boardSize];

	for (int i = 0; i < boardSize; i++) {
		board[i] = rhs.board[i];
	}
	return *this;
}

int partialCounter(puzzle puzzle1, const int &partial) {
	int rSize = puzzle1.getRowSize();
	int total;
	int partialCount = 0;
	for (int rowStartIndex = 0; rowStartIndex <= rSize*(rSize-1); rowStartIndex += rSize) {
		for (int j = rowStartIndex; j <= rowStartIndex + (rSize-partial); j++) {
			if (*(puzzle1.getBoard() + j + partial - 1) - *(puzzle1.getBoard() + j) == partial - 1)
				partialCount++;
		}
	}

	for (int rowStartIndex = 0; rowStartIndex <= rSize * (rSize - 1); rowStartIndex += rSize) {
		for (int j = rowStartIndex; j <= rowStartIndex + (rSize - partial); j++) {
			if (*(puzzle1.getBoard() + j)- *(puzzle1.getBoard() + j + partial - 1) == partial - 1)
				partialCount++;
		}
	}

	for (int j = 0; j < rSize; j++) {
		for (int colIndex = j; colIndex <= j + rSize * (rSize-partial); colIndex += rSize) {
			if (*(puzzle1.getBoard() + colIndex) - *(puzzle1.getBoard() + colIndex + rSize*(partial-1)) == partial - 1)
				partialCount++;
		}
	}
	for (int j = 0; j < rSize; j++) {
		for (int colIndex = j; colIndex <= j + rSize*(rSize-partial); colIndex += rSize) {
			if (*(puzzle1.getBoard() + colIndex + rSize * (partial - 1)) - *(puzzle1.getBoard() + colIndex) == partial - 1)
				partialCount++;
		}
	}
	return partialCount;
}


void puzzle::solveThisConfig() {
	ofstream mySolutionFile;
	mySolutionFile.open("Solution-File.txt", ios::app);

	mySolutionFile << "(total for row and column, including reverse, in this configuration)\n";
	cout << "(total for row and column, including reverse, in this configuration)\n";

	for (int partialNum = 2; partialNum <= rowSize; partialNum++) {
		mySolutionFile << partialNum << " = " << partialCounter(*this, partialNum) << endl;
		cout << partialNum << " = " << partialCounter(*this, partialNum) << endl;
	}
	mySolutionFile.close();
}
