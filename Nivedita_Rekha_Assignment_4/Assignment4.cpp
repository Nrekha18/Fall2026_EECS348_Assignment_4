// ============================================================================
// Program:       EECS 348 Assignment 4 - Sudoku Solver
// Description:   Object-oriented Sudoku solver. It reads each puzzle file,
//                checks that the puzzle is legal, then uses recursive
//                depth-first search with backtracking to find EVERY solution.
// Inputs:        puzzle1.txt ... puzzle5.txt (digits 1-9 = given, _ = blank)
// Output:        For each file: the file name, the puzzle, then every solution,
//                or "No solution found".
// Collaborators: None
// Other sources: ChatGPT (base version of the code), Gemini (compared only),
//                Claude (Anthropic) helped with the improvements below.
// Author:        Nivedita Rekha
// Creation date: 10/15/2026
// ============================================================================

#include <iostream>   // cout / cerr for printing
#include <fstream>    // ifstream for reading the puzzle files
#include <string>     // std::string for file names
#include <cctype>     // isspace() for skipping whitespace in the files

// ---------------------------------------------------------------------------
// Class SudokuBoard
// Holds one 9x9 grid and remembers which digits are already used in every
// row, column, and 3x3 block (as bitmasks), so candidate checks are instant.
// Origin: structure from ChatGPT's Sudoku class; bitmasks added with Claude.
// ---------------------------------------------------------------------------
class SudokuBoard {
private:
    int cells[9][9];      // the grid: 0 = empty, 1-9 = a digit
    int rowUsed[9];       // bit d is set if digit d is already in that row
    int colUsed[9];       // bit d is set if digit d is already in that column
    int blockUsed[9];     // bit d is set if digit d is already in that block
    bool givensValid;     // false if the puzzle's given digits break a rule

    // Converts a (row, col) position into its block number 0-8.
    static int blockOf(int row, int col) {
        return (row / 3) * 3 + (col / 3);
    }

public:
    // Constructor: start with an empty board and no digits used.
    SudokuBoard() : givensValid(true) {
        for (int r = 0; r < 9; r++) {
            rowUsed[r] = colUsed[r] = blockUsed[r] = 0;   // nothing used yet
            for (int c = 0; c < 9; c++) {
                cells[r][c] = 0;                          // every cell empty
            }
        }
    }

    // Reads a puzzle file. Returns false if the file cannot be opened, has a
    // character other than 1-9, _ or whitespace, or lacks exactly 81 cells.
    // Origin: ChatGPT's readPuzzle; duplicate-given check added by author.
    bool readPuzzle(const std::string& filename) {
        std::ifstream inputFile(filename);                // open the file
        if (!inputFile.is_open()) {                       // could not open it
            std::cerr << "Error: Could not open " << filename << std::endl;
            return false;
        }

        int count = 0;                                    // cells read so far
        char ch;                                          // current character
        while (inputFile.get(ch)) {                       // read one char at a time
            if (std::isspace(static_cast<unsigned char>(ch))) {
                continue;                                 // skip spaces/newlines
            }
            if (ch != '_' && (ch < '1' || ch > '9')) {    // illegal character
                std::cerr << "Error: Invalid character in " << filename << std::endl;
                return false;
            }
            if (count >= 81) {                            // more than 81 cells
                std::cerr << "Error: " << filename
                          << " must contain exactly 81 cells." << std::endl;
                return false;
            }
            int row = count / 9;                          // row of this cell
            int col = count % 9;                          // column of this cell
            if (ch != '_') {                              // a given digit
                int digit = ch - '0';                     // char -> number
                cells[row][col] = digit;                  // keep it so it prints
                int bit = 1 << digit;                     // bit for this digit
                int b = blockOf(row, col);                // block of this cell
                if ((rowUsed[row] | colUsed[col] | blockUsed[b]) & bit) {
                    givensValid = false;                  // digit repeats: invalid
                }
                rowUsed[row] |= bit;                      // mark digit as used
                colUsed[col] |= bit;
                blockUsed[b] |= bit;
            }
            count++;                                      // next cell
        }

        if (count != 81) {                                // fewer than 81 cells
            std::cerr << "Error: " << filename
                      << " must contain exactly 81 cells." << std::endl;
            return false;
        }
        return true;
    }

    // True if the given digits in the file do not repeat in any row/col/block.
    bool hasValidGivens() const { return givensValid; }

