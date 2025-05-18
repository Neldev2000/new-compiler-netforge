#include "specialized_sections.hpp"

// SpecializedSection implementation
SpecializedSection::SpecializedSection(std::string_view name) noexcept
    : SectionStatement(name, SectionType::CUSTOM) // Temporarily set as CUSTOM, will be overridden
{
}

std::string SpecializedSection::to_mikrotik(const std::string& ident) const {
    // Common translation logic
    return translate_section(ident);
}

// DeviceSection implementation
DeviceSection::DeviceSection(std::string_view name) noexcept
    : SpecializedSection(name)
{
    // Override the section type
    this->type = SectionType::DEVICE;
}

bool DeviceSection::validate() const noexcept {
    // Validate device section - requires vendor, model, and hostname
    const BlockStatement* block = get_block();
    if (!block) return false;
    
    bool has_vendor = false;
    bool has_model = false;
    bool has_hostname = false;
    
    // Check if required properties exist
    for (const Statement* stmt : block->get_statements()) {
        const PropertyStatement* prop = dynamic_cast<const PropertyStatement*>(stmt);
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
        }
    }
    
    // Ensure all required properties are present
    return has_vendor && has_model && has_hostname;
}

std::string DeviceSection::translate_section(const std::string& ident) const {
    std::string result = "# Device Configuration\n";
    
    if (get_block()) {
        // Extract device properties
        std::string vendor = "";
        std::string model = "";
        std::string hostname = "";
        
        // Iterate through statements to find vendor, model, and hostname properties
        const BlockStatement* block = get_block();
        for (const Statement* stmt : block->get_statements()) {
            const PropertyStatement* prop = dynamic_cast<const PropertyStatement*>(stmt);
            if (prop) {
                const std::string& name = prop->get_name();
                Expression* expr = prop->get_value();
                
                if (name == "vendor" && expr) {
                    const StringValue* value = dynamic_cast<const StringValue*>(expr);
                    if (value) {
                        // Remove any quotes from the string value
                        vendor = value->get_value();
                        if (vendor.size() >= 2 && vendor.front() == '"' && vendor.back() == '"') {
                            vendor = vendor.substr(1, vendor.size() - 2);
                        }
                    }
                }
                else if (name == "model" && expr) {
                    const StringValue* value = dynamic_cast<const StringValue*>(expr);
                    if (value) {
                        // Remove any quotes from the string value
                        model = value->get_value();
                        if (model.size() >= 2 && model.front() == '"' && model.back() == '"') {
                            model = model.substr(1, model.size() - 2);
                        }
                    }
                }
                else if (name == "hostname" && expr) {
                    const StringValue* value = dynamic_cast<const StringValue*>(expr);
                    if (value) {
                        // Remove any quotes from the string value
                        hostname = value->get_value();
                        if (hostname.size() >= 2 && hostname.front() == '"' && hostname.back() == '"') {
                            hostname = hostname.substr(1, hostname.size() - 2);
                        }
                    }
                }
            }
        }
        
        // Create the combined name: vendor_hostname_model
        std::string combined_name = "";
        if (!vendor.empty()) {
            combined_name += vendor;
            if (!hostname.empty() || !model.empty()) combined_name += "_";
        }
        
        if (!hostname.empty()) {
            combined_name += hostname;
            if (!model.empty()) combined_name += "_";
        }
        
        if (!model.empty()) {
            combined_name += model;
        }
        
        // If we couldn't extract the values, use a default name
        if (combined_name.empty()) {
            combined_name = "router";
        }
        
        // Generate the MikroTik script
        result += "/system identity set name=\"" + combined_name + "\"\n";
    }
    
    return result;
}

// InterfacesSection implementation
InterfacesSection::InterfacesSection(std::string_view name) noexcept
    : SpecializedSection(name)
{
    this->type = SectionType::INTERFACES;
}

bool InterfacesSection::validate() const noexcept {
    const BlockStatement* block = get_block();
    if (!block) return false;
    
    // Add specific interface section validation logic here
    return true;
}

std::string InterfacesSection::translate_section(const std::string& ident) const {
    std::string result = ident + "# Interface Configuration: " + get_name() + "\n";
    
    if (get_block()) {
        // Interface-specific translation
        result += ident + "/interface\n";
        result += get_block()->to_mikrotik(ident + "  ");
    }
    
    return result;
}

