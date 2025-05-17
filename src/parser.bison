%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "datatype.hpp"
#include "declaration.hpp"
#include "expression.hpp"
#include "statement.hpp"

extern int yylex();
extern char* yytext;
extern int line_number;
extern FILE* yyin;
int yyerror(const char* s);

// Global result for the parser
ProgramDeclaration* parser_result = nullptr;

// Helper function to map string to SectionType
SectionStatement::SectionType get_section_type(const char* section_name) {
    if (strcmp(section_name, "device") == 0) return SectionStatement::SectionType::DEVICE;
    else if (strcmp(section_name, "interfaces") == 0) return SectionStatement::SectionType::INTERFACES;
    else if (strcmp(section_name, "ip") == 0) return SectionStatement::SectionType::IP;
    else if (strcmp(section_name, "routing") == 0) return SectionStatement::SectionType::ROUTING;
    else if (strcmp(section_name, "firewall") == 0) return SectionStatement::SectionType::FIREWALL;
    else if (strcmp(section_name, "system") == 0) return SectionStatement::SectionType::SYSTEM;
    else return SectionStatement::SectionType::CUSTOM;
}

// Debug logging functions
int nesting_level = 0;

void enter_section(const char* name) {
    nesting_level = 1;
    printf("Debug: Entering section %s\n", name);
}

void enter_block(const char* name) {
    nesting_level++;
    printf("Debug: Increasing nesting to %d, entering block %s\n", nesting_level, name);
}

void exit_block() {
    nesting_level--;
    printf("Debug: Decreasing nesting to %d\n", nesting_level);
    
    if (nesting_level == 0) {
        printf("Debug: Leaving section\n");
    }
}
%}

%define parse.error verbose

/* Enable location tracking for better error messages */
%locations

/* Define value types for tokens and non-terminals */
%union {
    const char* str_val;
    int int_val;
    Statement* stmt_val;
    Expression* expr_val;
    SectionStatement* section_val;
    BlockStatement* block_val;
    PropertyStatement* property_val;
    Value* value_val;
    ListValue* list_val;
    ProgramDeclaration* program_val;
}

/* Tokens from flex scanner */
%token TOKEN_COLON TOKEN_EQUALS TOKEN_LEFT_BRACKET TOKEN_RIGHT_BRACKET
%token TOKEN_LEFT_BRACE TOKEN_RIGHT_BRACE TOKEN_COMMA TOKEN_SLASH
%token TOKEN_MINUS TOKEN_DOT
%token TOKEN_SEMICOLON

/* Keyword tokens */
%token TOKEN_DEVICE TOKEN_VENDOR TOKEN_INTERFACES TOKEN_IP TOKEN_ROUTING
%token TOKEN_FIREWALL TOKEN_SYSTEM TOKEN_TYPE TOKEN_ADMIN_STATE
%token TOKEN_DESCRIPTION TOKEN_ETHERNET TOKEN_SPEED TOKEN_DUPLEX
%token TOKEN_VLAN TOKEN_VLAN_ID TOKEN_INTERFACE TOKEN_ADDRESS
%token TOKEN_DHCP TOKEN_DHCP_CLIENT TOKEN_DHCP_SERVER
%token TOKEN_STATIC_ROUTE_DEFAULT_GW TOKEN_DESTINATION TOKEN_GATEWAY
%token TOKEN_CHAIN TOKEN_CONNECTION_STATE TOKEN_ACTION
%token TOKEN_INPUT TOKEN_OUTPUT TOKEN_FORWARD TOKEN_SRCNAT
%token TOKEN_MASQUERADE TOKEN_ENABLED TOKEN_DISABLED
%token TOKEN_ACCEPT TOKEN_DROP TOKEN_REJECT

/* Literal tokens */
%token <str_val> TOKEN_IDENTIFIER TOKEN_STRING TOKEN_BOOL
%token <int_val> TOKEN_NUMBER
%token <str_val> TOKEN_IP_ADDRESS TOKEN_IP_CIDR TOKEN_IP_RANGE
%token <str_val> TOKEN_IPV6_ADDRESS TOKEN_IPV6_CIDR TOKEN_IPV6_RANGE

/* UNKNOWN */
%token TOKEN_UNKNOWN

