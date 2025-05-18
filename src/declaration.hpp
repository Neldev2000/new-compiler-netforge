#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <set>

#include "ast_node_interface.hpp"
#include "statement.hpp"

// Base class for declarations
class Declaration : public ASTNodeInterface
{
public:
    Declaration(std::string_view decl_name) noexcept;
    
    const std::string& get_name() const noexcept;
    std::string to_mikrotik(const std::string& ident) const override;
    
protected:
    std::string name;
};

// Declaration for a configuration section
class ConfigDeclaration : public Declaration
{
public:
    ConfigDeclaration(std::string_view config_name) noexcept;
    ConfigDeclaration(std::string_view config_name, const StatementList& statements) noexcept;
    
    // Add a statement to this configuration
    void add_statement(Statement* statement) noexcept;
    
    const StatementList& get_statements() const noexcept;
    void destroy() noexcept override;
    std::string to_string() const override;
    std::string to_mikrotik(const std::string& ident) const override;
    
private:
    // Helper method to determine action based on path and config name
    std::string determine_action(const std::string& menu_path) const;
    
    StatementList statements;
};

// Program root declaration (holds all top-level sections)
class ProgramDeclaration : public Declaration
{
public:
    ProgramDeclaration() noexcept;
    
    // Add a section to this program
    void add_section(SectionStatement* section) noexcept;
    
    const std::vector<SectionStatement*>& get_sections() const noexcept;
    void destroy() noexcept override;
    std::string to_string() const override;
    std::string to_mikrotik(const std::string& ident) const override;
    
private:
    std::vector<SectionStatement*> sections;
}; 