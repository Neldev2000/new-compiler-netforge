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
    ss << ident << "# Configuration section: " << name << "\n";
    
    // Determine the MikroTik base path from the configuration name
    std::string menu_path;
    
    // Convert name to lowercase for case-insensitive comparison
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    
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
    
    // Interface declarations in MikroTik use /interface path
    ss << ident << "# Interface: " << name << "\n";
    
    // Mejorar detección del tipo de interfaz a partir del nombre
    std::string iface_type = "ethernet"; // Default type
    std::string iface_name = name;
    
    // Detección mejorada de interfaces por patrón de nombre
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    
    if (lower_name.find("bridge") != std::string::npos) {
        iface_type = "bridge";
    } else if (lower_name.find("vlan") != std::string::npos) {
        iface_type = "vlan";
    } else if (lower_name.find("wlan") != std::string::npos || lower_name.find("wifi") != std::string::npos) {
        iface_type = "wireless";
    } else if (lower_name.find("bond") != std::string::npos) {
        iface_type = "bonding";
    } else if (lower_name.find("ppp") != std::string::npos || lower_name.find("ovpn") != std::string::npos) {
        iface_type = "pppoe-client";
        if (lower_name.find("ovpn") != std::string::npos || lower_name.find("vpn") != std::string::npos) {
            iface_type = "ovpn-client";
        }
    } else if (lower_name.find("veth") != std::string::npos) {
        iface_type = "veth";
    } else if (lower_name.find("vrrp") != std::string::npos) {
        iface_type = "vrrp";
    } else if (lower_name.find("list") != std::string::npos) {
        iface_type = "list";
    } else {
        // El enfoque original: intentar determinar por prefijo numérico
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
            } else if (iface_type == "vlan") {
                iface_type = "vlan";
            } else if (iface_type == "lo") {
                iface_type = "loopback";
            }
        }
    }
    
    // Clasificar las propiedades para el comando add vs set
    std::vector<std::string> add_properties;
    std::vector<std::pair<std::string, std::string>> set_properties; // Par de (propiedad, valor)
    std::stringstream bridge_port_commands;
    std::stringstream other_commands;
    
    // Propiedades comunes para el comando add inicial
    std::set<std::string> add_compatible_props = {
        "name", "type", "mtu", "mac-address", "comment", "disabled", "vlan-id", "interface", 
        "master-port", "vlan-mode", "protocol-mode"
    };
    
    // Propiedades específicas por tipo de interfaz
    if (iface_type == "bridge") {
        add_compatible_props.insert({"protocol-mode", "priority", "auto-mac", "ageing-time"});
    } else if (iface_type == "wireless") {
        add_compatible_props.insert({"ssid", "mode", "frequency", "band", "channel-width"});
    } else if (iface_type == "vlan") {
        add_compatible_props.insert({"vlan-id", "interface"});
    }
    
    // Recopilar propiedades y sub-configuraciones
    for (const auto* statement : statements) {
        if (!statement) continue;
        
        if (const auto* prop_stmt = dynamic_cast<const PropertyStatement*>(statement)) {
            std::string prop_name = prop_stmt->get_name();
            std::string prop_value = prop_stmt->to_mikrotik("");
            
            // Manejar propiedades especiales
            if (prop_name == "name") {
                iface_name = prop_value.substr(prop_value.find('=') + 1); // Extraer el valor después de =
                // Limpiar comillas si las hay
                if (iface_name.front() == '"' && iface_name.back() == '"') {
                    iface_name = iface_name.substr(1, iface_name.size() - 2);
                }
                add_properties.push_back(prop_value);
            } else if (prop_name == "type") {
                iface_type = prop_value.substr(prop_value.find('=') + 1);
                // Limpiar comillas si las hay
                if (iface_type.front() == '"' && iface_type.back() == '"') {
                    iface_type = iface_type.substr(1, iface_type.size() - 2);
                }
                add_properties.push_back(prop_value);
            } else if (add_compatible_props.find(prop_name) != add_compatible_props.end()) {
                add_properties.push_back(prop_value);
            } else {
                // Es una propiedad que debe usar comando set
                set_properties.push_back({prop_name, prop_value});
            }
        } else if (const auto* config_decl = dynamic_cast<const ConfigDeclaration*>(statement)) {
            // Manejar sub-configuraciones
            if (iface_type == "bridge" && config_decl->get_name().find("port") != std::string::npos) {
                // Bridge ports sub-configuration
                bridge_port_commands << statement->to_mikrotik(ident);
            } else {
                // Other interface-specific configurations
                other_commands << statement->to_mikrotik(ident);
            }
        } else {
            // Otro tipo de declaración o statement
            other_commands << statement->to_mikrotik(ident);
        }
    }
    
    // Generar el comando add principal
    ss << ident << "/interface " << iface_type << " add";
    
    // Asegurar que la propiedad name siempre esté presente
    bool has_name = false;
    for (const auto& prop : add_properties) {
        if (prop.find("name=") == 0) {
            has_name = true;
            break;
        }
    }
    
    if (!has_name) {
        ss << " name=" << "\"" << iface_name << "\"";
    }
    
    // Añadir todas las propiedades compatibles con add
    for (const auto& prop : add_properties) {
        // Evitar duplicar name si ya lo agregamos
        if (!has_name || prop.find("name=") != 0) {
            ss << " " << prop;
        }
    }
    ss << "\n";
    
    // Generar comandos set separados para propiedades que requieren set
    if (!set_properties.empty()) {
        for (const auto& [prop_name, prop_value] : set_properties) {
            ss << ident << "/interface " << iface_type << " set [find name=\"" << iface_name << "\"] " 
               << prop_value << "\n";
        }
    }
    
    // Añadir comandos bridge port si existen
    if (bridge_port_commands.str().length() > 0) {
        ss << bridge_port_commands.str();
    }
    
    // Añadir otros comandos relacionados con la interfaz
    if (other_commands.str().length() > 0) {
        ss << other_commands.str();
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
    
    
    // Process all top-level sections
    for (const auto* section : sections) {
        if (section) {
            ss << section->to_mikrotik(ident + "    ");
        }
    }
    
    
    return ss.str();
} 