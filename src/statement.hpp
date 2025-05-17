#pragma once

#include "ast_node_interface.hpp"
#include "expression.hpp"
#include "datatype.hpp"

// Forward declaration
class Declaration;

// Base class for all statements
class Statement : public ASTNodeInterface
{
    // Empty base class
};

// Property assignment statement (key = value)
class PropertyStatement : public Statement
{
public:
    PropertyStatement(std::string_view name, Expression* value) noexcept;
    
    const std::string& get_name() const noexcept;
    Expression* get_value() const noexcept;
    void destroy() noexcept override;
    std::string to_string() const override;
    std::string to_mikrotik(const std::string& ident) const override;
    
private:
    std::string name;
    Expression* value;
};

// Block statement (a collection of statements)
class BlockStatement : public Statement
{
public:
    BlockStatement() noexcept;
    BlockStatement(const StatementList& statements) noexcept;
    
    // Add a statement to this block
    void add_statement(Statement* statement) noexcept;
    
    const StatementList& get_statements() const noexcept;
    void destroy() noexcept override;
    std::string to_string() const override;
    std::string to_mikrotik(const std::string& ident) const override;
    
private:
    StatementList statements;
};

// Section statement (named block with type)
class SectionStatement : public Statement
{
public:
    // Enum to define different section types
    enum class SectionType {
        DEVICE,
        INTERFACES,
        IP,
        ROUTING,
        FIREWALL,
        SYSTEM,
        CUSTOM
    };
    
    SectionStatement(std::string_view name, SectionType type) noexcept;
    SectionStatement(std::string_view name, SectionType type, BlockStatement* block) noexcept;
    
    const std::string& get_name() const noexcept;
    SectionType get_section_type() const noexcept;
    BlockStatement* get_block() const noexcept;
    
    // Set the block for this section
    void set_block(BlockStatement* block) noexcept;
    
    // Static method to convert section type to string
    static std::string section_type_to_string(SectionType type);
    
    void destroy() noexcept override;
    std::string to_string() const override;
    std::string to_mikrotik(const std::string& ident) const override;
    
private:
    std::string name;
    SectionType type;
    BlockStatement* block;
};

// Declaration statement (wrapper for a declaration)
class DeclarationStatement : public Statement
{
public:
    DeclarationStatement(Declaration* decl) noexcept;
    
    Declaration* get_declaration() const noexcept;
    void destroy() noexcept override;
    std::string to_string() const override;
    std::string to_mikrotik(const std::string& ident) const override;
    
private:
    Declaration* declaration;
}; 