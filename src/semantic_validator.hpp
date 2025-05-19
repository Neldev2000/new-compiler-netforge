#pragma once

#include <string>
#include <tuple>
#include <set>
#include <functional>
#include <memory>
#include <unordered_map>

#include "statement.hpp"

// Forward declarations
class SectionStatement;
class BlockStatement;
class PropertyStatement;
class Statement;


/**
 * @class SectionValidator
 * @brief Base validator class for specialized sections
 * 
 * This abstract class provides common functionality for validating
 * the semantic structure of specialized sections with consistent
 * hierarchy rules.
 */
class SectionValidator {
public:
    /**
     * @brief Validate the section structure and properties
     * @param block The block statement containing the section content
     * @return Tuple of validation result (success/failure) and error message
     */
    std::tuple<bool, std::string> validate(const BlockStatement* block) const;

protected:
    // Types of section nesting allowed
    enum class NestingRule {
        NO_NESTING,         // No subsections allowed
        SHALLOW_NESTING,    // Subsections allowed, but only one level deep
        DEEP_NESTING,       // Multiple levels of subsection nesting allowed
        CONDITIONAL_NESTING // Custom condition for nesting
    };

    /**
     * @brief Constructor with section name and nesting rule
     * @param section_name The name of the section being validated
     * @param nesting_rule The nesting rule to apply for this section
     */
    SectionValidator(std::string section_name, NestingRule nesting_rule);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~SectionValidator() = default;
    
    /**
     * @brief Validate properties specific to this section type
     * @param section The section statement to validate
     * @return Tuple of validation result and error message
     */
    virtual std::tuple<bool, std::string> validateProperties(
        const SectionStatement* section) const = 0;
    
    /**
     * @brief Check if nesting is valid for the given parent and child
     * @param parent_name The name of the parent section
     * @param child_name The name of the child section
     * @return True if nesting is allowed, false otherwise
     */
    virtual bool isValidNesting(const std::string& parent_name, 
                               const std::string& child_name) const;
                               
    /**
     * @brief Get the name of this section type
     * @return Section name
     */
    std::string getSectionName() const;
    
private:
    std::string section_name_;
    NestingRule nesting_rule_;
    
    /**
     * @brief Validate the hierarchical structure of the section
     * @param block The block statement containing the section content
     * @return Tuple of validation result and error message
     */
    std::tuple<bool, std::string> validateHierarchy(const BlockStatement* block) const;
};

class DeviceValidator : public SectionValidator {
public:
    DeviceValidator();
    
protected:
    std::tuple<bool, std::string> validateProperties(
        const SectionStatement* section) const override;
};
/**
 * @class InterfacesValidator
 * @brief Validator for Interfaces section
 */
class InterfacesValidator : public SectionValidator {
public:
    InterfacesValidator();
    
protected:
    std::tuple<bool, std::string> validateProperties(
        const SectionStatement* section) const override;
        
    bool isValidNesting(const std::string& parent_name, 
                       const std::string& child_name) const override;
                       
private:
    // Define valid properties for different interface types
    std::set<std::string> common_valid_props_;
    std::set<std::string> vlan_specific_props_;
    std::set<std::string> bonding_specific_props_;
    std::set<std::string> bridge_specific_props_;
    std::set<std::string> ethernet_specific_props_;
};

/**
 * @class IPValidator
 * @brief Validator for IP section
 */
class IPValidator : public SectionValidator {
public:
    IPValidator();
    
protected:
    std::tuple<bool, std::string> validateProperties(
        const SectionStatement* section) const override;
};

/**
 * @class RoutingValidator
 * @brief Validator for Routing section
 */
class RoutingValidator : public SectionValidator {
public:
    RoutingValidator();
    
protected:
    std::tuple<bool, std::string> validateProperties(
        const SectionStatement* section) const override;
};

/**
 * @class FirewallValidator
 * @brief Validator for Firewall section
 */
class FirewallValidator : public SectionValidator {
public:
    FirewallValidator();
    
protected:
    std::tuple<bool, std::string> validateProperties(
        const SectionStatement* section) const override;
};

/**
 * @class CustomValidator
 * @brief Validator for Custom section
 */
class CustomValidator : public SectionValidator {
public:
    CustomValidator();
    
protected:
    std::tuple<bool, std::string> validateProperties(
        const SectionStatement* section) const override;
};