#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "datatype.hpp"
#include "declaration.hpp"
#include "expression.hpp"
#include "statement.hpp"

extern FILE* yyin;
extern int yyparse();
extern int line_number;
extern int yydebug;
extern ProgramDeclaration* parser_result;

void usage(char* argv[]) {
    printf("Usage: %s input_file [output_file]\n", argv[0]);
    printf("       If output_file is not specified, it will be input_file.rsc\n");
    exit(1);
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        usage(argv);
    }

    yyin = fopen(argv[1], "r");

    if (!yyin) {
        printf("Could not open %s\n", argv[1]);
        exit(1);
    }

    printf("Parsing %s...\n", argv[1]);
    
    /* Enable parser debugging if needed */
    // yydebug = 1;
    
    int parse_result = yyparse();

    if (parse_result == 0) {
        printf("Parse successful! The input conforms to the Mikrotik DSL grammar.\n");
        
        // Generate output filename from input if not provided
        char output_filename[256];
        if (argc == 3) {
            strncpy(output_filename, argv[2], sizeof(output_filename) - 1);
            output_filename[sizeof(output_filename) - 1] = '\0';
        } else {
            snprintf(output_filename, sizeof(output_filename), "%s.rsc", argv[1]);
        }
        
        // Check if the AST was successfully built
        if (parser_result) {
            printf("Translating to RouterOS script...\n");
            
            // Open output file for writing
            std::ofstream output_file(output_filename);
            if (output_file.is_open()) {
                // Get the translated script as a string
                std::string routeros_script = parser_result->to_mikrotik("");
                
                // Write to the output file
                output_file << routeros_script;
                output_file.close();
                
                printf("Translation complete. Output written to %s\n", output_filename);
                
                // Print AST for debugging if needed
                printf("Parsed AST Structure:\n%s\n", parser_result->to_string().c_str());
            } else {
                printf("Error: Could not open output file %s\n", output_filename);
            }
            
            // Clean up resources
            parser_result->destroy();
            delete parser_result;
        } else {
            printf("Error: Failed to build AST during parsing.\n");
        }
    } else {
        printf("Parse failed! The input contains syntax errors.\n");
    }

    fclose(yyin);
    
    return parse_result;
} 