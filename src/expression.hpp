#pragma once

#include "ast_node_interface.hpp"
#include "datatype.hpp"

// Base class for all expressions
class Expression : public ASTNodeInterface
{
public:
    // Get the data type of this expression
    virtual Datatype* get_type() const = 0;
};

// Base class for values (literals)
class Value : public Expression
{
public:
    enum class ValueType {
        STRING,
        NUMBER,
        BOOLEAN,
        IP_ADDRESS,
        IP_CIDR,
        IP_RANGE,
        IPV6_ADDRESS,
        IPV6_CIDR,
        IPV6_RANGE
    };

    Value(ValueType val_type) noexcept;
    ValueType get_value_type() const noexcept;
    
    void destroy() noexcept override;
    std::string to_mikrotik(const std::string& ident) const override;
    
protected:
    ValueType value_type;
};

// String literal value
class StringValue : public Value
{
public:
    StringValue(std::string_view str_value) noexcept;
    
    const std::string& get_value() const noexcept;
    Datatype* get_type() const override;
    std::string to_string() const override;
    std::string to_mikrotik(const std::string& ident) const override;
    
private:
    std::string str_value;
};

// Numeric literal value
class NumberValue : public Value
{
public:
    NumberValue(int num_value) noexcept;
    
    int get_value() const noexcept;
    Datatype* get_type() const override;
    std::string to_string() const override;
    std::string to_mikrotik(const std::string& ident) const override;
    
private:
    int num_value;
};

// Boolean literal value
class BooleanValue : public Value
{
public:
    BooleanValue(bool bool_value) noexcept;
    
    bool get_value() const noexcept;
    Datatype* get_type() const override;
    std::string to_string() const override;
    std::string to_mikrotik(const std::string& ident) const override;
    
private:
    bool bool_value;
};

// IP address value
class IPAddressValue : public Value
{
public:
    IPAddressValue(std::string_view ip_value) noexcept;
    
    const std::string& get_value() const noexcept;
    Datatype* get_type() const override;
    std::string to_string() const override;
    std::string to_mikrotik(const std::string& ident) const override;
    
private:
    std::string ip_value;
};

// IP CIDR value (e.g., 192.168.1.0/24)
class IPCIDRValue : public Value
{
public:
    IPCIDRValue(std::string_view cidr_value) noexcept;
    
    const std::string& get_value() const noexcept;
    Datatype* get_type() const override;
    std::string to_string() const override;
    std::string to_mikrotik(const std::string& ident) const override;
    
private:
    std::string cidr_value;
};

// List of values
class ListValue : public Expression
{
public:
    ListValue(const ValueList& values, Datatype* element_type = nullptr) noexcept;
    
    const ValueList& get_values() const noexcept;
    void destroy() noexcept override;
    Datatype* get_type() const override;
    std::string to_string() const override;
    std::string to_mikrotik(const std::string& ident) const override;
    
private:
    ValueList values;
    Datatype* element_type;
};

// Identifier reference
class IdentifierExpression : public Expression
{
public:
    IdentifierExpression(std::string_view name) noexcept;
    
    const std::string& get_name() const noexcept;
    void destroy() noexcept override;
    Datatype* get_type() const override;
    std::string to_string() const override;
    std::string to_mikrotik(const std::string& ident) const override;
    
private:
    std::string name;
};

// Property reference (identifier.property)
class PropertyReference : public Expression
{
public:
    PropertyReference(Expression* base, std::string_view property_name) noexcept;
    
    const std::string& get_property_name() const noexcept;
    Expression* get_base() const noexcept;
    void destroy() noexcept override;
    Datatype* get_type() const override;
    std::string to_string() const override;
    std::string to_mikrotik(const std::string& ident) const override;
    
private:
    Expression* base;
    std::string property_name;
}; 