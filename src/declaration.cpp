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
    
    // Skip adding comments about the configuration section
    // ss << ident << "# Configuration section: " << name << "\n";
    
    // Determine the MikroTik base path from the configuration name
    std::string menu_path;
    
    // Convert name to lowercase for case-insensitive comparison
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    
    // Skip device/vendor/model processing - this is already handled by SectionStatement
    if (lower_name == "device" || lower_name == "system identity") {
        return ""; // Skip processing to avoid duplicate system identity commands
    }
    
    // Map common configuration names to MikroTik paths
    if (lower_name.find("dhcp") != std::string::npos) {
        if (lower_name.find("server") != std::string::npos) {
            menu_path = "/ip dhcp-server";
        } else if (lower_name.find("network") != std::string::npos) {
            menu_path = "/ip dhcp-server network";
        } else if (lower_name.find("client") != std::string::npos) {
            menu_path = "/ip dhcp-client";
        } else if (lower_name.find("pool") != std::string::npos) {
            menu_path = "/ip pool";
        } else {
            menu_path = "/ip dhcp-server";
        }
    } else if (lower_name.find("firewall") != std::string::npos) {
        if (lower_name.find("nat") != std::string::npos) {
            menu_path = "/ip firewall nat";
        } else if (lower_name.find("filter") != std::string::npos) {
            menu_path = "/ip firewall filter";
        } else if (lower_name.find("mangle") != std::string::npos) {
            menu_path = "/ip firewall mangle";
        } else {
            menu_path = "/ip firewall filter";
        }
    } else if (lower_name.find("interface") != std::string::npos || lower_name.find("iface") != std::string::npos) {
        if (lower_name.find("bridge") != std::string::npos) {
            if (lower_name.find("port") != std::string::npos) {
                menu_path = "/interface bridge port";
            } else {
                menu_path = "/interface bridge";
            }
        } else if (lower_name.find("vlan") != std::string::npos) {
            menu_path = "/interface vlan";
        } else if (lower_name.find("wireless") != std::string::npos || lower_name.find("wifi") != std::string::npos) {
            menu_path = "/interface wireless";
        } else {
            menu_path = "/interface";
        }
    } else if (lower_name.find("ip") != std::string::npos) {
        if (lower_name.find("address") != std::string::npos) {
            menu_path = "/ip address";
        } else if (lower_name.find("dns") != std::string::npos) {
            menu_path = "/ip dns";
        } else if (lower_name.find("route") != std::string::npos) {
            menu_path = "/ip route";
        } else {
            menu_path = "/ip";
        }
    } else if (lower_name.find("routing") != std::string::npos) {
        if (lower_name.find("ospf") != std::string::npos) {
            menu_path = "/routing ospf";
        } else if (lower_name.find("bgp") != std::string::npos) {
            menu_path = "/routing bgp";
        } else {
            menu_path = "/routing";
        }
    } else if (lower_name.find("system") != std::string::npos) {
        if (lower_name.find("scheduler") != std::string::npos) {
            menu_path = "/system scheduler";
        } else if (lower_name.find("script") != std::string::npos) {
            menu_path = "/system script";
        } else if (lower_name.find("identity") != std::string::npos) {
            menu_path = "/system identity";
        } else if (lower_name.find("ntp") != std::string::npos || lower_name.find("time") != std::string::npos) {
            menu_path = "/system ntp client";
        } else if (lower_name.find("clock") != std::string::npos) {
            menu_path = "/system clock";
        } else if (lower_name.find("backup") != std::string::npos) {
            menu_path = "/system backup";
        } else {
            menu_path = "/system";
        }
    } else if (lower_name.find("user") != std::string::npos) {
        menu_path = "/user";
    } else {
        // Default: Convert section name to MikroTik menu path format
        // Replace spaces with dashes and use lowercase
        menu_path = "/" + lower_name;
        std::replace(menu_path.begin(), menu_path.end(), ' ', '-');
    }
    
    // Determine the appropriate action based on path and configuration
    std::string action = determine_action(menu_path);
    
    // Special handling for system identity
    if (menu_path == "/system identity") {
        std::string vendor_value = "";
        std::string model_value = "";
        std::stringstream nested_commands;
        
        // Find vendor and model properties
        for (const auto* statement : statements) {
            if (statement) {
                if (const auto* prop_stmt = dynamic_cast<const PropertyStatement*>(statement)) {
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
                    }
                } else {
                    // Process other types of statements
                    nested_commands << statement->to_mikrotik(ident + "    ");
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
            ss << ident << menu_path << " " << action << " name=\"" << device_name << "\"\n";
        }
        
        // Add nested commands
        ss << nested_commands.str();
        
        return ss.str();
    }
    
    // Collect parameters from child statements
    std::vector<std::string> property_params;
    std::stringstream nested_commands;
    
    // Process all statements within this configuration block to gather parameters
    for (const auto* statement : statements) {
        if (statement) {
            if (const auto* prop_stmt = dynamic_cast<const PropertyStatement*>(statement)) {
                // For property statements, extract the name=value pair
                property_params.push_back(prop_stmt->to_mikrotik(""));
            } else if (const auto* prop_decl = dynamic_cast<const PropertyDeclaration*>(statement)) {
                // For property declarations, extract the name=value pair
                std::string prop_value = prop_decl->to_mikrotik("");
                // If to_mikrotik returns a full command like "set name=value\n", extract just the parameter
                size_t equals_pos = prop_value.find('=');
                if (equals_pos != std::string::npos) {
                    size_t start_pos = 0;
                    if (prop_value.find("set ") == 0) {
                        start_pos = 4; // Skip "set "
                    }
                    size_t newline_pos = prop_value.find('\n');
                    if (newline_pos != std::string::npos) {
                        prop_value = prop_value.substr(start_pos, newline_pos - start_pos);
                    } else {
                        prop_value = prop_value.substr(start_pos);
                    }
                }
                property_params.push_back(prop_value);
            } else {
                // For other types of statements (like nested configurations)
                nested_commands << statement->to_mikrotik(ident + "    ");
            }
        }
    }
    
    // Assemble the complete command with path, action, and parameters
    if (!property_params.empty()) {
        ss << ident << menu_path << " " << action;
        
        // Add all parameters separated by spaces
        for (const auto& param : property_params) {
            ss << " " << param;
        }
        
        ss << "\n";
    }
    
    // Add any nested commands
    ss << nested_commands.str();
    
    return ss.str();
}