/* Non-terminals */
%type <str_val> property_name section_name any_identifier
%type <program_val> config
%type <section_val> section section_list
%type <block_val> block statement_list
%type <stmt_val> statement property_statement subsection_statement error_statement
%type <value_val> simple_value value_item
%type <list_val> list_value
%type <expr_val> value
%type <list_val> value_list

/* Start symbol */
%start config

%%

config
    : section_list {
        parser_result = new ProgramDeclaration();
        if ($1 != nullptr) {
            parser_result->add_section($1);
        }
        $$ = parser_result;
    }
    | config section {
        if ($2 != nullptr) {
            parser_result->add_section($2);
        }
        $$ = parser_result;
    }
    ;

section_list
    : section { $$ = $1; }
    ;

section
    : section_name TOKEN_COLON block {
        enter_section($1);
        $$ = new SectionStatement($1, get_section_type($1), $3);
        exit_block();
    }
    ;

section_name
    : TOKEN_DEVICE { $$ = "device"; }
    | TOKEN_INTERFACES { $$ = "interfaces"; }
    | TOKEN_IP { $$ = "ip"; }
    | TOKEN_ROUTING { $$ = "routing"; }
    | TOKEN_FIREWALL { $$ = "firewall"; }
    | TOKEN_SYSTEM { $$ = "system"; }
    ;

block
    : /* empty */ {
        $$ = new BlockStatement();
    }
    | TOKEN_LEFT_BRACE statement_list TOKEN_RIGHT_BRACE {
        $$ = $2;
    }
    | statement_list {
        $$ = $1;
    }
    ;

statement_list
    : statement {
        $$ = new BlockStatement();
        if ($1 != nullptr) {
            $$->add_statement($1);
        }
    }
    | statement_list statement {
        $$ = $1;
        if ($2 != nullptr) {
            $$->add_statement($2);
        }
    }
    ;

statement
    : property_statement { $$ = $1; }
    | subsection_statement { $$ = $1; }
    | error_statement { $$ = nullptr; }
    ;

error_statement
    : TOKEN_SEMICOLON {
        yyerror("Semicolons are not allowed in this DSL");
        YYERROR;
        $$ = nullptr;
    }
    | TOKEN_UNKNOWN {
        yyerror("Unknown token or invalid syntax encountered");
        YYERROR;
        $$ = nullptr;
    }
    | property_name error {
        yyerror("Invalid property assignment syntax");
        YYERROR;
        $$ = nullptr;
    }
    | value error {
        yyerror("Unexpected token after value");
        YYERROR;
        $$ = nullptr;
    }
    | error TOKEN_EQUALS {
        yyerror("Invalid token before equals sign");
        YYERROR;
        $$ = nullptr;
    }
    | error TOKEN_COLON {
        yyerror("Invalid token before colon");
        YYERROR;
        $$ = nullptr;
    }
    ;

property_statement
    : property_name TOKEN_EQUALS value {
        $$ = new PropertyStatement($1, static_cast<Value*>($3));
    }
    ;

subsection_statement
    : any_identifier TOKEN_COLON block {
        enter_block($1);
        $$ = new SectionStatement($1, SectionStatement::SectionType::CUSTOM, $3);
        exit_block();
    }
    | any_identifier TOKEN_COLON TOKEN_SEMICOLON {
        yyerror("Invalid syntax: semicolon after colon. After a section declaration, only a newline or comment is allowed");
        YYERROR;
        $$ = nullptr;
    }
    | any_identifier TOKEN_COLON error {
        yyerror("Invalid syntax after colon. After a section declaration, only a newline or comment is allowed");
        YYERROR;
        $$ = nullptr;
    }
    ;

