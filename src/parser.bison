%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "datatype.hpp"
#include "declaration.hpp"
#include "expression.hpp"
#include "statement.hpp"
#include "section_factory.hpp"

extern int yylex();  // Use standard yylex - it will internally handle our token queue
extern char* yytext;
extern int line_number;
extern int column_number; // Track indentation
extern FILE* yyin;
int yyerror(const char* s);

// Extern declarations for scanner globals
extern std::vector<int> indent_stack;
extern std::vector<int> token_queue;

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
    int indent_val;
}

/* Tokens from flex scanner */
%token TOKEN_COLON TOKEN_EQUALS TOKEN_LEFT_BRACKET TOKEN_RIGHT_BRACKET
%token TOKEN_LEFT_BRACE TOKEN_RIGHT_BRACE TOKEN_COMMA TOKEN_SLASH
%token TOKEN_MINUS TOKEN_DOT
%token TOKEN_SEMICOLON
%token TOKEN_INDENT TOKEN_DEDENT TOKEN_NEWLINE   /* Indentation tokens */

/* Keyword tokens */
%token TOKEN_DEVICE TOKEN_VENDOR TOKEN_MODEL TOKEN_HOSTNAME TOKEN_INTERFACES TOKEN_IP TOKEN_ROUTING
%token TOKEN_FIREWALL TOKEN_SYSTEM TOKEN_TYPE TOKEN_ADMIN_STATE
%token TOKEN_DESCRIPTION TOKEN_ETHERNET TOKEN_SPEED TOKEN_DUPLEX
%token TOKEN_VLAN TOKEN_VLAN_ID TOKEN_INTERFACE TOKEN_ADDRESS
%token TOKEN_DHCP TOKEN_DHCP_CLIENT TOKEN_DHCP_SERVER
%token TOKEN_STATIC_ROUTE_DEFAULT_GW TOKEN_DESTINATION TOKEN_GATEWAY
%token TOKEN_CHAIN TOKEN_CONNECTION_STATE TOKEN_ACTION
%token TOKEN_INPUT TOKEN_OUTPUT TOKEN_FORWARD TOKEN_SRCNAT
%token TOKEN_MASQUERADE TOKEN_ENABLED TOKEN_DISABLED
%token TOKEN_ACCEPT TOKEN_DROP TOKEN_REJECT
%token TOKEN_OUT_INTERFACE TOKEN_IN_INTERFACE TOKEN_SRC_ADDRESS TOKEN_DST_ADDRESS
%token TOKEN_SRC_PORT TOKEN_DST_PORT TOKEN_TO_ADDRESSES TOKEN_TO_PORTS

/* Literal tokens */
%token <str_val> TOKEN_IDENTIFIER TOKEN_STRING TOKEN_BOOL
%token <int_val> TOKEN_NUMBER
%token <str_val> TOKEN_IP_ADDRESS TOKEN_IP_CIDR TOKEN_IP_RANGE
%token <str_val> TOKEN_IPV6_ADDRESS TOKEN_IPV6_CIDR TOKEN_IPV6_RANGE

/* UNKNOWN */
%token TOKEN_UNKNOWN

/* Non-terminals */
%type <str_val> property_name section_name identifier
%type <program_val> config
%type <section_val> section section_list
%type <block_val> block statement_list indented_block
%type <stmt_val> statement subsection
%type <value_val> simple_value value_item
%type <list_val> list_value
%type <expr_val> value
%type <list_val> value_list