    // Prints the board with separators between the 3x3 blocks.
    // Origin: ChatGPT's printBoard (unchanged in behavior).
    void print() const {
        for (int row = 0; row < 9; row++) {
            for (int col = 0; col < 9; col++) {
                if (cells[row][col] == 0) {
                    std::cout << '_';                     // blank cell
                } else {
                    std::cout << cells[row][col];         // digit
                }
                if (col == 2 || col == 5) {
                    std::cout << " | ";                   // vertical separator
                } else if (col < 8) {
                    std::cout << ' ';                     // space between cells
                }
            }
            std::cout << std::endl;
            if (row == 2 || row == 5) {
                std::cout << "------+-------+------" << std::endl;   // horizontal
            }
        }
    }

    // Candidate digits for a cell as a bitmask: bit d is 1 if digit d fits.
    // 0x3FE keeps only bits 1-9. Origin: replaces ChatGPT's getCandidates.
    int candidates(int row, int col) const {
        int used = rowUsed[row] | colUsed[col] | blockUsed[blockOf(row, col)];
        return ~used & 0x3FE;
    }

    // Puts a digit in an empty cell and marks it as used.
    void place(int row, int col, int digit) {
        int bit = 1 << digit;
        cells[row][col] = digit;
        rowUsed[row] |= bit;
        colUsed[col] |= bit;
        blockUsed[blockOf(row, col)] |= bit;
    }

    // Takes a digit back out of a cell (this is the "backtrack" step).
    void remove(int row, int col) {
        int bit = 1 << cells[row][col];
        cells[row][col] = 0;
        rowUsed[row] &= ~bit;
        colUsed[col] &= ~bit;
        blockUsed[blockOf(row, col)] &= ~bit;
    }

    // Finds the empty cell with the FEWEST candidates (fails fast, so the
    // search tries far fewer dead ends). Returns false if no empty cell is left.
    // Origin: author/Claude improvement (ChatGPT used the first empty cell).
    bool findBestCell(int& bestRow, int& bestCol, int& bestMask) const {
        int fewest = 10;                                  // more than any real count
        for (int r = 0; r < 9; r++) {
            for (int c = 0; c < 9; c++) {
                if (cells[r][c] != 0) continue;           // only empty cells
                int mask = candidates(r, c);              // digits that fit here
                int count = __builtin_popcount(mask);     // how many fit
                if (count < fewest) {                     // best so far
                    fewest = count;
                    bestRow = r;
                    bestCol = c;
                    bestMask = mask;
                }
            }
        }
        return fewest != 10;                              // false => board is full
    }
};

// ---------------------------------------------------------------------------
// Class SudokuSolver
// Runs the recursive depth-first search with backtracking on a board and
// prints every solution the moment it is found (nothing is stored).
// Origin: ChatGPT's solve() logic, moved into its own class by the author.
// ---------------------------------------------------------------------------
class SudokuSolver {
private:
    SudokuBoard board;     // the solver's own working copy of the puzzle
    int solutionCount;     // how many solutions have been found so far

    // Recursive search: fill one empty cell, recurse, then undo the choice.
    void search() {
        int row, col, mask;
        if (!board.findBestCell(row, col, mask)) {        // no empty cells left
            solutionCount++;                              // a full valid grid
            std::cout << "\nSolution " << solutionCount << ":" << std::endl;
            board.print();
            return;
        }
        for (int digit = 1; digit <= 9; digit++) {        // try each candidate
            if (!(mask & (1 << digit))) continue;         // digit does not fit
            board.place(row, col, digit);                 // make a choice
            search();                                     // k+1 cells filled
            board.remove(row, col);                       // undo (backtrack)
        }
        // If the mask was 0, the loop did nothing: dead end, so we return.
    }

public:
    explicit SudokuSolver(const SudokuBoard& puzzle)
        : board(puzzle), solutionCount(0) {}

    // Finds and prints all solutions; returns how many were found.
    int solveAll() {
        solutionCount = 0;
        search();
        return solutionCount;
    }
};

int main() {
    for (int i = 1; i <= 5; i++) {                        // the five puzzle files
        std::string filename = "puzzle" + std::to_string(i) + ".txt";

        std::cout << "\n========================================" << std::endl;
        std::cout << "File: " << filename << std::endl;
        std::cout << "========================================" << std::endl;

        SudokuBoard puzzle;
        if (!puzzle.readPuzzle(filename)) {               // skip unreadable files
            continue;
        }

        std::cout << "\nPuzzle:" << std::endl;             // show the puzzle as read
        puzzle.print();

        if (!puzzle.hasValidGivens()) {                   // illegal puzzle
            std::cout << "\nNo solution found" << std::endl;
            continue;
        }

        SudokuSolver solver(puzzle);                      // search for solutions
        int found = solver.solveAll();
        if (found == 0) {
            std::cout << "\nNo solution found" << std::endl;
        } else {
            std::cout << "\nTotal solutions found: " << found << std::endl;
        }
    }
    return 0;
}