/* Generic property name to cover all token types that can appear before equals */
property_name
    : TOKEN_IDENTIFIER { $$ = strdup(yytext); }
    | TOKEN_VENDOR { $$ = "vendor"; }
    | TOKEN_TYPE { $$ = "type"; }
    | TOKEN_ADMIN_STATE { $$ = "admin_state"; }
    | TOKEN_DESCRIPTION { $$ = "description"; }
    | TOKEN_ADDRESS { $$ = "address"; }
    | TOKEN_STATIC_ROUTE_DEFAULT_GW { $$ = "static_route_default_gw"; }
    | TOKEN_CHAIN { $$ = "chain"; }
    | TOKEN_CONNECTION_STATE { $$ = "connection_state"; }
    | TOKEN_ACTION { $$ = "action"; }
    | TOKEN_SPEED { $$ = "speed"; }
    | TOKEN_DUPLEX { $$ = "duplex"; }
    | TOKEN_VLAN_ID { $$ = "vlan_id"; }
    | TOKEN_INTERFACE { $$ = "interface"; }
    | TOKEN_DESTINATION { $$ = "destination"; }
    | TOKEN_GATEWAY { $$ = "gateway"; }
    ;

/* Generic identifier to cover all token types that can appear before colon */
any_identifier
    : TOKEN_IDENTIFIER { $$ = strdup(yytext); }
    | TOKEN_ETHERNET { $$ = "ethernet"; }
    | TOKEN_VLAN { $$ = "vlan"; }
    | TOKEN_IP { $$ = "ip"; }
    | TOKEN_DHCP { $$ = "dhcp"; }
    | TOKEN_DHCP_SERVER { $$ = "dhcp_server"; }
    | TOKEN_DHCP_CLIENT { $$ = "dhcp_client"; }
    ;

value
    : simple_value { $$ = $1; }
    | list_value { $$ = $1; }
    ;

simple_value
    : TOKEN_STRING { 
        $$ = new StringValue($1);
    }
    | TOKEN_NUMBER { 
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%d", $1);
        $$ = new NumberValue($1);
    }
    | TOKEN_BOOL { 
        $$ = new BooleanValue(strcmp($1, "true") == 0);
    }
    | TOKEN_IP_ADDRESS { 
        $$ = new IPAddressValue($1);
    }
    | TOKEN_IP_CIDR { 
        $$ = new IPCIDRValue($1);
    }
    | TOKEN_IP_RANGE { 
        // Use a compatible string value if IPRangeValue doesn't exist
        $$ = new StringValue($1);
    }
    | TOKEN_IPV6_ADDRESS { 
        // Use a compatible value type if IPv6AddressValue doesn't exist
        $$ = new StringValue($1); 
    }
    | TOKEN_IPV6_CIDR { 
        // Use a compatible value type if IPv6CIDRValue doesn't exist
        $$ = new StringValue($1);
    }
    | TOKEN_IPV6_RANGE { 
        // Use a compatible value type if IPv6RangeValue doesn't exist
        $$ = new StringValue($1);
    }
    | TOKEN_ENABLED { 
        $$ = new StringValue("enabled");
    }
    | TOKEN_DISABLED { 
        $$ = new StringValue("disabled");
    }
    | TOKEN_INPUT { 
        $$ = new StringValue("input");
    }
    | TOKEN_OUTPUT { 
        $$ = new StringValue("output");
    }
    | TOKEN_FORWARD { 
        $$ = new StringValue("forward");
    }
    | TOKEN_SRCNAT { 
        $$ = new StringValue("srcnat");
    }
    | TOKEN_ACCEPT { 
        $$ = new StringValue("accept");
    }
    | TOKEN_DROP { 
        $$ = new StringValue("drop");
    }
    | TOKEN_REJECT { 
        $$ = new StringValue("reject");
    }
    | TOKEN_MASQUERADE { 
        $$ = new StringValue("masquerade");
    }
    ;

list_value
    : TOKEN_LEFT_BRACKET value_list TOKEN_RIGHT_BRACKET { 
        $$ = $2;
    }
    ;

value_list
    : value_item { 
        // Create a vector of Value pointers for the ListValue constructor
        std::vector<Value*> values;
        values.push_back($1);
        $$ = new ListValue(values);
    }
    | value_list TOKEN_COMMA value_item { 
        // Get the existing list
        $$ = $1;
        
        // Create a new list with all existing values plus the new one
        ValueList values = $$->get_values();
        values.push_back($3);
        
        // Replace the old list with a new one containing all values
        delete $$;
        $$ = new ListValue(values);
    }
    ;

value_item
    : simple_value { $$ = $1; }
    ;

%%

int yyerror(const char* s) {
    fprintf(stderr, "Parse error at line %d: %s\n", line_number, s);
    return 1;
} 