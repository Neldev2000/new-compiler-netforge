#include "ast_node_interface.hpp"
#include "statement.hpp"
#include "declaration.hpp"
#include <sstream>

// Implementation of helper function to destroy a list of statements
void destroy_statements(StatementList& statements) noexcept
{
    for (auto* statement : statements)
    {
        if (statement)
        {
            statement->destroy();
            delete statement;
        }
    }
    statements.clear();
}

// Implementation of helper function to safely destroy a program declaration
void destroy_program(ProgramDeclaration* program) noexcept
{
    if (program)
    {
        program->destroy();
        delete program;
    }
}

std::string body_to_mikrotik(const Body& body, const std::string& ident) noexcept
{
    std::stringstream result;
    
    for(Statement* statement : body)
    {
        if (statement) {
            std::string stmt_output = statement->to_mikrotik(ident);
            
            // Añadir la salida del statement a nuestro resultado
            result << stmt_output;
            
            // Solo añadir un salto de línea si el statement no termina ya con uno
            if (!stmt_output.empty() && stmt_output.back() != '\n') {
                result << '\n';
            }
        }
    }
    
    return result.str();
}

// Virtual destructor implementation
ASTNodeInterface::~ASTNodeInterface() noexcept {} 