#pragma once

#include <list>
#include <forward_list>
#include <string>
#include <string_view>
#include <vector>

// Forward declarations of main node types
class Declaration;
class Expression;
class Statement;
class Datatype;
class ConfigSection;
class Property;
class Value;
class ProgramDeclaration;
using Body = std::list<Statement*>;
// Type aliases for common structures
using StatementList = std::vector<Statement*>;
using PropertyList = std::vector<Property*>;
using ValueList = std::vector<Value*>;

// Helper function to destroy a list of statements
void destroy_statements(StatementList& statements) noexcept;
// Helper function to safely destroy a program declaration
void destroy_program(ProgramDeclaration* program) noexcept;
std::string body_to_mikrotik(const Body& body, const std::string& ident) noexcept;
// Base interface for all AST nodes
class ASTNodeInterface
{
public:
    virtual ~ASTNodeInterface() noexcept;
    
    // Method to properly destroy the node and its children
    virtual void destroy() noexcept = 0;
    
    // Method to generate a string representation (useful for debugging)
    virtual std::string to_string() const = 0;
    virtual std::string to_mikrotik(const std::string& ident) const = 0;
}; 