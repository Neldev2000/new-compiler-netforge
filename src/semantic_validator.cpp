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