/* Define precedence */
%left TOKEN_COLON
%left TOKEN_EQUALS

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
    | config TOKEN_NEWLINE section {
        if ($3 != nullptr) {
            parser_result->add_section($3);
        }
        $$ = parser_result;
    }
    | config TOKEN_NEWLINE {
        // Allow trailing newlines in a config
        $$ = parser_result;
    }
    | config TOKEN_DEDENT {
        // Handle dedents at the end of the file
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
    | TOKEN_NEWLINE section { $$ = $2; }
    ;

section
    : section_name TOKEN_COLON indented_block {
        SectionStatement::SectionType type = get_section_type($1);
        $$ = SectionFactory::create_section($1, type, $3);
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

indented_block
    : /* empty */ {
        $$ = new BlockStatement();
    }
    | TOKEN_NEWLINE TOKEN_INDENT statement_list TOKEN_DEDENT {
        $$ = $3;
    }
    | TOKEN_NEWLINE TOKEN_INDENT TOKEN_DEDENT {
        /* Empty block with just indentation and dedentation */
        $$ = new BlockStatement();
    }
    ;

statement_list
    : statement {
        $$ = new BlockStatement();
        if ($1 != nullptr) {
            $$->add_statement($1);
        }
    }
    | statement_list TOKEN_NEWLINE {
        /* Allow trailing newlines in a block without requiring a statement */
        $$ = $1;
    }
    | statement_list statement {
        $$ = $1;
        if ($2 != nullptr) {
            $$->add_statement($2);
        }
    }
    | statement_list TOKEN_NEWLINE statement {
        $$ = $1;
        if ($3 != nullptr) {
            $$->add_statement($3);
        }
    }
    ;

statement
    : property_name TOKEN_EQUALS value {
        $$ = new PropertyStatement($1, static_cast<Value*>($3));
    }
    | subsection {
        $$ = $1;
    }
    | TOKEN_SEMICOLON {
        yyerror("Semicolons are not allowed in this DSL");
        YYERROR;
        $$ = nullptr;
    }
    | TOKEN_UNKNOWN {
        yyerror("Unknown token or invalid syntax encountered");
        YYERROR;
        $$ = nullptr;
    }
    | error {
        yyerror("Invalid syntax");
        YYERROR;
        $$ = nullptr;
    }
    ;

subsection
    : identifier TOKEN_COLON indented_block {
 
        SectionStatement* section = SectionFactory::create_section($1, SectionStatement::SectionType::CUSTOM, $3);

        $$ = section;
    }
    ;

/* Generic property name that can appear before equals */
property_name
    : TOKEN_IDENTIFIER { $$ = strdup(yytext); }
    | TOKEN_VENDOR { $$ = "vendor"; }
    | TOKEN_MODEL { $$ = "model"; }
    | TOKEN_HOSTNAME {$$ = "hostname";}
    | TOKEN_TYPE { $$ = "type"; }
    | TOKEN_ADMIN_STATE { $$ = "admin_state"; }
    | TOKEN_DESCRIPTION { $$ = "comment"; }
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
    | TOKEN_OUT_INTERFACE { $$ = "out_interface"; }
    | TOKEN_IN_INTERFACE { $$ = "in_interface"; }
    | TOKEN_SRC_ADDRESS { $$ = "src_address"; }
    | TOKEN_DST_ADDRESS { $$ = "dst_address"; }
    | TOKEN_SRC_PORT { $$ = "src_port"; }
    | TOKEN_DST_PORT { $$ = "dst_port"; }
    | TOKEN_TO_ADDRESSES { $$ = "to_addresses"; }
    | TOKEN_TO_PORTS { $$ = "to_ports"; }
    ;

/* Generic identifier for tokens that can appear before colon */
identifier
    : TOKEN_IDENTIFIER { 
        $$ = $1; // Use the value passed from the scanner ($1) instead of yytext
      
    }
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
        $$ = new StringValue($1);
    }
    | TOKEN_IPV6_ADDRESS { 
        $$ = new StringValue($1); 
    }
    | TOKEN_IPV6_CIDR { 
        $$ = new StringValue($1);
    }
    | TOKEN_IPV6_RANGE { 
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
        std::vector<Value*> values;
        values.push_back($1);
        $$ = new ListValue(values);
    }
    | value_list TOKEN_COMMA value_item { 
        $$ = $1;
        ValueList values = $$->get_values();
        values.push_back($3);
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