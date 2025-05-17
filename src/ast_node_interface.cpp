#include "ast_node_interface.hpp"
#include "statement.hpp"
#include "declaration.hpp"

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
    std::string result = "";
    for(Statement* statement : body)
    {
        result += statement->to_mikrotik(ident) + "\n";
    }
    return result;
}

// Virtual destructor implementation
ASTNodeInterface::~ASTNodeInterface() noexcept {} 