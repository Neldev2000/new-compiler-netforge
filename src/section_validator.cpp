/*
#include "section_validator.hpp"
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

// Implement the missing validateHierarchy method
std::tuple<bool, std::string> SectionValidator::validateHierarchy(const BlockStatement* block) const {
    // Handle different nesting rules
    if (nesting_rule_ == NestingRule::NO_NESTING) {
        // Check that no subsections exist
        for (const Statement* stmt : block->get_statements()) {
            const SectionStatement* subsection = dynamic_cast<const SectionStatement*>(stmt);
            if (subsection) {
                return std::make_tuple(false, "No subsections allowed in " + section_name_);
            }
        }
    } else if (nesting_rule_ == NestingRule::SHALLOW_NESTING || 
               nesting_rule_ == NestingRule::DEEP_NESTING || 
               nesting_rule_ == NestingRule::CONDITIONAL_NESTING) {
        // For these rules, validate each subsection
        for (const Statement* stmt : block->get_statements()) {
            const SectionStatement* subsection = dynamic_cast<const SectionStatement*>(stmt);
            
            if (subsection) {
                // Check if the specific nesting is allowed
                if (!isValidNesting(section_name_, subsection->get_name())) {
                    return std::make_tuple(false, 
                        "Invalid subsection '" + subsection->get_name() + 
                        "' in " + section_name_ + " section");
                }
                
                // For DEEP_NESTING, recursively check subsections
                if (nesting_rule_ == NestingRule::DEEP_NESTING) {
                    const BlockStatement* subblock = subsection->get_block();
                    if (subblock) {
                        // Verify sub-sections
                        for (const Statement* substmt : subblock->get_statements()) {
                            const SectionStatement* subsubsection = 
                                dynamic_cast<const SectionStatement*>(substmt);
                            
                            if (subsubsection && 
                                !isValidNesting(subsection->get_name(), subsubsection->get_name())) {
                                return std::make_tuple(false, 
                                    "Invalid subsection '" + subsubsection->get_name() + 
                                    "' in " + subsection->get_name() + " section");
                            }
                        }
                    }
                } else if (nesting_rule_ == NestingRule::SHALLOW_NESTING) {
                    // Check that no nested subsections exist
                    const BlockStatement* subblock = subsection->get_block();
                    if (subblock) {
                        for (const Statement* substmt : subblock->get_statements()) {
                            const SectionStatement* subsubsection = 
                                dynamic_cast<const SectionStatement*>(substmt);
                            
                            if (subsubsection) {
                                return std::make_tuple(false, 
                                    "Nested subsections not allowed in " + section_name_);
                            }
                        }
                    }
                }
            }
        }
    }
    
    return std::make_tuple(true, "");
}

// Implement the missing isValidNesting method
bool SectionValidator::isValidNesting(const std::string& parent_name, 
                                     const std::string& child_name) const {
    // Default implementation allows any nesting if not overridden
    return true;
}


DeviceValidator::DeviceValidator()
    : SectionValidator("device", NestingRule::NO_NESTING) {}

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
    : SectionValidator("interfaces", NestingRule::DEEP_NESTING) {}

bool InterfacesValidator::isValidNesting(const std::string& parent_name, 
                                        const std::string& child_name) const {
    // Only allow nesting for specific parent interface types
    if (parent_name == "template" || parent_name == "group") {
        return true;
    }
    
    // By default, do not allow interfaces to be nested under other interfaces
    return false;
}

std::tuple<bool, std::string> InterfacesValidator::validateProperties(
    const SectionStatement* section) const {
    // Add implementation here if needed
    return std::make_tuple(true, "");
}
*/