#include "specialized_sections.hpp"

// SpecializedSection implementation
SpecializedSection::SpecializedSection(std::string_view name) noexcept
    : SectionStatement(name, SectionType::CUSTOM) // Temporarily set as CUSTOM, will be overridden
{
}

std::string SpecializedSection::to_mikrotik(const std::string& ident) const {
    // Common translation logic
    return translate_section(ident);
}

// DeviceSection implementation
DeviceSection::DeviceSection(std::string_view name) noexcept
    : SpecializedSection(name)
{
    // Override the section type
    this->type = SectionType::DEVICE;
}

bool DeviceSection::validate() const noexcept {
    // Validate device section - requires vendor, model, and hostname
    const BlockStatement* block = get_block();
    if (!block) return false;
    
    bool has_vendor = false;
    bool has_model = false;
    bool has_hostname = false;
    
    // Check if required properties exist
    for (const Statement* stmt : block->get_statements()) {
        const PropertyStatement* prop = dynamic_cast<const PropertyStatement*>(stmt);
        if (prop) {
            const std::string& name = prop->get_name();
            Expression* expr = prop->get_value();
            
            if (name == "vendor" && expr) {
                const StringValue* value = dynamic_cast<const StringValue*>(expr);
                if (value) has_vendor = true;
            }
            else if (name == "model" && expr) {
                const StringValue* value = dynamic_cast<const StringValue*>(expr);
                if (value) has_model = true;
            }
            else if (name == "hostname" && expr) {
                const StringValue* value = dynamic_cast<const StringValue*>(expr);
                if (value) has_hostname = true;
            }
        }
    }
    
    // Ensure all required properties are present
    return has_vendor && has_model && has_hostname;
}

std::string DeviceSection::translate_section(const std::string& ident) const {
    std::string result = "# Device Configuration\n";
    
    if (get_block()) {
        // Extract device properties
        std::string vendor = "";
        std::string model = "";
        std::string hostname = "";
        
        // Iterate through statements to find vendor, model, and hostname properties
        const BlockStatement* block = get_block();
        for (const Statement* stmt : block->get_statements()) {
            const PropertyStatement* prop = dynamic_cast<const PropertyStatement*>(stmt);
            if (prop) {
                const std::string& name = prop->get_name();
                Expression* expr = prop->get_value();
                
                if (name == "vendor" && expr) {
                    const StringValue* value = dynamic_cast<const StringValue*>(expr);
                    if (value) {
                        // Remove any quotes from the string value
                        vendor = value->get_value();
                        if (vendor.size() >= 2 && vendor.front() == '"' && vendor.back() == '"') {
                            vendor = vendor.substr(1, vendor.size() - 2);
                        }
                    }
                }
                else if (name == "model" && expr) {
                    const StringValue* value = dynamic_cast<const StringValue*>(expr);
                    if (value) {
                        // Remove any quotes from the string value
                        model = value->get_value();
                        if (model.size() >= 2 && model.front() == '"' && model.back() == '"') {
                            model = model.substr(1, model.size() - 2);
                        }
                    }
                }
                else if (name == "hostname" && expr) {
                    const StringValue* value = dynamic_cast<const StringValue*>(expr);
                    if (value) {
                        // Remove any quotes from the string value
                        hostname = value->get_value();
                        if (hostname.size() >= 2 && hostname.front() == '"' && hostname.back() == '"') {
                            hostname = hostname.substr(1, hostname.size() - 2);
                        }
                    }
                }
            }
        }
        
        // Create the combined name: vendor_hostname_model
        std::string combined_name = "";
        if (!vendor.empty()) {
            combined_name += vendor;
            if (!hostname.empty() || !model.empty()) combined_name += "_";
        }
        
        if (!hostname.empty()) {
            combined_name += hostname;
            if (!model.empty()) combined_name += "_";
        }
        
        if (!model.empty()) {
            combined_name += model;
        }
        
        // If we couldn't extract the values, use a default name
        if (combined_name.empty()) {
            combined_name = "router";
        }
        
        // Generate the MikroTik script
        result += "/system identity set name=\"" + combined_name + "\"\n";
    }
    
    return result;
}


