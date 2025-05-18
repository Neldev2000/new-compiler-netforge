#pragma once

#include "statement.hpp"

// Base class for all specialized sections
class SpecializedSection : public SectionStatement {
public:
    SpecializedSection(std::string_view name) noexcept;
    
    // Add semantic validation method
    virtual bool validate() const noexcept = 0;
    
    // Override the to_mikrotik method for specialized translation
    std::string to_mikrotik(const std::string& ident) const override;
    
protected:
    // Helper method to be implemented by derived classes for specialized translation
    virtual std::string translate_section(const std::string& ident) const = 0;
};

// Device section
class DeviceSection : public SpecializedSection {
public:
    DeviceSection(std::string_view name) noexcept;
    
    bool validate() const noexcept override;
    
protected:
    std::string translate_section(const std::string& ident) const override;
};

// Interfaces section
class InterfacesSection : public SpecializedSection {
public:
    InterfacesSection(std::string_view name) noexcept;
    
    bool validate() const noexcept override;
    
protected:
    std::string translate_section(const std::string& ident) const override;
};

// IP section
class IPSection : public SpecializedSection {
public:
    IPSection(std::string_view name) noexcept;
    
    bool validate() const noexcept override;
    
protected:
    std::string translate_section(const std::string& ident) const override;
};

// Routing section
class RoutingSection : public SpecializedSection {
public:
    RoutingSection(std::string_view name) noexcept;
    
    bool validate() const noexcept override;
    
protected:
    std::string translate_section(const std::string& ident) const override;
};

// Firewall section
class FirewallSection : public SpecializedSection {
public:
    FirewallSection(std::string_view name) noexcept;
    
    bool validate() const noexcept override;
    
protected:
    std::string translate_section(const std::string& ident) const override;
};

// System section
class SystemSection : public SpecializedSection {
public:
    SystemSection(std::string_view name) noexcept;
    
    bool validate() const noexcept override;
    
protected:
    std::string translate_section(const std::string& ident) const override;
};

// Custom section
class CustomSection : public SpecializedSection {
public:
    CustomSection(std::string_view name) noexcept;
    
    bool validate() const noexcept override;
    
protected:
    std::string translate_section(const std::string& ident) const override;
};

// Factory function to create the appropriate specialized section
SpecializedSection* create_specialized_section(std::string_view name, SectionStatement::SectionType type); 