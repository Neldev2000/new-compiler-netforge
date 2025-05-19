#include "semantic_validator.hpp"
#include "specialized_sections.hpp"
#include <regex>

// Base SectionValidator implementation
SectionValidator::SectionValidator(std::string section_name, NestingRule nesting_rule)
    : section_name_(std::move(section_name)), nesting_rule_(nesting_rule) {}

std::string SectionValidator::getSectionName() const {
    return section_name_;
}

std::tuple<bool, std::string> SectionValidator::validate(const BlockStatement* block) const {
    if (!block) {
        return std::make_tuple(false, section_name_ + " section is missing a block statement");
    }
    
    // First validate the overall hierarchy
    auto hierarchy_result = validateHierarchy(block);
    if (!std::get<0>(hierarchy_result)) {
        return hierarchy_result;
    }
    
    // Then validate individual properties for each subsection
    for (const Statement* stmt : block->get_statements()) {
        const SectionStatement* subsection = dynamic_cast<const SectionStatement*>(stmt);
        
        if (subsection) {
            auto props_result = validateProperties(subsection);
            if (!std::get<0>(props_result)) {
                return props_result;
            }
        }
    }
    
    return std::make_tuple(true, "");
}

std::tuple<bool, std::string> SectionValidator::validateHierarchy(const BlockStatement* block) const {
    // If nesting is fully allowed, nothing to check
    if (nesting_rule_ == NestingRule::DEEP_NESTING) {
        return std::make_tuple(true, "");
    }
    
    // Keep track of top-level sections
    std::set<std::string> top_level_sections;
    
    for (const Statement* stmt : block->get_statements()) {
        const SectionStatement* subsection = dynamic_cast<const SectionStatement*>(stmt);
        
        if (subsection) {
            const std::string& subsection_name = subsection->get_name();
            top_level_sections.insert(subsection_name);
            
            // If nesting is completely disallowed, check there are no nested sections
            if (nesting_rule_ == NestingRule::NO_NESTING) {
                const BlockStatement* sub_block = subsection->get_block();
                if (sub_block) {
                    for (const Statement* nested_stmt : sub_block->get_statements()) {
                        if (dynamic_cast<const SectionStatement*>(nested_stmt)) {
                            return std::make_tuple(false, 
                                "Semantic error: Section '" + subsection_name + 
                                "' cannot contain nested sections in " + section_name_ + " section");
                        }
                    }
                }
            }
            // For shallow nesting or conditional nesting, check each nested section
            else if (nesting_rule_ == NestingRule::SHALLOW_NESTING || 
                     nesting_rule_ == NestingRule::CONDITIONAL_NESTING) {
                const BlockStatement* sub_block = subsection->get_block();
                if (sub_block) {
                    for (const Statement* nested_stmt : sub_block->get_statements()) {
                        const SectionStatement* nested_section = 
                            dynamic_cast<const SectionStatement*>(nested_stmt);
                        
                        if (nested_section) {
                            const std::string& nested_name = nested_section->get_name();
                            
                            // For conditional nesting, check the condition
                            if (nesting_rule_ == NestingRule::CONDITIONAL_NESTING && 
                                !isValidNesting(subsection_name, nested_name)) {
                                return std::make_tuple(false, 
                                    "Semantic error: Section '" + nested_name + 
                                    "' cannot be defined under '" + subsection_name + 
                                    "' in " + section_name_ + " section");
                            }
                            
                            // For shallow nesting, make sure there are no deeper nestings
                            if (nesting_rule_ == NestingRule::SHALLOW_NESTING) {
                                const BlockStatement* nested_block = nested_section->get_block();
                                if (nested_block) {
                                    for (const Statement* deep_stmt : nested_block->get_statements()) {
                                        if (dynamic_cast<const SectionStatement*>(deep_stmt)) {
                                            return std::make_tuple(false, 
                                                "Semantic error: Nesting depth exceeded in " + 
                                                section_name_ + " section (max 2 levels)");
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    return std::make_tuple(true, "");
}

bool SectionValidator::isValidNesting(const std::string& parent_name, 
                                     const std::string& child_name) const {
    // Default implementation: no special nesting rules
    return true;
}

DeviceValidator::DeviceValidator()
    : SectionValidator("device", NestingRule::DEEP_NESTING) {}

std::tuple<bool, std::string> DeviceValidator::validateProperties(
    const SectionStatement* section) const {
     bool has_vendor = false;
    bool has_model = false;
    bool has_hostname = false;
        const PropertyStatement* prop = dynamic_cast<const PropertyStatement*>(section);
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
            else {
                // Invalid property found - only hostname, vendor, and model are allowed
                return {false, "Device section contains invalid property: " + name + 
                              ". Only 'hostname', 'vendor', and 'model' are allowed"};
            }
        }
        else {
            // Non-property statement found in device section
            return {false, "Device section contains an invalid statement type. Only property statements are allowed"};
        }
    
    if (!has_vendor) 
        return {false, "Device section is missing required 'vendor' property"};
    if (!has_model) 
        return {false, "Device section is missing required 'model' property"};
    if (!has_hostname) 
        return {false, "Device section is missing required 'hostname' property"};

    return std::make_tuple(true, "");
}

InterfacesValidator::InterfacesValidator()
    : SectionValidator("interfaces", NestingRule::CONDITIONAL_NESTING) {
    // Initialize the sets of valid properties for different interface types
    common_valid_props_ = {
        "type", "mtu", "disabled", "admin_state", "mac_address", "mac", 
        "comment", "description", "lists", "arp"
    };
    
    vlan_specific_props_ = {
        "vlan_id", "interface"
    };
    
    bonding_specific_props_ = {
        "mode", "slaves"
    };
    
    bridge_specific_props_ = {
        "protocol-mode", "fast-forward", "ports"
    };
    
    ethernet_specific_props_ = {
        "advertise", "auto-negotiation", "speed", "duplex"
    };
}

std::tuple<bool, std::string> InterfacesValidator::validateProperties(
    const SectionStatement* section) const {
    
    bool has_type = false;
    std::string interface_type = "";
    const BlockStatement* block = section->get_block();
    
    if (!block) {
        return std::make_tuple(false, "Interface section '" + section->get_name() + "' is missing a block statement");
    }
    
    // Check for required properties and validate all properties
    for (const Statement* stmt : block->get_statements()) {
        const PropertyStatement* prop = dynamic_cast<const PropertyStatement*>(stmt);
        const SectionStatement* subsection = dynamic_cast<const SectionStatement*>(stmt);
        
        // Skip subsections as they are validated separately
        if (subsection) continue;
        
        // Non-property, non-section statement found (invalid)
        if (!prop && !subsection) {
            return std::make_tuple(false, "Interface section contains an invalid statement type");
        }
        
        // Process properties
        if (prop) {
            const std::string& name = prop->get_name();
            Expression* expr = prop->get_value();
            
            // Check if this is a common valid property
            if (common_valid_props_.find(name) != common_valid_props_.end()) {
                if (name == "type" && expr) {
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
            // Check for VLAN-specific properties
            else if (interface_type == "vlan" && vlan_specific_props_.find(name) != vlan_specific_props_.end()) {
                // Valid VLAN property
            }
            // Check for bonding-specific properties
            else if (interface_type == "bonding" && bonding_specific_props_.find(name) != bonding_specific_props_.end()) {
                // Valid bonding property
            } 
            // Check for bridge-specific properties
            else if (interface_type == "bridge" && bridge_specific_props_.find(name) != bridge_specific_props_.end()) {
                // Valid bridge property
            }
            // Check for ethernet-specific properties
            else if ((interface_type == "ethernet" || interface_type.empty()) && 
                    ethernet_specific_props_.find(name) != ethernet_specific_props_.end()) {
                // Valid ethernet property
            }
            // Invalid property found
            else {
                return std::make_tuple(false, "Interface section contains invalid property '" + name + 
                    "'. This property is not valid for interface configuration.");
            }
        }
    }
    
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
        
        if (!has_vlan_id) return std::make_tuple(false, "VLAN interface is missing required 'vlan_id' property");
        if (!has_parent) return std::make_tuple(false, "VLAN interface is missing required 'interface' property");
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
        
        if (!has_mode) return std::make_tuple(false, "Bonding interface is missing required 'mode' property");
        if (!has_slaves) return std::make_tuple(false, "Bonding interface is missing required 'slaves' property");
    }
    
    return std::make_tuple(true, "");
}

bool InterfacesValidator::isValidNesting(const std::string& parent_name, 
                                       const std::string& child_name) const {
    // Most interface types should not have nested interfaces
    // Exceptions: configuration groups, profiles, templates
    
    if (parent_name == "template" || parent_name == "group") {
        return true;
    }
    
    // By default, don't allow nesting for interfaces
    return false;
}

IPValidator::IPValidator()
    : SectionValidator("IP", NestingRule::CONDITIONAL_NESTING) {
}

std::tuple<bool, std::string> IPValidator::validateProperties(
    const SectionStatement* section) const {

    // Define regular expression for IPv4 validation
    // Format: xxx.xxx.xxx.xxx/xx where xxx is 0-255 and xx is 0-32
    std::regex ipv4_pattern("^((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])\\.){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])(\\/(3[0-2]|[1-2]?[0-9]))?$");
    
    // Define valid subsections in IP section
    const std::set<std::string> valid_subsections = {
        "address", "route", "firewall", "dhcp-server", "dhcp-client", 
        "dns", "arp", "service", "neighbor", "proxy"
    };
    
    // Define valid properties directly under IP section
    const std::set<std::string> valid_direct_props = {
        "dns-server", "allow-remote-requests"
    };
    
    std::string section_name = section->get_name();
    
    // Check if this is an interface subsection (for address assignment)
    bool is_interface_section = true;
    
    // Check if this is a known subsection type
    for (const auto& valid_name : valid_subsections) {
        if (section_name == valid_name) {
            is_interface_section = false;
            break;
        }
    }
    
    // Validate interface address assignments
    if (is_interface_section) {
        // This is likely an interface name - validate its properties
        const BlockStatement* block = section->get_block();
        if (!block) {
            return {false, "IP interface section '" + section_name + "' is missing its block"};
        }
        
        bool has_address = false;
        
        // Check properties
        for (const Statement* if_stmt : block->get_statements()) {
            // Skip nested sections as they're validated by hierarchy validation
            if (dynamic_cast<const SectionStatement*>(if_stmt)) {
                continue;
            }
            
            const PropertyStatement* prop = dynamic_cast<const PropertyStatement*>(if_stmt);
            if (prop) {
                const std::string& prop_name = prop->get_name();
                
                // Validate address property
                if (prop_name == "address") {
                    has_address = true;
                    
                    // Check if the value is a valid IP address
                    if (prop->get_value()) {
                        const StringValue* addr_value = dynamic_cast<const StringValue*>(prop->get_value());
                        if (addr_value) {
                            std::string ip_addr = addr_value->get_value();
                            // Remove quotes if present
                            if (ip_addr.size() >= 2 && ip_addr.front() == '"' && ip_addr.back() == '"') {
                                ip_addr = ip_addr.substr(1, ip_addr.size() - 2);
                            }
                            
                            // Validate IP address format using regex
                            if (!std::regex_match(ip_addr, ipv4_pattern)) {
                                return {false, "Invalid IP address format in interface '" + section_name + 
                                              "': " + ip_addr};
                            }
                        }
                    }
                } 
                else {
                    // Invalid property for interface IP section
                    return {false, "Invalid property '" + prop_name + "' in IP interface section '" + 
                                 section_name + "'. Only 'address' is allowed."};
                }
            }
            else {
                // Unknown statement type that is not a property or section
                return {false, "IP interface section contains an invalid statement type"};
            }
        }
        
        // Ensure address is specified
        if (!has_address) {
            return {false, "IP interface section '" + section_name + 
                          "' is missing required 'address' property"};
        }
    }
    // Validate route subsection
    else if (section_name == "route" || section_name == "routes") {
        const BlockStatement* block = section->get_block();
        if (!block) {
            return {false, "IP route section is missing its block"};
        }
        
        for (const Statement* route_stmt : block->get_statements()) {
            const SectionStatement* route_section = dynamic_cast<const SectionStatement*>(route_stmt);
            const PropertyStatement* route_prop = dynamic_cast<const PropertyStatement*>(route_stmt);
            
            // Default route is configured as a property
            if (route_prop && route_prop->get_name() == "default") {
                // Valid default route property
                continue;
            }
            
            // Specific route entries are sections
            if (route_section) {
                const BlockStatement* route_block = route_section->get_block();
                if (!route_block) {
                    return {false, "IP route entry '" + route_section->get_name() + "' is missing its block"};
                }
                
                bool has_gateway = false;
                
                for (const Statement* route_detail : route_block->get_statements()) {
                    const PropertyStatement* detail_prop = dynamic_cast<const PropertyStatement*>(route_detail);
                    if (detail_prop) {
                        if (detail_prop->get_name() == "gateway") {
                            has_gateway = true;
                            
                            // Validate gateway IP
                            if (detail_prop->get_value()) {
                                const StringValue* gw_value = dynamic_cast<const StringValue*>(detail_prop->get_value());
                                if (gw_value) {
                                    std::string gateway = gw_value->get_value();
                                    // Remove quotes if present
                                    if (gateway.size() >= 2 && gateway.front() == '"' && gateway.back() == '"') {
                                        gateway = gateway.substr(1, gateway.size() - 2);
                                    }
                                    
                                    // Validate gateway IP address format (without subnet)
                                    std::regex ip_only("^((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])\\.){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])$");
                                    if (!std::regex_match(gateway, ip_only)) {
                                        return {false, "Invalid gateway IP address format in route '" + 
                                                      route_section->get_name() + "': " + gateway};
                                    }
                                }
                            }
                        }
                    }
                }
                
                // All routes should have a gateway
                if (!has_gateway) {
                    return {false, "IP route entry '" + route_section->get_name() + 
                                  "' is missing required 'gateway' property"};
                }
            }
        }
    }
    // For direct properties under the IP section
    else if (!section->get_block()) { 
        // This would be a direct property statement
        const PropertyStatement* prop = dynamic_cast<const PropertyStatement*>(section);
        if (prop) {
            const std::string& prop_name = prop->get_name();
            
            // Check if it's a valid direct property
            if (valid_direct_props.find(prop_name) == valid_direct_props.end()) {
                return {false, "Invalid property '" + prop_name + "' directly under IP section"};
            }
        }
        else {
            // Unknown statement type
            return {false, "IP section contains an invalid statement type"};
        }
    }
    
    return {true, ""};
}

bool IPValidator::isValidNesting(const std::string& parent_name, 
                              const std::string& child_name) const {
    // Define valid subsections
    const std::set<std::string> valid_subsections = {
        "address", "route", "firewall", "dhcp-server", "dhcp-client", 
        "dns", "arp", "service", "neighbor", "proxy"
    };
    
    // Interface sections should not have nested interfaces
    bool is_parent_interface = true;
    for (const auto& valid_name : valid_subsections) {
        if (parent_name == valid_name) {
            is_parent_interface = false;
            break;
        }
    }
    
    if (is_parent_interface) {
        // Exception for template/group sections
        if (parent_name == "template" || parent_name == "group") {
            return true;
        }
        return false; // No nesting for interface sections
    }
    
    // Allow nesting for standard subsections like route, dns, etc.
    return true;
}

RoutingValidator::RoutingValidator()
    : SectionValidator("routing", NestingRule::CONDITIONAL_NESTING) {
}

std::tuple<bool, std::string> RoutingValidator::validateProperties(
    const SectionStatement* section) const {
    
    // Define regular expression for IPv4 validation
    std::regex ipv4_pattern("^((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])\\.){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])$");
    std::regex cidr_pattern("^((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])\\.){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])(\\/(3[0-2]|[1-2]?[0-9]))$");
    
    // Define valid routing section properties
    const std::set<std::string> valid_top_props = {
        "static_route_default_gw" // Default gateway property
    };
    
    // Define valid properties for static routes
    const std::set<std::string> valid_route_props = {
        "destination", "dst-address", "dst",       // Destination network
        "gateway", "gw",                          // Next hop
        "distance",                               // Administrative distance
        "routing-table", "table",                 // Routing table name
        "check-gateway",                          // Failover check method
        "scope",                                  // Route scope
        "target-scope",                           // Target scope
        "suppress-hw-offload"                     // Hardware offload control
    };
    
    // Define valid routing subsections
    const std::set<std::string> valid_subsections = {
        "table", "tables", "rule", "rules", "filter"
    };
    
    std::string section_name = section->get_name();
    
    // First, check if this is a direct property entry (top-level)
    const PropertyStatement* prop = dynamic_cast<const PropertyStatement*>(section);
    if (prop) {
        const std::string& name = prop->get_name();
        
        // Check if it's a valid top-level property
        if (valid_top_props.find(name) == valid_top_props.end()) {
            return {false, "Invalid property '" + name + "' in routing section. Top-level routing properties are limited."};
        }
        
        // Validate default gateway
        if (name == "static_route_default_gw") {
            // Validate gateway IP address
            if (prop->get_value()) {
                const StringValue* gw_value = dynamic_cast<const StringValue*>(prop->get_value());
                if (gw_value) {
                    std::string gateway = gw_value->get_value();
                    // Remove quotes if present
                    if (gateway.size() >= 2 && gateway.front() == '"' && gateway.back() == '"') {
                        gateway = gateway.substr(1, gateway.size() - 2);
                    }
                    
                    // Validate gateway format using regex
                    if (!std::regex_match(gateway, ipv4_pattern)) {
                        return {false, "Invalid default gateway IP address format: " + gateway};
                    }
                }
            }
        }
        
        return {true, ""};
    }
    
    // Check for standard subsections
    bool is_standard_subsection = false;
    for (const auto& valid_name : valid_subsections) {
        if (section_name == valid_name) {
            is_standard_subsection = true;
            break;
        }
    }
    
    // Table subsections validation
    if (section_name == "table" || section_name == "tables") {
        const BlockStatement* block = section->get_block();
        if (!block) {
            return {false, "Routing table section is missing its block"};
        }
        
        // Validation for table entries happens in isValidNesting
        
        return {true, ""};
    }
    
    // Rule subsections validation
    if (section_name == "rule" || section_name == "rules") {
        const BlockStatement* block = section->get_block();
        if (!block) {
            return {false, "Routing rule section is missing its block"};
        }
        
        // Validation for rule entries happens in isValidNesting
        
        return {true, ""};
    }
    
    // Check if this is a route definition (neither standard subsection nor direct property)
    if (!is_standard_subsection) {
        const BlockStatement* block = section->get_block();
        if (!block) {
            return {false, "Route entry '" + section_name + "' is missing its block"};
        }
        
        bool has_destination = false;
        bool has_gateway = false;
        
        // Validate route properties
        for (const Statement* route_stmt : block->get_statements()) {
            // Skip nested statements as they are validated by hierarchy validation
            if (dynamic_cast<const SectionStatement*>(route_stmt)) {
                continue;
            }
            
            const PropertyStatement* route_prop = dynamic_cast<const PropertyStatement*>(route_stmt);
            if (route_prop) {
                const std::string& prop_name = route_prop->get_name();
                
                // Check if this is a valid route property
                if (valid_route_props.find(prop_name) == valid_route_props.end()) {
                    return {false, "Invalid property '" + prop_name + "' in route '" + section_name + "'"};
                }
                
                // Validate destination
                if (prop_name == "destination" || prop_name == "dst-address" || prop_name == "dst") {
                    has_destination = true;
                    
                    // Validate destination format
                    if (route_prop->get_value()) {
                        const StringValue* dst_value = dynamic_cast<const StringValue*>(route_prop->get_value());
                        if (dst_value) {
                            std::string destination = dst_value->get_value();
                            // Remove quotes if present
                            if (destination.size() >= 2 && destination.front() == '"' && destination.back() == '"') {
                                destination = destination.substr(1, destination.size() - 2);
                            }
                            
                            // Validate CIDR format
                            if (!std::regex_match(destination, cidr_pattern)) {
                                return {false, "Invalid destination network format in route '" + 
                                              section_name + "': " + destination + 
                                              ". Must be in CIDR format (e.g. 192.168.1.0/24)"};
                            }
                        }
                    }
                }
                
                // Validate gateway
                if (prop_name == "gateway" || prop_name == "gw") {
                    has_gateway = true;
                    
                    // Validate gateway format
                    if (route_prop->get_value()) {
                        const StringValue* gw_value = dynamic_cast<const StringValue*>(route_prop->get_value());
                        if (gw_value) {
                            std::string gateway = gw_value->get_value();
                            // Remove quotes if present
                            if (gateway.size() >= 2 && gateway.front() == '"' && gateway.back() == '"') {
                                gateway = gateway.substr(1, gateway.size() - 2);
                            }
                            
                            // Allow interface names, IP addresses, or routing marks
                            if (!std::regex_match(gateway, ipv4_pattern) && 
                                gateway.find("ether") != 0 && 
                                gateway.find("wlan") != 0 &&
                                gateway.find("bridge") != 0) {
                                
                                // If not an IP or common interface, warn but don't fail
                                // (could be a custom interface name or routing mark)
                                // Consider adding more validation here if needed
                            }
                        }
                    }
                }
                
                // Validate distance
                if (prop_name == "distance") {
                    if (route_prop->get_value()) {
                        const NumberValue* distance_value = dynamic_cast<const NumberValue*>(route_prop->get_value());
                        if (!distance_value) {
                            return {false, "Distance property in route '" + section_name + 
                                          "' must be a number"};
                        }
                        
                        // Check distance range (1-255)
                        double distance = distance_value->get_value();
                        if (distance < 1 || distance > 255) {
                            return {false, "Distance value in route '" + section_name + 
                                          "' must be between 1 and 255"};
                        }
                    }
                }
            }
        }
        
        // All static routes should have both destination and gateway
        if (!has_destination) {
            return {false, "Route '" + section_name + "' is missing required 'destination/dst-address' property"};
        }
        
        if (!has_gateway) {
            return {false, "Route '" + section_name + "' is missing required 'gateway' property"};
        }
    }
    
    return {true, ""};
}

bool RoutingValidator::isValidNesting(const std::string& parent_name, 
                                    const std::string& child_name) const {
    // Define valid routing subsections
    const std::set<std::string> valid_subsections = {
        "table", "tables", "rule", "rules", "filter"
    };
    
    // Check if parent is a standard subsection
    bool is_standard_subsection = false;
    for (const auto& valid_name : valid_subsections) {
        if (parent_name == valid_name) {
            is_standard_subsection = true;
            break;
        }
    }
    
    // Exception for template/group sections
    if (parent_name == "template" || parent_name == "group") {
        return true;
    }
    
    // For table or rule subsections - generally no nesting allowed
    if (is_standard_subsection) {
        return false;
    }
    
    // For regular routes - no nesting allowed
    return false;
}

FirewallValidator::FirewallValidator()
    : SectionValidator("firewall", NestingRule::CONDITIONAL_NESTING) {
}

std::tuple<bool, std::string> FirewallValidator::validateProperties(
    const SectionStatement* section) const {
    
    // Define valid subsections in a firewall configuration
    const std::set<std::string> valid_subsections = {
        "filter", "nat", "mangle", "raw", "address-list", "service-port", "layer7-protocol"
    };
    
    // Define valid filter chains
    const std::set<std::string> valid_filter_chains = {
        "input", "forward", "output"
    };
    
    // Define valid NAT chains
    const std::set<std::string> valid_nat_chains = {
        "srcnat", "dstnat", "prerouting", "postrouting"
    };
    
    // Define valid actions for filter rules
    const std::set<std::string> valid_filter_actions = {
        "accept", "drop", "reject", "log", "tarpit", "jump", "fasttrack-connection",
        "add-src-to-address-list", "add-dst-to-address-list"
    };
    
    // Define valid actions for NAT rules
    const std::set<std::string> valid_nat_actions = {
        "accept", "drop", "masquerade", "redirect", "dst-nat", "src-nat", "same", "netmap"
    };
    
    // Define valid common properties for all rule types
    const std::set<std::string> common_rule_props = {
        "chain", "action", "protocol", "src-address", "dst-address", 
        "src-port", "dst-port", "in-interface", "out-interface", 
        "src_address", "dst_address", "src_port", "dst_port", 
        "in_interface", "out_interface", "comment"
    };
    
    // Define connection-state related properties
    const std::set<std::string> connection_state_props = {
        "connection-state", "connection_state"
    };
    
    // Define valid connection states
    const std::set<std::string> valid_connection_states = {
        "established", "related", "new", "invalid"
    };
    
    // Define NAT specific properties
    const std::set<std::string> nat_specific_props = {
        "to-addresses", "to-ports", "to_addresses", "to_ports"
    };
    
    // If this is a top-level firewall section, validate its subsections
    if (section->get_block()) {
        // We're simply checking if the name is one of the valid top-level firewall sections
        std::string section_name = section->get_name();
        
        // Not a top-level section? Check if it's a filter rule or NAT rule
        if (section_name == "filter") {
            // Validate filter rule
            const BlockStatement* block = section->get_block();
            if (!block) {
                return {false, "Filter section is missing its block"};
            }
            
            for (const auto* rule_stmt : block->get_statements()) {
                const SectionStatement* rule = dynamic_cast<const SectionStatement*>(rule_stmt);
                if (!rule) {
                    return {false, "Filter section can only contain rule subsections"};
                }
                
                const BlockStatement* rule_block = rule->get_block();
                if (!rule_block) {
                    return {false, "Filter rule '" + rule->get_name() + "' is missing its block"};
                }
                
                bool has_chain = false;
                bool has_action = false;
                std::string chain_value;
                std::string action_value;
                
                // Validate rule properties
                for (const auto* prop_stmt : rule_block->get_statements()) {
                    const PropertyStatement* prop = dynamic_cast<const PropertyStatement*>(prop_stmt);
                    if (!prop) {
                        continue;
                    }
                    
                    std::string prop_name = prop->get_name();
                    
                    // Check if property is valid for filter rule
                    if (common_rule_props.find(prop_name) == common_rule_props.end() && 
                        connection_state_props.find(prop_name) == connection_state_props.end()) {
                        return {false, "Invalid property '" + prop_name + "' in filter rule '" + 
                                     rule->get_name() + "'"};
                    }
                    
                    // Validate chain
                    if (prop_name == "chain") {
                        has_chain = true;
                        if (prop->get_value()) {
                            const StringValue* chain_str = dynamic_cast<const StringValue*>(prop->get_value());
                            if (chain_str) {
                                chain_value = chain_str->get_value();
                                // Remove quotes if present
                                if (chain_value.size() >= 2 && chain_value.front() == '"' && chain_value.back() == '"') {
                                    chain_value = chain_value.substr(1, chain_value.size() - 2);
                                }
                                
                                if (valid_filter_chains.find(chain_value) == valid_filter_chains.end()) {
                                    return {false, "Invalid filter chain '" + chain_value + 
                                                 "'. Valid chains are: input, forward, output"};
                                }
                            }
                        }
                    }
                    
                    // Validate action
                    if (prop_name == "action") {
                        has_action = true;
                        if (prop->get_value()) {
                            const StringValue* action_str = dynamic_cast<const StringValue*>(prop->get_value());
                            if (action_str) {
                                action_value = action_str->get_value();
                                // Remove quotes if present
                                if (action_value.size() >= 2 && action_value.front() == '"' && action_value.back() == '"') {
                                    action_value = action_value.substr(1, action_value.size() - 2);
                                }
                                
                                if (valid_filter_actions.find(action_value) == valid_filter_actions.end()) {
                                    return {false, "Invalid filter action '" + action_value + 
                                                 "'. Valid actions are: accept, drop, reject, etc."};
                                }
                            }
                        }
                    }
                    
                    // Validate connection-state if present
                    if (prop_name == "connection_state" || prop_name == "connection-state") {
                        if (prop->get_value()) {
                            // Could be a string or a list
                            const StringValue* state_str = dynamic_cast<const StringValue*>(prop->get_value());
                            const ListValue* state_list = dynamic_cast<const ListValue*>(prop->get_value());
                            
                            if (state_str) {
                                std::string state = state_str->get_value();
                                // Remove quotes if present
                                if (state.size() >= 2 && state.front() == '"' && state.back() == '"') {
                                    state = state.substr(1, state.size() - 2);
                                }
                                
                                if (valid_connection_states.find(state) == valid_connection_states.end()) {
                                    return {false, "Invalid connection state '" + state + 
                                                 "'. Valid states are: established, related, new, invalid"};
                                }
                            } else if (state_list) {
                                // Validate each state in the list
                                for (const auto* state_value : state_list->get_values()) {
                                    const StringValue* state_str = dynamic_cast<const StringValue*>(state_value);
                                    if (state_str) {
                                        std::string state = state_str->get_value();
                                        // Remove quotes if present
                                        if (state.size() >= 2 && state.front() == '"' && state.back() == '"') {
                                            state = state.substr(1, state.size() - 2);
                                        }
                                        
                                        if (valid_connection_states.find(state) == valid_connection_states.end()) {
                                            return {false, "Invalid connection state '" + state + 
                                                         "' in list. Valid states are: established, related, new, invalid"};
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                
                // Ensure required properties are present
                if (!has_chain) {
                    return {false, "Filter rule '" + rule->get_name() + "' is missing required 'chain' property"};
                }
                
                if (!has_action) {
                    return {false, "Filter rule '" + rule->get_name() + "' is missing required 'action' property"};
                }
            }
        }
        // Validate NAT rules
        else if (section_name == "nat") {
            const BlockStatement* block = section->get_block();
            if (!block) {
                return {false, "NAT section is missing its block"};
            }
            
            for (const auto* rule_stmt : block->get_statements()) {
                const SectionStatement* rule = dynamic_cast<const SectionStatement*>(rule_stmt);
                if (!rule) {
                    return {false, "NAT section can only contain rule subsections"};
                }
                
                const BlockStatement* rule_block = rule->get_block();
                if (!rule_block) {
                    return {false, "NAT rule '" + rule->get_name() + "' is missing its block"};
                }
                
                bool has_chain = false;
                bool has_action = false;
                std::string chain_value;
                std::string action_value;
                
                // Validate rule properties
                for (const auto* prop_stmt : rule_block->get_statements()) {
                    const PropertyStatement* prop = dynamic_cast<const PropertyStatement*>(prop_stmt);
                    if (!prop) {
                        continue;
                    }
                    
                    std::string prop_name = prop->get_name();
                    
                    // Check if property is valid for NAT rule
                    if (common_rule_props.find(prop_name) == common_rule_props.end() && 
                        nat_specific_props.find(prop_name) == nat_specific_props.end()) {
                        return {false, "Invalid property '" + prop_name + "' in NAT rule '" + 
                                     rule->get_name() + "'"};
                    }
                    
                    // Validate chain
                    if (prop_name == "chain") {
                        has_chain = true;
                        if (prop->get_value()) {
                            const StringValue* chain_str = dynamic_cast<const StringValue*>(prop->get_value());
                            if (chain_str) {
                                chain_value = chain_str->get_value();
                                // Remove quotes if present
                                if (chain_value.size() >= 2 && chain_value.front() == '"' && chain_value.back() == '"') {
                                    chain_value = chain_value.substr(1, chain_value.size() - 2);
                                }
                                
                                if (valid_nat_chains.find(chain_value) == valid_nat_chains.end()) {
                                    return {false, "Invalid NAT chain '" + chain_value + 
                                                 "'. Valid chains are: srcnat, dstnat, prerouting, postrouting"};
                                }
                            }
                        }
                    }
                    
                    // Validate action
                    if (prop_name == "action") {
                        has_action = true;
                        if (prop->get_value()) {
                            const StringValue* action_str = dynamic_cast<const StringValue*>(prop->get_value());
                            if (action_str) {
                                action_value = action_str->get_value();
                                // Remove quotes if present
                                if (action_value.size() >= 2 && action_value.front() == '"' && action_value.back() == '"') {
                                    action_value = action_value.substr(1, action_value.size() - 2);
                                }
                                
                                if (valid_nat_actions.find(action_value) == valid_nat_actions.end()) {
                                    return {false, "Invalid NAT action '" + action_value + 
                                                 "'. Valid actions are: masquerade, dst-nat, src-nat, etc."};
                                }
                            }
                        }
                    }
                }
                
                // Ensure required properties are present
                if (!has_chain) {
                    return {false, "NAT rule '" + rule->get_name() + "' is missing required 'chain' property"};
                }
                
                if (!has_action) {
                    return {false, "NAT rule '" + rule->get_name() + "' is missing required 'action' property"};
                }
                
                // Check specific requirements for certain NAT actions
                if (action_value == "masquerade") {
                    bool has_out_interface = false;
                    for (const auto* prop_stmt : rule_block->get_statements()) {
                        const PropertyStatement* prop = dynamic_cast<const PropertyStatement*>(prop_stmt);
                        if (prop && (prop->get_name() == "out_interface" || prop->get_name() == "out-interface")) {
                            has_out_interface = true;
                            break;
                        }
                    }
                    
                    if (!has_out_interface) {
                        return {false, "NAT rule with 'masquerade' action requires 'out_interface' property"};
                    }
                }
            }
        }
        // Validation for other subsections can be added here
    }
    
    return {true, ""};
}

bool FirewallValidator::isValidNesting(const std::string& parent_name, 
                                     const std::string& child_name) const {
    // Define valid firewall subsections
    const std::set<std::string> valid_subsections = {
        "filter", "nat", "mangle", "raw", "address-list", "service-port", "layer7-protocol"
    };
    
    // Check if parent is a standard subsection
    bool is_standard_subsection = false;
    for (const auto& valid_name : valid_subsections) {
        if (parent_name == valid_name) {
            is_standard_subsection = true;
            break;
        }
    }
    
    // Exception for template/group sections
    if (parent_name == "template" || parent_name == "group") {
        return true;
    }
    
    // For firewall subsections like filter, nat - allow rule children but not deeper nesting
    if (is_standard_subsection) {
        // Rules under filter, nat, etc. are allowed
        return true;
    }
    
    // For rule entries - no nesting allowed
    return false;
}

CustomValidator::CustomValidator()
    : SectionValidator("custom", NestingRule::DEEP_NESTING) {
}

std::tuple<bool, std::string> CustomValidator::validateProperties(
    const SectionStatement* section) const {
    // Custom sections are more permissive
    return {true, ""};
}