// InterfacesSection implementation
InterfacesSection::InterfacesSection(std::string_view name) noexcept
    : SpecializedSection(name)
{
    this->type = SectionType::INTERFACES;
    
    // Add debug to print constructor initialization
    fprintf(stderr, "DEBUG: InterfacesSection constructor called with name '%s'\n", std::string(name).c_str());
}

bool InterfacesSection::validate() const noexcept {
    const BlockStatement* block = get_block();
    if (!block) return false;
    
    bool has_name = false;
    bool has_type = false;
    std::string interface_type = "";
    
    // Check for required properties
    for (const Statement* stmt : block->get_statements()) {
        const PropertyStatement* prop = dynamic_cast<const PropertyStatement*>(stmt);
        if (prop) {
            const std::string& name = prop->get_name();
            Expression* expr = prop->get_value();
            
            if (name == "name" && expr) {
                has_name = true;
            } else if (name == "type" && expr) {
                has_type = true;
                
                const StringValue* type_value = dynamic_cast<const StringValue*>(expr);
                if (type_value) {
                    interface_type = type_value->get_value();
                    // Remove quotes if present
                    if (interface_type.size() >= 2 && interface_type.front() == '"' && interface_type.back() == '"') {
                        interface_type = interface_type.substr(1, interface_type.size() - 2);
                    }
                }
            }
        }
    }
    
    // All interfaces should have a name
    if (!has_name) return false;
    
    // For VLAN, check if parent interface and VLAN ID exist
    if (interface_type == "vlan") {
        bool has_vlan_id = false;
        bool has_parent = false;
        
        for (const Statement* stmt : block->get_statements()) {
            const PropertyStatement* prop = dynamic_cast<const PropertyStatement*>(stmt);
            if (prop) {
                const std::string& name = prop->get_name();
                Expression* expr = prop->get_value();
                
                if (name == "vlan_id" && expr) {
                    has_vlan_id = true;
                } else if (name == "interface" && expr) {
                    has_parent = true;
                }
            }
        }
        
        if (!has_vlan_id || !has_parent) return false;
    }
    
    // For bonding, check if mode and slaves are set
    if (interface_type == "bonding") {
        bool has_mode = false;
        bool has_slaves = false;
        
        for (const Statement* stmt : block->get_statements()) {
            const PropertyStatement* prop = dynamic_cast<const PropertyStatement*>(stmt);
            if (prop) {
                const std::string& name = prop->get_name();
                Expression* expr = prop->get_value();
                
                if (name == "mode" && expr) {
                    has_mode = true;
                } else if (name == "slaves" && expr) {
                    has_slaves = true;
                }
            }
        }
        
        if (!has_mode || !has_slaves) return false;
    }
    
    return true;
}

std::string InterfacesSection::translate_section(const std::string& ident) const {
    std::string result = "# Interface Configuration\n";
    
    // TRULY GENERAL SOLUTION: Find and process ALL interface sections in the entire program
    // We need a reference to the program's top-level sections to access all parsed sections
    
    // Process subsections in our block - these are "officially" inside the interfaces: section
    if (get_block()) {
        const BlockStatement* block = get_block();
        fprintf(stderr, "DEBUG: InterfacesSection has %zu statements in block\n", block->get_statements().size());
        
        // 1. First approach - look for subsections within our block (normal case)
        for (const Statement* stmt : block->get_statements()) {
            if (const SectionStatement* section = dynamic_cast<const SectionStatement*>(stmt)) {
                std::string interface_name = section->get_name();
                fprintf(stderr, "DEBUG: Processing interface subsection '%s' from within interfaces block\n", interface_name.c_str());
                
                // Clean up interface name
                if (!interface_name.empty() && interface_name.back() == ':') {
                    interface_name = interface_name.substr(0, interface_name.size() - 1);
                }
                
                // If we somehow still have a colon in the name at this point, try to extract the name part
                size_t colon_pos = interface_name.find(':');
                if (colon_pos != std::string::npos) {
                    interface_name = interface_name.substr(0, colon_pos);
                }
                
                // Ensure interface name is valid
                if (interface_name.empty()) {
                    fprintf(stderr, "ERROR: Empty interface name found, section name='%s'\n", section->get_name().c_str());
                    continue; // Skip invalid interface names
                }
                
                // Process this interface using our helper
                result += process_interface_section(section, interface_name);
            }
        }
    }
    return result;
}