// IPSection implementation
IPSection::IPSection(std::string_view name) noexcept
    : SpecializedSection(name)
{
    this->type = SectionType::IP;
}

bool IPSection::validate() const noexcept {
    const BlockStatement* block = get_block();
    if (!block) return false;
    
    // Add specific IP section validation logic here
    return true;
}

std::string IPSection::translate_section(const std::string& ident) const {
    std::string result = ident + "# IP Configuration: " + get_name() + "\n";
    
    if (get_block()) {
        // IP-specific translation
        result += ident + "/ip\n";
        result += get_block()->to_mikrotik(ident + "  ");
    }
    
    return result;
}

// RoutingSection implementation
RoutingSection::RoutingSection(std::string_view name) noexcept
    : SpecializedSection(name)
{
    this->type = SectionType::ROUTING;
}

bool RoutingSection::validate() const noexcept {
    const BlockStatement* block = get_block();
    if (!block) return false;
    
    // Add specific routing section validation logic here
    return true;
}

std::string RoutingSection::translate_section(const std::string& ident) const {
    std::string result = ident + "# Routing Configuration: " + get_name() + "\n";
    
    if (get_block()) {
        // Routing-specific translation
        result += ident + "/routing\n";
        result += get_block()->to_mikrotik(ident + "  ");
    }
    
    return result;
}

// FirewallSection implementation
FirewallSection::FirewallSection(std::string_view name) noexcept
    : SpecializedSection(name)
{
    this->type = SectionType::FIREWALL;
}

bool FirewallSection::validate() const noexcept {
    const BlockStatement* block = get_block();
    if (!block) return false;
    
    // Add specific firewall section validation logic here
    return true;
}

std::string FirewallSection::translate_section(const std::string& ident) const {
    std::string result = ident + "# Firewall Configuration: " + get_name() + "\n";
    
    if (get_block()) {
        // Firewall-specific translation
        result += ident + "/ip firewall\n";
        result += get_block()->to_mikrotik(ident + "  ");
    }
    
    return result;
}

// SystemSection implementation
SystemSection::SystemSection(std::string_view name) noexcept
    : SpecializedSection(name)
{
    this->type = SectionType::SYSTEM;
}

bool SystemSection::validate() const noexcept {
    const BlockStatement* block = get_block();
    if (!block) return false;
    
    // Add specific system section validation logic here
    return true;
}

std::string SystemSection::translate_section(const std::string& ident) const {
    std::string result = ident + "# System Configuration: " + get_name() + "\n";
    
    if (get_block()) {
        // System-specific translation
        result += ident + "/system\n";
        result += get_block()->to_mikrotik(ident + "  ");
    }
    
    return result;
}

// CustomSection implementation
CustomSection::CustomSection(std::string_view name) noexcept
    : SpecializedSection(name)
{
    this->type = SectionType::CUSTOM;
}

bool CustomSection::validate() const noexcept {
    const BlockStatement* block = get_block();
    if (!block) return false;
    
    // Custom sections are more permissive
    return true;
}

std::string CustomSection::translate_section(const std::string& ident) const {
    std::string result = ident + "# Custom Configuration: " + get_name() + "\n";
    
    if (get_block()) {
        // For custom sections, simply translate the block
        result += get_block()->to_mikrotik(ident);
    }
    
    return result;
}

// Factory function implementation
SpecializedSection* create_specialized_section(std::string_view name, SectionStatement::SectionType type) {
    switch (type) {
        case SectionStatement::SectionType::DEVICE:
            return new DeviceSection(name);
        case SectionStatement::SectionType::INTERFACES:
            return new InterfacesSection(name);
        case SectionStatement::SectionType::IP:
            return new IPSection(name);
        case SectionStatement::SectionType::ROUTING:
            return new RoutingSection(name);
        case SectionStatement::SectionType::FIREWALL:
            return new FirewallSection(name);
        case SectionStatement::SectionType::SYSTEM:
            return new SystemSection(name);
        case SectionStatement::SectionType::CUSTOM:
        default:
            return new CustomSection(name);
    }
} 