#ifndef SCANNER_HPP
#define SCANNER_HPP

#include <vector>

// Variables from scanner
extern int line_number;
extern int column_number;
extern std::vector<int> indent_stack;
extern std::vector<int> token_queue;

// Function declarations
extern int yylex();
extern int yylex_flex();
int check_token_queue();

#endif /* SCANNER_HPP */ 