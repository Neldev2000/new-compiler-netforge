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
    
    ss << name << "=";
    
    if (value) {
        ss << value->to_mikrotik("");
    } else {
        ss << "\"\""; // Empty string for null values
    }
    
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
    
    // Process all statements in the block without adding extra indentation
    for (const auto* statement : statements) {
        if (statement) {
            ss << statement->to_mikrotik(ident);
        }
    }

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

    // Determine action based on section type and name
    std::string action = determine_action(type, name);

    // Recopilate parameters from PropertyStatement children
    std::vector<std::string> property_params;
    std::stringstream nested_commands;

    if (block) {
        for (const auto* stmt : block->get_statements()) {
            if (const auto* prop_stmt = dynamic_cast<const PropertyStatement*>(stmt)) {
                // Add the name=value pair without additional formatting
                property_params.push_back(prop_stmt->to_mikrotik(""));
            } else if (const auto* sub_section = dynamic_cast<const SectionStatement*>(stmt)) {
                // Handle sub-section: adjust the path for the nested section
                // For example, if we're in /ip and have a firewall sub-section, 
                // the full path would be /ip firewall
                std::string sub_path = mikrotik_path;
                
                // If the current path doesn't end with a slash, add a space
                if (!sub_path.empty() && sub_path.back() != '/') {
                    sub_path += " ";
                }
                
                // Append the sub-section name to the path
                sub_path += sub_section->get_name();
                
                // Replace spaces with dashes and convert to lowercase for path consistency
                std::string formatted_sub_path = sub_path;
                std::transform(formatted_sub_path.begin(), formatted_sub_path.end(), 
                                formatted_sub_path.begin(), ::tolower);
                std::replace(formatted_sub_path.begin(), formatted_sub_path.end(), ' ', '-');
                
                // Create a temporary section statement with the new path
                SectionStatement temp_section(
                    sub_section->get_name(), 
                    SectionType::CUSTOM,  // Using CUSTOM since we've already built the path
                    sub_section->get_block()
                );
                
                // Process the sub-section with the combined path
                std::stringstream sub_section_ss;
                sub_section_ss << ident << "# Sub-section: " << sub_section->get_name() 
                              << " (Full path: " << formatted_sub_path << ")\n";
                
                // Process the properties of the sub-section
                std::vector<std::string> sub_property_params;
                std::stringstream sub_nested_commands;
                
                if (sub_section->get_block()) {
                    for (const auto* sub_stmt : sub_section->get_block()->get_statements()) {
                        if (const auto* sub_prop = dynamic_cast<const PropertyStatement*>(sub_stmt)) {
                            sub_property_params.push_back(sub_prop->to_mikrotik(""));
                        } else {
                            // For deeper nested statements, use regular processing with increased indentation
                            sub_nested_commands << sub_stmt->to_mikrotik(ident + "    ");
                        }
                    }
                }
                
                // Determine action for the sub-section
                std::string sub_action = determine_action(sub_section->get_section_type(), sub_section->get_name());
                
                // If we have properties, assemble the command for the sub-section
                if (!sub_property_params.empty()) {
                    sub_section_ss << ident << formatted_sub_path << " " << sub_action;
                    
                    // Add all parameters with spaces between them
                    for (const auto& param : sub_property_params) {
                        sub_section_ss << " " << param;
                    }
                    sub_section_ss << "\n";
                }
                
                // Add any deeply nested commands
                sub_section_ss << sub_nested_commands.str();
                
                // Add the sub-section output to our nested commands
                nested_commands << sub_section_ss.str();
            } else {
                // For other statement types (that are not PropertyStatement or SectionStatement)
                nested_commands << stmt->to_mikrotik(ident + "    ");
            }
        }
    }

    // If we have properties, assemble the command
    if (!property_params.empty()) {
        ss << ident << mikrotik_path << " " << action;
        
        // Add all parameters with spaces between them
        for (const auto& param : property_params) {
            ss << " " << param;
        }
        ss << "\n";
    }

    // Add any nested commands
    ss << nested_commands.str();
    
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

  static std::string determine_action(SectionType type, const std::string& section_name) {
      // Usar sección system
      if (type == SectionType::SYSTEM) {
          if (section_name == "identity" || section_name == "clock" || section_name == "ntp client") {
              return "set";
          } else if (section_name == "backup") {
              return "save";
          } else if (section_name == "scheduler" || section_name == "script") {
              return "add";
          }
      }
      // Usar sección interfaces
      else if (type == SectionType::INTERFACES) {
          if (section_name == "bridge port") {
              return "add";
          }
          return "add"; // Por defecto para interfaces
      }
      // Usar sección IP
      else if (type == SectionType::IP) {
          if (section_name == "dns" || section_name == "settings") {
              return "set";
          } else if (section_name == "address" || section_name == "route" ||
                    section_name == "pool" || section_name == "dhcp-server" ||
                    section_name.find("firewall") != std::string::npos) {
              return "add";
          }
      }
      // Routing normalmente usa add
      else if (type == SectionType::ROUTING) {
          return "add";
      }
      // Firewall normalmente usa add
      else if (type == SectionType::FIREWALL) {
          return "add";
      }
      // Device depende del contexto
      else if (type == SectionType::DEVICE) {
          if (section_name == "user") {
              return "add";
          }
      }

      // Valor por defecto
      return "set";
  }