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
    // Special case: Skip vendor and model properties since they're handled
    // specially in the device section by combining them into a name
    if (name == "vendor" || name == "model") {
        // Check if this is inside a device section - we'd need context here
        // For now, let's just generate no output for these properties
        // They'll be combined into a name in the device section
        return "";
    }

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
        fprintf(stderr, "DEBUG: Adding statement to block, now has %zu statements\n", statements.size() + 1);
        if (dynamic_cast<SectionStatement*>(statement)) {
            SectionStatement* section = dynamic_cast<SectionStatement*>(statement);
            fprintf(stderr, "DEBUG: Statement is a section with name '%s'\n", section->get_name().c_str());
        }
        statements.push_back(statement);
        
        // If this statement is a section, look for its parent in the surrounding blocks
        if (dynamic_cast<SectionStatement*>(statement)) {
            // Find the parent section by walking up the AST
            // We can only do this if we have a parent/owner tracking mechanism
            // For now, this will be handled by the ProgramDeclaration::add_section method
        }
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
            ss  << statement->to_string() << "\n";
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
    : name(name), type(type), block(nullptr), parent_section(nullptr) {}

SectionStatement::SectionStatement(std::string_view name, SectionType type, BlockStatement* block) noexcept 
    : name(name), type(type), block(block), parent_section(nullptr) {}

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

void SectionStatement::set_parent(SectionStatement* parent) noexcept 
{
    parent_section = parent;
}

SectionStatement* SectionStatement::get_parent() const noexcept 
{
    return parent_section;
}