// Add helper method to process a single interface section
std::string InterfacesSection::process_interface_section(const SectionStatement* section, const std::string& interface_name) const {
    std::string result = "";
    
    if (!section || !section->get_block()) {
        return result;
    }
    
    const BlockStatement* interface_block = section->get_block();
    
    // Extract interface properties
    std::string type = "";
    std::string mtu = "";
    std::string disabled = "";
    std::string mac_address = "";
    std::string comment = "";
    std::string description = "";
    std::string vlan_id = "";
    std::string parent_interface = "";
    std::map<std::string, std::string> other_props;
    
    // Process all properties in the interface section
    for (const Statement* prop_stmt : interface_block->get_statements()) {
        const PropertyStatement* prop = dynamic_cast<const PropertyStatement*>(prop_stmt);
        if (prop) {
            const std::string& prop_name = prop->get_name();
            Expression* expr = prop->get_value();
            
            // Extract string value if possible
            std::string value = "";
            if (const StringValue* str_val = dynamic_cast<const StringValue*>(expr)) {
                value = str_val->get_value();
                // Remove quotes if present
                if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                    value = value.substr(1, value.size() - 2);
                }
            } else if (const NumberValue* num_val = dynamic_cast<const NumberValue*>(expr)) {
                value = std::to_string(num_val->get_value());
            } else if (const BooleanValue* bool_val = dynamic_cast<const BooleanValue*>(expr)) {
                value = bool_val->get_value() ? "yes" : "no";
            }
            
            // Store property values
            if (prop_name == "type") {
                type = value;
            } else if (prop_name == "mtu") {
                mtu = value;
            } else if (prop_name == "disabled" || prop_name == "admin_state") {
                // Map admin_state to disabled 
                if (value == "enabled") {
                    disabled = "no";  // not disabled = enabled
                } else if (value == "disabled") {
                    disabled = "yes"; // disabled = yes
                } else {
                    disabled = value; // use as-is if not a recognized value
                }
            } else if (prop_name == "mac_address" || prop_name == "mac") {
                mac_address = value;
            } else if (prop_name == "comment") {
                comment = value;
            } else if (prop_name == "description") {
                description = value;
            } else if (prop_name == "vlan_id") {
                vlan_id = value;
            } else if (prop_name == "interface") {
                parent_interface = value;
            } else {
                other_props[prop_name] = value;
            }
        }
    }
    
    // If description is set but comment is not, use description as comment
    if (comment.empty() && !description.empty()) {
        comment = description;
    }
    
    // Detect interface type if not explicitly specified
    if (type.empty()) {
        if (interface_name.find("ether") == 0) {
            type = "ethernet";
        } else if (interface_name.find("bridge") == 0) {
            type = "bridge";
        } else if (interface_name.find("vlan") == 0) {
            type = "vlan";
        } else if (interface_name.find("bond") == 0) {
            type = "bonding";
        } else if (interface_name.find("loop") == 0) {
            type = "loopback";
        }
    }
    
    fprintf(stderr, "DEBUG: Generating command for interface '%s' with type '%s'\n", 
            interface_name.c_str(), type.c_str());
    
    // Generate commands based on interface type
    if (type == "ethernet") {
        result += "/interface set ethernet " + interface_name;
        if (!mtu.empty()) result += " mtu=" + mtu;
        if (!disabled.empty()) result += " disabled=" + disabled;
        if (!mac_address.empty()) result += " mac-address=" + mac_address;
        if (!comment.empty()) result += " comment=\"" + comment + "\"";
        
        // Add other ethernet-specific properties
        if (other_props.count("advertise")) 
            result += " advertise=" + other_props["advertise"];
        if (other_props.count("arp")) 
            result += " arp=" + other_props["arp"];
        
        result += "\n";
    } else if (type == "vlan") {
        result += "/interface vlan add";
        result += " name=" + interface_name;
        if (!vlan_id.empty()) result += " vlan-id=" + vlan_id;
        if (!parent_interface.empty()) result += " interface=" + parent_interface;
        if (!disabled.empty()) result += " disabled=" + disabled;
        if (!mtu.empty()) result += " mtu=" + mtu;
        if (!comment.empty()) result += " comment=\"" + comment + "\"";
        result += "\n";
    } else if (type == "bridge") {
        result += "/interface bridge add";
        result += " name=" + interface_name;
        if (!disabled.empty()) result += " disabled=" + disabled;
        if (!mtu.empty()) result += " mtu=" + mtu;
        if (!comment.empty()) result += " comment=\"" + comment + "\"";
        
        // Add other bridge-specific properties
        if (other_props.count("protocol-mode")) 
            result += " protocol-mode=" + other_props["protocol-mode"];
        if (other_props.count("fast-forward")) 
            result += " fast-forward=" + other_props["fast-forward"];
        
        result += "\n";
        
        // Add bridge ports if specified
        if (other_props.count("ports")) {
            std::string ports = other_props["ports"];
            // Remove brackets if present (assuming list format)
            if (ports.size() >= 2 && ports.front() == '[' && ports.back() == ']') {
                ports = ports.substr(1, ports.size() - 2);
            }
            
            // Split ports by comma
            size_t pos = 0;
            std::string port;
            while ((pos = ports.find(',')) != std::string::npos) {
                port = ports.substr(0, pos);
                if (!port.empty()) {
                    // Trim whitespace
                    port.erase(0, port.find_first_not_of(" \t"));
                    port.erase(port.find_last_not_of(" \t") + 1);
                    
                    result += "/interface bridge port add bridge=" + interface_name + " interface=" + port + "\n";
                }
                ports.erase(0, pos + 1);
            }
            
            // Add the last port
            if (!ports.empty()) {
                ports.erase(0, ports.find_first_not_of(" \t"));
                ports.erase(ports.find_last_not_of(" \t") + 1);
                
                result += "/interface bridge port add bridge=" + interface_name + " interface=" + ports + "\n";
            }
        }
    } else if (type == "loopback") {
        result += "/interface add name=" + interface_name + " type=loopback";
        if (!disabled.empty()) result += " disabled=" + disabled;
        if (!comment.empty()) result += " comment=\"" + comment + "\"";
        result += "\n";
    } else if (type == "bonding") {
        result += "/interface bonding add";
        result += " name=" + interface_name;
        if (!disabled.empty()) result += " disabled=" + disabled;
        if (!mtu.empty()) result += " mtu=" + mtu;
        if (!comment.empty()) result += " comment=\"" + comment + "\"";
        
        // Add other bonding-specific properties
        if (other_props.count("mode")) 
            result += " mode=" + other_props["mode"];
        if (other_props.count("slaves")) 
            result += " slaves=" + other_props["slaves"];
        
        result += "\n";
    } else {
        // Generic interface command
        result += "/interface set " + interface_name;
        if (!disabled.empty()) result += " disabled=" + disabled;
        if (!mtu.empty()) result += " mtu=" + mtu;
        if (!comment.empty()) result += " comment=\"" + comment + "\"";
        result += "\n";
    }
    
    // Add interface lists if specified
    if (other_props.count("lists")) {
        std::string lists = other_props["lists"];
        // Similar processing as for bridge ports
        if (lists.size() >= 2 && lists.front() == '[' && lists.back() == ']') {
            lists = lists.substr(1, lists.size() - 2);
        }
        
        size_t pos = 0;
        std::string list;
        while ((pos = lists.find(',')) != std::string::npos) {
            list = lists.substr(0, pos);
            if (!list.empty()) {
                list.erase(0, list.find_first_not_of(" \t"));
                list.erase(list.find_last_not_of(" \t") + 1);
                
                result += "/interface list member add list=" + list + " interface=" + interface_name + "\n";
            }
            lists.erase(0, pos + 1);
        }
        
        if (!lists.empty()) {
            lists.erase(0, lists.find_first_not_of(" \t"));
            lists.erase(lists.find_last_not_of(" \t") + 1);
            
            result += "/interface list member add list=" + lists + " interface=" + interface_name + "\n";
        }
    }
    
    return result;
}

