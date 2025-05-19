#include "semantic_validator.hpp"
#include "specialized_sections.hpp"

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