SectionStatement::SectionType SectionStatement::get_effective_type() const noexcept 
{
    // If this is a custom section and has a parent, determine its actual type based on context
    if (type == SectionType::CUSTOM && parent_section != nullptr) {
        SectionType parent_type = parent_section->get_section_type();
        
        // Handle subsections based on parent context
        if (parent_type == SectionType::INTERFACES) {
            // Inside interfaces section, treat subsections as interface definitions
            return SectionType::INTERFACES;
        }
        else if (parent_type == SectionType::IP) {
            // Inside IP section, treat subsections as IP configuration
            return SectionType::IP;
        }
        else if (parent_type == SectionType::ROUTING) {
            // Inside routing section
            return SectionType::ROUTING;
        }
        else if (parent_type == SectionType::FIREWALL) {
            // Inside firewall section
            return SectionType::FIREWALL;
        }
    }
    
    // Default to the original type
    return type;
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

    // Special handling for device section which maps to /system identity
    if (type == SectionType::DEVICE) {
        std::string vendor_value = "";
        std::string model_value = "";
        std::stringstream nested_commands;

        // Extract vendor and model values from property statements
        if (block) {
            for (const auto* stmt : block->get_statements()) {
                if (const auto* prop_stmt = dynamic_cast<const PropertyStatement*>(stmt)) {
                    const std::string& prop_name = prop_stmt->get_name();
                    if (prop_name == "vendor") {
                        if (prop_stmt->get_value()) {
                            vendor_value = prop_stmt->get_value()->to_mikrotik("");
                            // Remove quotes if present
                            if (vendor_value.size() >= 2 && vendor_value.front() == '"' && vendor_value.back() == '"') {
                                vendor_value = vendor_value.substr(1, vendor_value.size() - 2);
                            }
                        }
                    } else if (prop_name == "model") {
                        if (prop_stmt->get_value()) {
                            model_value = prop_stmt->get_value()->to_mikrotik("");
                            // Remove quotes if present
                            if (model_value.size() >= 2 && model_value.front() == '"' && model_value.back() == '"') {
                                model_value = model_value.substr(1, model_value.size() - 2);
                            }
                        }
                    } else {
                        // Process other statements - BUT NOT vendor or model separately
                        // Skip individual vendor/model properties as they're combined into name
                        nested_commands << stmt->to_mikrotik("");
                    }
                }
            }
        }

        // Concatenate vendor and model for the name parameter
        if (!vendor_value.empty() || !model_value.empty()) {
            std::string device_name;
            if (!vendor_value.empty() && !model_value.empty()) {
                // Remove quotes if present in both values
                if (vendor_value.size() >= 2 && vendor_value.front() == '"' && vendor_value.back() == '"') {
                    vendor_value = vendor_value.substr(1, vendor_value.size() - 2);
                }
                if (model_value.size() >= 2 && model_value.front() == '"' && model_value.back() == '"') {
                    model_value = model_value.substr(1, model_value.size() - 2);
                }
                device_name = vendor_value + "_" + model_value;
            } else if (!vendor_value.empty()) {
                // Remove quotes if present
                if (vendor_value.size() >= 2 && vendor_value.front() == '"' && vendor_value.back() == '"') {
                    vendor_value = vendor_value.substr(1, vendor_value.size() - 2);
                }
                device_name = vendor_value;
            } else {
                // Remove quotes if present
                if (model_value.size() >= 2 && model_value.front() == '"' && model_value.back() == '"') {
                    model_value = model_value.substr(1, model_value.size() - 2);
                }
                device_name = model_value;
            }
            
            // Generate the system identity command
            ss << mikrotik_path << " " << action << " name=\"" << device_name << "\"\n";
        }

        // Add nested commands, but NOT duplicate vendor/model statements
        // (Only include commands from nested sections that are not device-related)
        ss << nested_commands.str();
        
        return ss.str(); // Return early after processing device section
    }
    
    // Special handling for interfaces section
if (type == SectionType::INTERFACES) {
    // Reset mikrotik_path to just "/interface" without any dashes
    mikrotik_path = "/interface";
    
    printf("DEBUG: Processing interfaces section with %zu statement(s)\n", 
           block ? block->get_statements().size() : 0);
    
    std::stringstream nested_commands;
        
        // Process all statements within interfaces block
        if (block) {
            for (const auto* stmt : block->get_statements()) {
                // Check if this is a subsection (like "ether1:")
                if (const auto* sub_section = dynamic_cast<const SectionStatement*>(stmt)) {
                    printf("DEBUG: Found subsection with name='%s'\n", sub_section->get_name().c_str());

                    // Get the interface name (e.g., "ether1" from "ether1:")
                    std::string interface_name = sub_section->get_name();
                    
                    // Debug output
                    printf("DEBUG: Interface name before processing: '%s'\n", interface_name.c_str());
                    
                    // Remove trailing colon if present
                    if (!interface_name.empty() && interface_name.back() == ':') {
                        interface_name = interface_name.substr(0, interface_name.size() - 1);
                        printf("DEBUG: Interface name after removing colon: '%s'\n", interface_name.c_str());
                    }
                    
                    // Ensure interface name is valid (not just a colon)
                    if (interface_name.empty()) {
                        printf("ERROR: Empty interface name found, section name='%s'\n", sub_section->get_name().c_str());
                        continue; // Skip invalid interface names
                    }
                    
                    // Variables to store interface properties
                    std::string interface_type = "ethernet"; // Default type
                    std::string description;
                    std::vector<std::string> interface_properties;
                    std::stringstream sub_nested_commands;
                    
                    // Process properties of this interface
                    if (sub_section->get_block()) {
                        for (const auto* sub_stmt : sub_section->get_block()->get_statements()) {
                            if (const auto* prop_stmt = dynamic_cast<const PropertyStatement*>(sub_stmt)) {
                                const std::string& prop_name = prop_stmt->get_name();
                                std::string prop_value;
                                
                                // Extract the value carefully
                                if (prop_stmt->get_value()) {
                                    prop_value = prop_stmt->get_value()->to_mikrotik("");
                                    // Remove quotes if present
                                    if (prop_value.size() >= 2 && prop_value.front() == '"' && prop_value.back() == '"') {
                                        prop_value = prop_value.substr(1, prop_value.size() - 2);
                                    }
                                }
                                
                                // Handle specific properties
                                if (prop_name == "type") {
                                    interface_type = prop_value;
                                }
                                else if (prop_name == "description") {
                                    // Map description to comment
                                    interface_properties.push_back("comment=\"" + prop_value + "\"");
                                }
                                else {
                                    // Add other properties as-is but with cleaned values
                                    interface_properties.push_back(prop_name + "=\"" + prop_value + "\"");
                                }
                            }
                            else if (const auto* nested_section = dynamic_cast<const SectionStatement*>(sub_stmt)) {
                                // Process nested sections (like IP configuration)
                                std::string nested_section_name = nested_section->get_name();
                                
                                // Remove trailing colon if present in nested section name
                                if (!nested_section_name.empty() && nested_section_name.back() == ':') {
                                    nested_section_name = nested_section_name.substr(0, nested_section_name.size() - 1);
                                }
                                
                                // Handle specific nested sections
                                if (nested_section_name == "ip") {
                                    // Process IP configuration for this interface
                                    if (nested_section->get_block()) {
                                        for (const auto* ip_stmt : nested_section->get_block()->get_statements()) {
                                            if (const auto* ip_prop = dynamic_cast<const PropertyStatement*>(ip_stmt)) {
                                                if (ip_prop->get_name() == "address" && ip_prop->get_value()) {
                                                    std::string ip_value = ip_prop->get_value()->to_mikrotik("");
                                                    // Remove quotes if present
                                                    if (ip_value.size() >= 2 && ip_value.front() == '"' && ip_value.back() == '"') {
                                                        ip_value = ip_value.substr(1, ip_value.size() - 2);
                                                    }
                                                    
                                                    // Generate /ip address add command - use hardcoded path
                                                    sub_nested_commands << "/ip address add address=" 
                                                                      << ip_value << " interface=" 
                                                                      << interface_name << "\n";
                                                    
                                                    // Debug output
                                                    printf("DEBUG: Generated IP command: '/ip address add address=%s interface=%s'\n",
                                                           ip_value.c_str(), interface_name.c_str());
                                                }
                                            }
                                        }
                                    }
                                }
                                else {
                                    // Handle other nested sections
                                    sub_nested_commands << nested_section->to_mikrotik("");
                                }
                            }
                            else {
                                // Other types of statements
                                sub_nested_commands << sub_stmt->to_mikrotik("");
                            }
                        }
                    }
                    
                    // Ethernet interfaces often use 'set' instead of 'add'
                    std::string command = "add";
                    if (interface_type == "ethernet") {
                        command = "set";
                        
                        // Debug output
                        printf("DEBUG: Constructing interface command with type='%s', command='%s', name='%s'\n", 
                               interface_type.c_str(), command.c_str(), interface_name.c_str());
                        
                        // Directly generate the interface command with the proper path
                        nested_commands << "/interface " << interface_type << " " << command << " " 
                                       << interface_name;
                        
                        // Add all interface properties
                        for (const auto& prop : interface_properties) {
                            nested_commands << " " << prop;
                        }
                        
                        nested_commands << "\n";
                        
                        // Debug output the generated command
                        printf("DEBUG: Generated interface command: '/interface %s %s %s...'\n", 
                               interface_type.c_str(), command.c_str(), interface_name.c_str());
                    }
                    else {
                        // For other interface types, use 'add' with name as a parameter
                        nested_commands << "/interface " << interface_type << " " << command 
                                      << " name=\"" << interface_name << "\"";
                        
                        // Add all interface properties
                        for (const auto& prop : interface_properties) {
                            nested_commands << " " << prop;
                        }
                        
                        nested_commands << "\n";
                    }
                    
                    // Add any nested commands for this interface
                    nested_commands << sub_nested_commands.str();
                }
                else {
                    // Handle non-subsection statements (e.g., global interface properties)
                    nested_commands << stmt->to_mikrotik("");
                }
            }
        }
        
        // Add all interface commands
        ss << nested_commands.str();
    }

    // Regular processing for other non-device, non-interface sections
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
                std::string sub_name = sub_section->get_name();
                
                // Remove any trailing colon
                if (!sub_name.empty() && sub_name.back() == ':') {
                    sub_name = sub_name.substr(0, sub_name.size() - 1);
                }
                
                // Build the full path for the subsection
                std::string sub_path = mikrotik_path;
                
                // If the current path doesn't end with a slash, add a space
                if (!sub_path.empty() && sub_path.back() != '/') {
                    sub_path += " ";
                }
                
                // Append the sub-section name to the path
                sub_path += sub_name;
                
                // Replace spaces with dashes and convert to lowercase for path consistency
                std::string formatted_sub_path = sub_path;
                std::transform(formatted_sub_path.begin(), formatted_sub_path.end(), 
                                formatted_sub_path.begin(), ::tolower);
                std::replace(formatted_sub_path.begin(), formatted_sub_path.end(), ' ', '-');
                
                // Process the sub-section with the combined path
                std::stringstream sub_section_ss;
                
                // Determine action for the sub-section
                std::string sub_action = determine_action(sub_section->get_section_type(), sub_name);
                
                // Process the properties of the sub-section
                std::vector<std::string> sub_property_params;
                std::stringstream sub_nested_commands;
                
                if (sub_section->get_block()) {
                    for (const auto* sub_stmt : sub_section->get_block()->get_statements()) {
                        if (const auto* sub_prop = dynamic_cast<const PropertyStatement*>(sub_stmt)) {
                            sub_property_params.push_back(sub_prop->to_mikrotik(""));
                        } else {
                            // For deeper nested statements, use regular processing with increased indentation
                            sub_nested_commands << sub_stmt->to_mikrotik("");
                        }
                    }
                }
                
                // If we have properties, assemble the command for the sub-section
                if (!sub_property_params.empty()) {
                    sub_section_ss  << formatted_sub_path << " " << sub_action;
                    
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
                nested_commands << stmt->to_mikrotik("");
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

std::string SectionStatement::determine_action(SectionType type, const std::string& section_name) {
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
      // Usar sección interfaces"
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