// IPSection implementation
IPSection::IPSection(std::string_view name) noexcept
    : SpecializedSection(name)
{
    this->type = SectionType::IP;
}

bool IPSection::validate() const noexcept {
    const BlockStatement* block = get_block();
    if (!block) return false;
    
    // Add specific IP section validation logic here
    return true;
}

std::string IPSection::translate_section(const std::string& ident) const {
    std::string result = ident + "# IP Configuration: " + get_name() + "\n";
    
    if (get_block()) {
        // IP-specific translation
        result += ident + "/ip\n";
        result += get_block()->to_mikrotik(ident + "  ");
    }
    
    return result;
}

// RoutingSection implementation
RoutingSection::RoutingSection(std::string_view name) noexcept
    : SpecializedSection(name)
{
    this->type = SectionType::ROUTING;
}

bool RoutingSection::validate() const noexcept {
    const BlockStatement* block = get_block();
    if (!block) return false;
    
    // Add specific routing section validation logic here
    return true;
}

std::string RoutingSection::translate_section(const std::string& ident) const {
    std::string result = ident + "# Routing Configuration: " + get_name() + "\n";
    
    if (get_block()) {
        // Routing-specific translation
        result += ident + "/routing\n";
        result += get_block()->to_mikrotik(ident + "  ");
    }
    
    return result;
}

