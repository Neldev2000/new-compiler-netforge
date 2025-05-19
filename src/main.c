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
#include "specialized_sections.hpp"

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

// Perform semantic analysis on the AST
bool validate_semantics(ProgramDeclaration* program) {
    bool valid = true;
    std::vector<std::string> validation_errors;
    
    // Option to disable validation during debugging
    bool skip_validation = false;
    
    // Check if there's an environment variable to skip validation
    const char* skip_env = getenv("SKIP_VALIDATION");
    if (skip_env && (strcmp(skip_env, "1") == 0 || strcmp(skip_env, "true") == 0)) {
        printf("Warning: Skipping semantic validation due to SKIP_VALIDATION environment variable\n");
        return true;
    }
    
    // Validate each section in the program
    for (const auto* section : program->get_sections()) {
        // Check if this is a specialized section
        const SpecializedSection* specialized = dynamic_cast<const SpecializedSection*>(section);
        if (specialized) {
            try {
                // Call the validate method
                auto [is_valid, error_message] = specialized->validate();
                if (!is_valid) {
                    valid = false;
                    validation_errors.push_back("Error in section '" + specialized->get_name() + "': " + error_message);
                }
            } catch (const std::exception& e) {
                valid = false;
                validation_errors.push_back("Exception in section '" + specialized->get_name() + "': " + e.what());
            } catch (...) {
                valid = false;
                validation_errors.push_back("Unknown error in section '" + specialized->get_name() + "'");
            }
        }
    }
    
    // Display validation errors if any
    if (!valid) {
        printf("Semantic validation failed with the following errors:\n");
        for (const auto& error : validation_errors) {
            printf("- %s\n", error.c_str());
        }
    }
    
    return valid;
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


    /* Enable parser debugging if needed */
    // yydebug = 1;
    
    int parse_result = yyparse();

    if (parse_result == 0) {
  
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
            // Perform semantic validation before generating code
            if (validate_semantics(parser_result)) {
                // Validation passed, generate code
                printf("Semantic validation passed. Generating RouterOS script...\n");
                
                // Open output file for writing
                std::ofstream output_file(output_filename);
                if (output_file.is_open()) {
                    // Get the translated script as a string
                    std::string routeros_script = parser_result->to_mikrotik("");
                    
                    // Write to the output file
                    output_file << routeros_script;
                    output_file.close();
                    
                    printf("RouterOS script successfully written to %s\n", output_filename);
                } else {
                    printf("Error: Could not open output file %s\n", output_filename);
                }
            } else {
                printf("Compilation aborted due to semantic errors.\n");
                return 1;
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