#include "statement.hpp"
#include "declaration.hpp"
#include <sstream>
#include <algorithm>

// PropertyStatement implementation
PropertyStatement::PropertyStatement(std::string_view name, Expression* value) noexcept 
    : name(name), value(value) {}

const std::string& PropertyStatement::get_name() const noexcept 
{
    return name;
}

Expression* PropertyStatement::get_value() const noexcept 
{
    return value;
}

void PropertyStatement::destroy() noexcept 
{
    if (value) {
        value->destroy();
        delete value;
        value = nullptr;
    }
}

std::string PropertyStatement::to_string() const 
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

std::string PropertyStatement::to_mikrotik(const std::string& ident) const
{
    std::stringstream ss;
    
    // In MikroTik, properties are typically passed as parameters in the format name=value
    ss << ident;
    
    // Special handling for common properties
    if (name == "enabled" || name == "disabled") {
        // Boolean properties often use lowercase in MikroTik
        ss << name << "=";
    } else {
        ss << name << "=";
    }
    
    if (value) {
        // Use empty indentation for inline values
        ss << value->to_mikrotik("");
    } else {
        ss << "\"\""; // Empty string for null values
    }
    
    ss << "\n";
    return ss.str();
}

// BlockStatement implementation
BlockStatement::BlockStatement() noexcept : statements() {}

BlockStatement::BlockStatement(const StatementList& statements) noexcept 
    : statements(statements) {}

void BlockStatement::add_statement(Statement* statement) noexcept 
{
    if (statement) {
        statements.push_back(statement);
    }
}

const StatementList& BlockStatement::get_statements() const noexcept 
{
    return statements;
}

void BlockStatement::destroy() noexcept 
{
    for (auto* statement : statements) {
        if (statement) {
            statement->destroy();
            delete statement;
        }
    }
    statements.clear();
}

std::string BlockStatement::to_string() const 
{
    std::stringstream ss;
    for (const auto* statement : statements) {
        if (statement) {
            ss << "    " << statement->to_string() << "\n";
        }
    }
    return ss.str();
}

std::string BlockStatement::to_mikrotik(const std::string& ident) const
{
    std::stringstream ss;
    
    // In MikroTik, code blocks are enclosed in curly braces
    ss << ident << "{\n";
    
    // Process all statements in the block
    for (const auto* statement : statements) {
        if (statement) {
            ss << statement->to_mikrotik(ident + "    ");
        }
    }
    
    ss << ident << "}\n";
    return ss.str();
}

// SectionStatement implementation
SectionStatement::SectionStatement(std::string_view name, SectionType type) noexcept 
    : name(name), type(type), block(nullptr) {}

SectionStatement::SectionStatement(std::string_view name, SectionType type, BlockStatement* block) noexcept 
    : name(name), type(type), block(block) {}

const std::string& SectionStatement::get_name() const noexcept 
{
    return name;
}

SectionStatement::SectionType SectionStatement::get_section_type() const noexcept 
{
    return type;
}

BlockStatement* SectionStatement::get_block() const noexcept 
{
    return block;
}

void SectionStatement::set_block(BlockStatement* block) noexcept 
{
    this->block = block;
}

std::string SectionStatement::section_type_to_string(SectionType type) 
{
    switch (type) {
        case SectionType::DEVICE: return "device";
        case SectionType::INTERFACES: return "interfaces";
        case SectionType::IP: return "ip";
        case SectionType::ROUTING: return "routing";
        case SectionType::FIREWALL: return "firewall";
        case SectionType::SYSTEM: return "system";
        case SectionType::CUSTOM: return "custom";
        default: return "unknown";
    }
}

void SectionStatement::destroy() noexcept 
{
    if (block) {
        block->destroy();
        delete block;
        block = nullptr;
    }
}

std::string SectionStatement::to_string() const 
{
    std::stringstream ss;
    ss << name << ":\n";
    if (block) {
        ss << block->to_string();
    }
    return ss.str();
}

std::string SectionStatement::to_mikrotik(const std::string& ident) const
{
    std::stringstream ss;
    ss << ident << "# Section: " << name << " (Type: " << section_type_to_string(type) << ")\n";
    
    // Map section types to MikroTik command paths
    std::string mikrotik_path;
    switch (type) {
        case SectionType::DEVICE:
            mikrotik_path = "/system identity";
            break;
        case SectionType::INTERFACES:
            mikrotik_path = "/interface";
            break;
        case SectionType::IP:
            mikrotik_path = "/ip";
            break;
        case SectionType::ROUTING:
            mikrotik_path = "/routing";
            break;
        case SectionType::FIREWALL:
            mikrotik_path = "/ip firewall";
            break;
        case SectionType::SYSTEM:
            mikrotik_path = "/system";
            break;
        case SectionType::CUSTOM:
        default:
            // For custom sections, use the name as the path
            mikrotik_path = "/" + name;
            // Convert spaces to dashes and make lowercase
            std::transform(mikrotik_path.begin(), mikrotik_path.end(), mikrotik_path.begin(), ::tolower);
            std::replace(mikrotik_path.begin(), mikrotik_path.end(), ' ', '-');
            break;
    }
    
    ss << ident << mikrotik_path << "\n";
    
    // Process the block if it exists
    if (block) {
        // For sections, we don't add a block wrapper, just process the statements
        // as they may need to use the current path context
        for (const auto* statement : block->get_statements()) {
            if (statement) {
                ss << statement->to_mikrotik(ident);
            }
        }
    }
    
    return ss.str();
}

// DeclarationStatement implementation
DeclarationStatement::DeclarationStatement(Declaration* decl) noexcept 
    : declaration(decl) {}

Declaration* DeclarationStatement::get_declaration() const noexcept 
{
    return declaration;
}

void DeclarationStatement::destroy() noexcept 
{
    if (declaration) {
        declaration->destroy();
        delete declaration;
        declaration = nullptr;
    }
}

std::string DeclarationStatement::to_string() const 
{
    return declaration ? declaration->to_string() : "null";
}

std::string DeclarationStatement::to_mikrotik(const std::string& ident) const
{
    // Just delegate to the declaration's to_mikrotik method
    return declaration ? declaration->to_mikrotik(ident) : ident + "# null declaration\n";
} 