// Helper method to determine the appropriate action based on the MikroTik path
std::string ConfigDeclaration::determine_action(const std::string& menu_path) const
{
    // Common paths that use "set"
    if (menu_path == "/system identity" || 
        menu_path == "/system clock" || 
        menu_path == "/system ntp client" ||
        menu_path == "/ip dns") {
        return "set";
    }
    
    // Backup always uses "save"
    if (menu_path == "/system backup") {
        return "save";
    }
    
    // Most configuration items use "add"
    if (menu_path.find("/interface") != std::string::npos ||
        menu_path.find("/ip address") != std::string::npos ||
        menu_path.find("/ip route") != std::string::npos ||
        menu_path.find("/ip pool") != std::string::npos ||
        menu_path.find("/ip dhcp-server") != std::string::npos ||
        menu_path.find("/ip firewall") != std::string::npos ||
        menu_path.find("/routing") != std::string::npos ||
        menu_path.find("/system scheduler") != std::string::npos ||
        menu_path.find("/system script") != std::string::npos ||
        menu_path.find("/user") != std::string::npos) {
        return "add";
    }
    
    // Check for any specific config name indicators
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    
    if (lower_name.find("add") != std::string::npos) {
        return "add";
    } else if (lower_name.find("set") != std::string::npos) {
        return "set";
    } else if (lower_name.find("print") != std::string::npos) {
        return "print";
    } else if (lower_name.find("remove") != std::string::npos || lower_name.find("delete") != std::string::npos) {
        return "remove";
    }
    
    // Default to "add" for most configurations
    return "add";
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
    ss << "set " << name << "=";
    
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
    
    // Ensure valid interface name - skip if invalid
    if (name.empty() || name == ":") {
        ss << ident << "# Warning: Invalid interface name: \"" << name << "\"\n";
        return ss.str();
    }
    
    // Interface declarations in MikroTik use /interface path
    ss << ident << "# Interface: " << name << "\n";
    
    // Determine interface type and parse properties
    std::string iface_type = "ethernet"; // Default type
    std::string iface_name = name;
    
    // Properties to collect
    std::vector<std::string> properties;
    std::stringstream bridge_port_commands;
    std::stringstream ip_commands;
    std::stringstream other_commands;
    
    // Process statements to find interface type and properties
    for (const auto* statement : statements) {
        if (!statement) continue;
        
        if (const auto* prop_stmt = dynamic_cast<const PropertyStatement*>(statement)) {
            std::string prop_name = prop_stmt->get_name();
            std::string prop_value;
            
            // Get the raw value
            if (prop_stmt->get_value()) {
                prop_value = prop_stmt->get_value()->to_mikrotik("");
                // Remove quotes if present
                if (prop_value.size() >= 2 && prop_value.front() == '"' && prop_value.back() == '"') {
                    prop_value = prop_value.substr(1, prop_value.size() - 2);
                }
            }
            
            // Handle specific properties
            if (prop_name == "type") {
                iface_type = prop_value;
            }
            else if (prop_name == "description") {
                // Map description to comment
                properties.push_back("comment=\"" + prop_value + "\"");
            }
            else if (prop_name == "admin_state") {
                // Map admin_state to disabled (inverted logic)
                bool disabled = (prop_value != "up" && prop_value != "enabled");
                properties.push_back("disabled=" + std::string(disabled ? "yes" : "no"));
            }
            else {
                // Other properties pass through as-is
                properties.push_back(prop_name + "=\"" + prop_value + "\"");
            }
        }
        else if (const auto* section_stmt = dynamic_cast<const SectionStatement*>(statement)) {
            // Handle nested sections
            std::string section_name = section_stmt->get_name();
            
            if (section_name == "ip") {
                // Process IP configuration for this interface
                if (section_stmt->get_block()) {
                    for (const auto* ip_stmt : section_stmt->get_block()->get_statements()) {
                        if (const auto* ip_prop = dynamic_cast<const PropertyStatement*>(ip_stmt)) {
                            if (ip_prop->get_name() == "address" && ip_prop->get_value()) {
                                std::string ip_value = ip_prop->get_value()->to_mikrotik("");
                                // Remove quotes if present
                                if (ip_value.size() >= 2 && ip_value.front() == '"' && ip_value.back() == '"') {
                                    ip_value = ip_value.substr(1, ip_value.size() - 2);
                                }
                                
                                // Generate /ip address add command
                                ip_commands << ident << "/ip address add address=" 
                                          << ip_value << " interface=" 
                                          << iface_name << "\n";
                            }
                        }
                    }
                }
            }
            else if (section_name.find("port") != std::string::npos && iface_type == "bridge") {
                // Handle bridge port configuration
                bridge_port_commands << section_stmt->to_mikrotik(ident);
            }
            else {
                // Other sections
                other_commands << section_stmt->to_mikrotik(ident);
            }
        }
        else {
            // Other types of statements
            other_commands << statement->to_mikrotik(ident);
        }
    }
    
    // Generate the interface command with proper path based on type
    // Ethernet interfaces use 'set' instead of 'add'
    std::string command = "add";
    if (iface_type == "ethernet") {
        command = "set";
        
        // With 'set', interface name comes after command
        ss << ident << "/interface " << iface_type << " " << command << " " << iface_name;
    }
    else {
        // With 'add', name is a parameter
        ss << ident << "/interface " << iface_type << " " << command << " name=\"" << iface_name << "\"";
    }
    
    // Add all the properties
    for (const auto& prop : properties) {
        ss << " " << prop;
    }
    ss << "\n";
    
    // Add IP configuration commands
    ss << ip_commands.str();
    
    // Add bridge port commands if this is a bridge
    if (iface_type == "bridge") {
        ss << bridge_port_commands.str();
    }
    
    // Add other commands
    ss << other_commands.str();
    
    return ss.str();
}

// ProgramDeclaration implementation
ProgramDeclaration::ProgramDeclaration() noexcept 
    : Declaration("program"), sections() {}

void ProgramDeclaration::add_section(SectionStatement* section) noexcept 
{
    if (section) {
        sections.push_back(section);
        
        // Set parent for any sub-sections in the block
        if (section->get_block()) {
            for (auto* stmt : section->get_block()->get_statements()) {
                if (auto* sub_section = dynamic_cast<SectionStatement*>(stmt)) {
                    sub_section->set_parent(section);
                }
            }
        }
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
    
    
    // Process all top-level sections
    for (const auto* section : sections) {
        if (section) {
            ss << section->to_mikrotik(ident + "    ");
        }
    }
    
    
    return ss.str();
} 