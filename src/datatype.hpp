#pragma once

#include "ast_node_interface.hpp"

// Base class for all data types
class Datatype : public ASTNodeInterface
{
public:
    // Enum to represent different data types in our DSL
    enum class Type {
        STRING,
        NUMBER,
        BOOLEAN,
        IP_ADDRESS,
        IP_CIDR,
        IP_RANGE,
        IPV6_ADDRESS,
        IPV6_CIDR,
        IPV6_RANGE,
        SECTION,  // For config sections
        LIST      // For lists of values
    };

    Datatype(Type type_value) noexcept;
    Type get_type() const noexcept;
    
    // Returns a string representation of the type
    virtual std::string type_name() const;
    std::string to_string() const override;
    std::string to_mikrotik(const std::string& ident) const override;

protected:
    Type type;
};

// Basic types used in the DSL
class BasicDatatype : public Datatype
{
public:
    BasicDatatype(Type type_value) noexcept;
    void destroy() noexcept override;
    std::string to_mikrotik(const std::string& ident) const override;
};

// String type (for names, descriptions, etc.)
class StringDatatype : public BasicDatatype
{
public:
    StringDatatype() noexcept;
    std::string to_mikrotik(const std::string& ident) const override;
};

// Number type (for port numbers, VLAN IDs, etc.)
class NumberDatatype : public BasicDatatype
{
public:
    NumberDatatype() noexcept;
    std::string to_mikrotik(const std::string& ident) const override;
};

// Boolean type (for enabled/disabled states)
class BooleanDatatype : public BasicDatatype
{
public:
    BooleanDatatype() noexcept;
    std::string to_mikrotik(const std::string& ident) const override;
};

// Network address types
class IPAddressDatatype : public BasicDatatype
{
public:
    IPAddressDatatype() noexcept;
    std::string to_mikrotik(const std::string& ident) const override;
};

class IPCIDRDatatype : public BasicDatatype
{
public:
    IPCIDRDatatype() noexcept;
    std::string to_mikrotik(const std::string& ident) const override;
};

// Config section type (for device, interfaces, firewall sections)
class ConfigSectionDatatype : public BasicDatatype
{
public:
    ConfigSectionDatatype() noexcept;
    std::string type_name() const override;
    std::string to_mikrotik(const std::string& ident) const override;
};

// Interface type (for network interfaces)
class InterfaceDatatype : public BasicDatatype
{
public:
    InterfaceDatatype() noexcept;
    std::string type_name() const override;
    std::string to_mikrotik(const std::string& ident) const override;
};

// List type (for arrays of values)
class ListDatatype : public Datatype
{
public:
    ListDatatype(Datatype* element_type) noexcept;
    void destroy() noexcept override;
    Datatype* get_element_type() const noexcept;
    std::string to_mikrotik(const std::string& ident) const override;

private:
    Datatype* element_type; // Type of elements in the list
}; 