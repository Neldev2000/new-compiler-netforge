/*
#pragma once
#include <string>
#include <tuple>
#include <set>
#include <functional>
#include <memory>
#include <unordered_map>

// Forward declarations
class SectionStatement;
class BlockStatement;
class PropertyStatement;
class Statement;
class Expression;


class SectionValidator {
public:

    std::tuple<bool, std::string> validate(const BlockStatement* block) const;

protected:
    // Types of section nesting allowed
    enum class NestingRule {
        NO_NESTING,         // No subsections allowed
        SHALLOW_NESTING,    // Subsections allowed, but only one level deep
        DEEP_NESTING,       // Multiple levels of subsection nesting allowed
        CONDITIONAL_NESTING // Custom condition for nesting
    };

    SectionValidator(std::string section_name, NestingRule nesting_rule);
    
  
    virtual ~SectionValidator() = default;
    

    virtual std::tuple<bool, std::string> validateProperties(
        const SectionStatement* section) const = 0;

    virtual bool isValidNesting(const std::string& parent_name, 
                               const std::string& child_name) const;
                               
  
    std::string getSectionName() const;
    
private:
    std::string section_name_;
    NestingRule nesting_rule_;

    std::tuple<bool, std::string> validateHierarchy(const BlockStatement* block) const;
};



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

class IPValidator : public SectionValidator {
public:
    IPValidator();
    
protected:
    std::tuple<bool, std::string> validateProperties(
        const SectionStatement* section) const override;
};

class RoutingValidator : public SectionValidator {
public:
    RoutingValidator();
    
protected:
    std::tuple<bool, std::string> validateProperties(
        const SectionStatement* section) const override;
};


class FirewallValidator : public SectionValidator {
public:
    FirewallValidator();
    
protected:
    std::tuple<bool, std::string> validateProperties(
        const SectionStatement* section) const override;
};

class CustomValidator : public SectionValidator {
public:
    CustomValidator();
    
protected:
    std::tuple<bool, std::string> validateProperties(
        const SectionStatement* section) const override;
};
*/