// FirewallSection implementation
FirewallSection::FirewallSection(std::string_view name) noexcept
    : SpecializedSection(name)
{
    this->type = SectionType::FIREWALL;
}

bool FirewallSection::validate() const noexcept {
    const BlockStatement* block = get_block();
    if (!block) return false;
    
    // Add specific firewall section validation logic here
    return true;
}

std::string FirewallSection::translate_section(const std::string& ident) const {
    std::string result = ident + "# Firewall Configuration: " + get_name() + "\n";
    
    if (get_block()) {
        // Firewall-specific translation
        result += ident + "/ip firewall\n";
        result += get_block()->to_mikrotik(ident + "  ");
    }
    
    return result;
}

// SystemSection implementation
SystemSection::SystemSection(std::string_view name) noexcept
    : SpecializedSection(name)
{
    this->type = SectionType::SYSTEM;
}

bool SystemSection::validate() const noexcept {
    const BlockStatement* block = get_block();
    if (!block) return false;
    
    // Add specific system section validation logic here
    return true;
}

std::string SystemSection::translate_section(const std::string& ident) const {
    std::string result = ident + "# System Configuration: " + get_name() + "\n";
    
    if (get_block()) {
        // System-specific translation
        result += ident + "/system\n";
        result += get_block()->to_mikrotik(ident + "  ");
    }
    
    return result;
}

// CustomSection implementation
CustomSection::CustomSection(std::string_view name) noexcept
    : SpecializedSection(name)
{
    this->type = SectionType::CUSTOM;
}

bool CustomSection::validate() const noexcept {
    const BlockStatement* block = get_block();
    if (!block) return false;
    
    // Custom sections are more permissive
    return true;
}

std::string CustomSection::translate_section(const std::string& ident) const {
    std::string result = ident + "# Custom Configuration: " + get_name() + "\n";
    
    if (get_block()) {
        // For custom sections, simply translate the block
        result += get_block()->to_mikrotik(ident);
    }
    
    return result;
}

// Factory function implementation
SpecializedSection* create_specialized_section(std::string_view name, SectionStatement::SectionType type) {
    switch (type) {
        case SectionStatement::SectionType::DEVICE:
            return new DeviceSection(name);
        case SectionStatement::SectionType::INTERFACES:
            return new InterfacesSection(name);
        case SectionStatement::SectionType::IP:
            return new IPSection(name);
        case SectionStatement::SectionType::ROUTING:
            return new RoutingSection(name);
        case SectionStatement::SectionType::FIREWALL:
            return new FirewallSection(name);
        case SectionStatement::SectionType::SYSTEM:
            return new SystemSection(name);
        case SectionStatement::SectionType::CUSTOM:
        default:
            return new CustomSection(name);
    }
} 