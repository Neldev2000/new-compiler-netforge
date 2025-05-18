#include "datatype.hpp"

// Datatype implementation
Datatype::Datatype(Type type_value) noexcept : type(type_value) {}

Datatype::Type Datatype::get_type() const noexcept 
{
    return type; 
}

std::string Datatype::type_name() const 
{
    switch (type) {
        case Type::STRING: return "string";
        case Type::NUMBER: return "number";
        case Type::BOOLEAN: return "boolean";
        case Type::IP_ADDRESS: return "ip_address";
        case Type::IP_CIDR: return "ip_cidr";
        case Type::IP_RANGE: return "ip_range";
        case Type::IPV6_ADDRESS: return "ipv6_address";
        case Type::IPV6_CIDR: return "ipv6_cidr";
        case Type::IPV6_RANGE: return "ipv6_range";
        case Type::SECTION: return "section";
        case Type::LIST: return "list";
        default: return "unknown";
    }
}

std::string Datatype::to_string() const 
{
    return type_name();
}

std::string Datatype::to_mikrotik(const std::string& ident) const 
{
    // Return empty string for base type - implementations should override
    return "";
}

// BasicDatatype implementation
BasicDatatype::BasicDatatype(Type type_value) noexcept : Datatype(type_value) {}

void BasicDatatype::destroy() noexcept 
{
    // No dynamic memory to clean up
}

std::string BasicDatatype::to_mikrotik(const std::string& ident) const 
{
    // Return empty string by default
    return "";
}

// StringDatatype implementation
StringDatatype::StringDatatype() noexcept : BasicDatatype(Type::STRING) {}

std::string StringDatatype::to_mikrotik(const std::string& ident) const 
{
    // In MikroTik scripting, strings are enclosed in double quotes
    return "\"\""; // Empty string representation
}

// NumberDatatype implementation
NumberDatatype::NumberDatatype() noexcept : BasicDatatype(Type::NUMBER) {}

std::string NumberDatatype::to_mikrotik(const std::string& ident) const 
{
    // Numbers in MikroTik are represented directly
    return "0"; // Default number representation
}

// BooleanDatatype implementation
BooleanDatatype::BooleanDatatype() noexcept : BasicDatatype(Type::BOOLEAN) {}

std::string BooleanDatatype::to_mikrotik(const std::string& ident) const 
{
    // Booleans in MikroTik are represented as true/false
    return "false"; // Default boolean representation
}

// IPAddressDatatype implementation
IPAddressDatatype::IPAddressDatatype() noexcept : BasicDatatype(Type::IP_ADDRESS) {}

std::string IPAddressDatatype::to_mikrotik(const std::string& ident) const 
{
    // IP addresses are represented as strings in MikroTik
    return "\"0.0.0.0\""; // Default IP address representation
}

// IPCIDRDatatype implementation
IPCIDRDatatype::IPCIDRDatatype() noexcept : BasicDatatype(Type::IP_CIDR) {}

std::string IPCIDRDatatype::to_mikrotik(const std::string& ident) const 
{
    // CIDR notation in MikroTik is represented as strings
    return "\"0.0.0.0/0\""; // Default CIDR representation
}

// ConfigSectionDatatype implementation
ConfigSectionDatatype::ConfigSectionDatatype() noexcept : BasicDatatype(Type::SECTION) {}

std::string ConfigSectionDatatype::type_name() const {
    return "ConfigSection";
}

std::string ConfigSectionDatatype::to_mikrotik(const std::string& ident) const 
{
    // Config sections in MikroTik are typically represented as paths
    return "\"/\""; // Root path representation
}

// InterfaceDatatype implementation
InterfaceDatatype::InterfaceDatatype() noexcept : BasicDatatype(Type::SECTION) {}

std::string InterfaceDatatype::type_name() const {
    return "Interface";
}

std::string InterfaceDatatype::to_mikrotik(const std::string& ident) const 
{
    // Interfaces in MikroTik are typically represented by their names
    return "\"interface\""; // Generic interface representation
}

// ListDatatype implementation
ListDatatype::ListDatatype(Datatype* element_type) noexcept 
    : Datatype(Type::LIST), element_type(element_type) {}

void ListDatatype::destroy() noexcept 
{
    if (element_type) {
        element_type->destroy();
        delete element_type;
        element_type = nullptr;
    }
}

Datatype* ListDatatype::get_element_type() const noexcept 
{
    return element_type;
}

std::string ListDatatype::to_mikrotik(const std::string& ident) const 
{
    // In MikroTik, arrays are represented using curly braces
    return "{}"; // Empty array representation
} 