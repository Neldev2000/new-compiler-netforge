#include "declaration.hpp"
#include <sstream>
#include <algorithm>

// Declaration implementation
Declaration::Declaration(std::string_view decl_name) noexcept : name(decl_name) {}

const std::string& Declaration::get_name() const noexcept 
{
    return name;
}

std::string Declaration::to_mikrotik(const std::string& ident) const
{
    return ident + "# Declaration: " + name;
}

// ConfigDeclaration implementation
ConfigDeclaration::ConfigDeclaration(std::string_view config_name) noexcept 
    : Declaration(config_name), statements() {}

ConfigDeclaration::ConfigDeclaration(std::string_view config_name, const StatementList& statements) noexcept 
    : Declaration(config_name), statements(statements) {}

void ConfigDeclaration::add_statement(Statement* statement) noexcept 
{
    if (statement) {
        statements.push_back(statement);
    }
}

const StatementList& ConfigDeclaration::get_statements() const noexcept 
{
    return statements;
}

void ConfigDeclaration::destroy() noexcept 
{
    for (auto* statement : statements) {
        if (statement) {
            statement->destroy();
            delete statement;
        }
    }
    statements.clear();
}

std::string ConfigDeclaration::to_string() const 
{
    std::stringstream ss;
    ss << name << ":\n";
    for (const auto* statement : statements) {
        if (statement) {
            ss << "    " << statement->to_string() << "\n";
        }
    }
    return ss.str();
}

std::string ConfigDeclaration::to_mikrotik(const std::string& ident) const
{
    std::stringstream ss;
    // MikroTik uses path-based configuration structure
    ss << ident << "# Configuration section: " << name << "\n";
    ss << ident << "/";
    
    // Convert section name to MikroTik menu path
    // Replace spaces with dashes and lowercase the name
    std::string menu_path = name;
    std::replace(menu_path.begin(), menu_path.end(), ' ', '-');
    std::transform(menu_path.begin(), menu_path.end(), menu_path.begin(), ::tolower);
    
    ss << menu_path << "\n";
    
    // Process all statements within this configuration block
    for (const auto* statement : statements) {
        if (statement) {
            ss << statement->to_mikrotik(ident + "    ");
        }
    }
    
    return ss.str();
}

// PropertyDeclaration implementation
PropertyDeclaration::PropertyDeclaration(std::string_view prop_name, Expression* value) noexcept 
    : Declaration(prop_name), value(value) {}

Expression* PropertyDeclaration::get_value() const noexcept 
{
    return value;
}

void PropertyDeclaration::destroy() noexcept 
{
    if (value) {
        value->destroy();
        delete value;
        value = nullptr;
    }
}

std::string PropertyDeclaration::to_string() const 
{
    std::stringstream ss;
    ss << name << " = ";
    if (value) {
        ss << value->to_string();
    } else {
        ss << "null";
    }
    return ss.str();
}

std::string PropertyDeclaration::to_mikrotik(const std::string& ident) const
{
    std::stringstream ss;
    
    // Property declarations in MikroTik are typically done with 'set' command
    // or directly as parameters to other commands
    ss << ident << "set " << name << "=";
    
    if (value) {
        // Remove indentation for inline values
        ss << value->to_mikrotik("");
    } else {
        ss << "\"\""; // Empty string for null values
    }
    
    ss << "\n";
    return ss.str();
}

// InterfaceDeclaration implementation
InterfaceDeclaration::InterfaceDeclaration(std::string_view iface_name) noexcept 
    : Declaration(iface_name), statements() {}

InterfaceDeclaration::InterfaceDeclaration(std::string_view iface_name, const StatementList& statements) noexcept 
    : Declaration(iface_name), statements(statements) {}

void InterfaceDeclaration::add_statement(Statement* statement) noexcept 
{
    if (statement) {
        statements.push_back(statement);
    }
}

const StatementList& InterfaceDeclaration::get_statements() const noexcept 
{
    return statements;
}

void InterfaceDeclaration::destroy() noexcept 
{
    for (auto* statement : statements) {
        if (statement) {
            statement->destroy();
            delete statement;
        }
    }
    statements.clear();
}

std::string InterfaceDeclaration::to_string() const 
{
    std::stringstream ss;
    ss << name << ":\n";
    for (const auto* statement : statements) {
        if (statement) {
            ss << "    " << statement->to_string() << "\n";
        }
    }
    return ss.str();
}

std::string InterfaceDeclaration::to_mikrotik(const std::string& ident) const
{
    std::stringstream ss;
    
    // Interface declarations in MikroTik use /interface path
    ss << ident << "# Interface: " << name << "\n";
    ss << ident << "/interface\n";
    
    // Assume the first part of the name indicates the interface type (e.g., "ether1" -> "ethernet")
    std::string iface_type = "ethernet"; // Default type
    std::string iface_name = name;
    
    size_t pos = name.find_first_of("0123456789");
    if (pos != std::string::npos && pos > 0) {
        iface_type = name.substr(0, pos);
        // Map common interface abbreviations to MikroTik types
        if (iface_type == "eth" || iface_type == "ether") {
            iface_type = "ethernet";
        } else if (iface_type == "wlan" || iface_type == "wifi") {
            iface_type = "wireless";
        } else if (iface_type == "br") {
            iface_type = "bridge";
        }
    }
    
    // Add interface command
    ss << ident << "add name=" << iface_name << " type=" << iface_type << "\n";
    
    // Process all statements for this interface
    for (const auto* statement : statements) {
        if (statement) {
            ss << statement->to_mikrotik(ident + "    ");
        }
    }
    
    return ss.str();
}

// ProgramDeclaration implementation
ProgramDeclaration::ProgramDeclaration() noexcept 
    : Declaration("program"), sections() {}

void ProgramDeclaration::add_section(SectionStatement* section) noexcept 
{
    if (section) {
        sections.push_back(section);
    }
}

const std::vector<SectionStatement*>& ProgramDeclaration::get_sections() const noexcept 
{
    return sections;
}

void ProgramDeclaration::destroy() noexcept 
{
    for (auto* section : sections) {
        if (section) {
            section->destroy();
            delete section;
        }
    }
    sections.clear();
}

std::string ProgramDeclaration::to_string() const 
{
    std::stringstream ss;
    for (const auto* section : sections) {
        if (section) {
            ss << section->to_string() << "\n";
        }
    }
    return ss.str();
}

std::string ProgramDeclaration::to_mikrotik(const std::string& ident) const
{
    std::stringstream ss;
    
    // Add script header with metadata and information
    ss << ident << "# MikroTik RouterOS Script\n";
    ss << ident << "# Generated by Compiler Project\n\n";
    
    // Add initialization block - this is where global variables would be defined
    ss << ident << "# Initialization\n";
    ss << ident << "{\n";
    
    // Process all top-level sections
    for (const auto* section : sections) {
        if (section) {
            ss << section->to_mikrotik(ident + "    ");
        }
    }
    
    // Close initialization block
    ss << ident << "}\n";
    
    return ss.str();
} 