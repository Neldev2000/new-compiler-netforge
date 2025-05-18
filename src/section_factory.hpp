#pragma once

#include "specialized_sections.hpp"

class SectionFactory {
public:
    // Create a specialized section based on section type
    static SectionStatement* create_section(std::string_view name, SectionStatement::SectionType type, BlockStatement* block = nullptr) {
        SpecializedSection* section = create_specialized_section(name, type);
        
        if (block) {
            section->set_block(block);
        }
        
        return section;
    }
    
    // Create a section from an existing SectionStatement
    static SectionStatement* create_section_from_generic(SectionStatement* generic_section) {
        if (!generic_section) return nullptr;
        
        SpecializedSection* specialized = create_specialized_section(
            generic_section->get_name(), 
            generic_section->get_section_type()
        );
        
        specialized->set_block(generic_section->get_block());
        
        // Return newly created specialized section and destroy the generic one
        generic_section->destroy();
        
        return specialized;
    }
}; 