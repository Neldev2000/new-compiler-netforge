%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <vector>
    #include "datatype.hpp"
    #include "declaration.hpp"
    #include "expression.hpp"
    #include "statement.hpp"
    #include "parser.tab.h"

    int line_number = 1;
    int column_number = 0;
    
    // Indentation handling - define globals only in scanner
    #ifndef SCANNER_GLOBALS
    #define SCANNER_GLOBALS
    std::vector<int> indent_stack{0}; // Start with indent level 0
    std::vector<int> token_queue;    // Buffer for INDENT/DEDENT tokens
    #endif
    
    int current_indent = 0;
    bool at_line_start = true;
    bool eof_handled = false;  // Flag to track if we've handled EOF

    // Function to check and return tokens from the queue
    int check_token_queue() {
        if (!token_queue.empty()) {
            int token = token_queue.front();
            token_queue.erase(token_queue.begin());
            return token;
        }
        return 0;
    }

    // Handle EOF - generate DEDENT tokens for any open indentation levels
    void handle_eof() {
        if (eof_handled) return;
        
        // First add a NEWLINE if we're not at the start of a line
        if (!at_line_start) {
            token_queue.push_back(TOKEN_NEWLINE);
        }
        
        // Add DEDENT tokens to get back to indentation level 0
        while (indent_stack.size() > 1) {  // Keep the base level 0
            indent_stack.pop_back();
            token_queue.push_back(TOKEN_DEDENT);
            fprintf(stderr, "DEBUG: SCANNER Adding DEDENT token at EOF\n");
        }
        
        eof_handled = true;
    }

    // Declare the internal lexer function
    static int yylex_internal();
    
    // Define the wrapper function
    int yylex() {
        // First check if we have any tokens in the queue
        int token = check_token_queue();
        if (token != 0) {
            return token;
        }
        
        // Call the flex-generated lexer
        token = yylex_internal();
        
        // If we reached EOF, handle any pending dedent tokens
        if (token == 0) {
            handle_eof();
            token = check_token_queue();
        }
        
        return token;
    }

    // Define the flex-generated lexer
    #define YY_DECL static int yylex_internal()
%}

/* Options */
%option noyywrap
%option yylineno
%option nounput
%option noinput

/* Regular definitions */
WHITESPACE      [ \t\r]+
NEWLINE         \n
DIGIT           [0-9]
LETTER          [a-zA-Z]
BOOL            true|false
IDENTIFIER      {LETTER}[a-zA-Z0-9_]*
INTERFACE_ID    ether[0-9]+|wlan[0-9]+|bridge[0-9]+|vlan[0-9]+|bond[0-9]+
NUMBER          {DIGIT}+
STRING          \"[^\"]*\"
COMMENT         #[^\n]*
MULTILINE       \"\"\"([^"]|\n|\"[^"]|\"\"[^"])*\"\"\"

/* IPv4 definitions */
OCTET           ([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])
IP_ADDRESS      {OCTET}\.{OCTET}\.{OCTET}\.{OCTET}
IP_CIDR         {IP_ADDRESS}\/(3[0-2]|[1-2][0-9]|[0-9])
IP_RANGE        {IP_ADDRESS}\-{IP_ADDRESS}

/* IPv6 definitions */
HEX             [0-9A-Fa-f]
HEX_QUAD        {HEX}{1,4}
IPV6_SEG        {HEX_QUAD}
IPV6_FULL       {IPV6_SEG}:{IPV6_SEG}:{IPV6_SEG}:{IPV6_SEG}:{IPV6_SEG}:{IPV6_SEG}:{IPV6_SEG}:{IPV6_SEG}
IPV6_COMPRESSED (({IPV6_SEG}:){6}{IPV6_SEG}|::({IPV6_SEG}:){5}{IPV6_SEG}|({IPV6_SEG})?::({IPV6_SEG}:){4}{IPV6_SEG}|(({IPV6_SEG}:){0,1}{IPV6_SEG})?::({IPV6_SEG}:){3}{IPV6_SEG}|(({IPV6_SEG}:){0,2}{IPV6_SEG})?::({IPV6_SEG}:){2}{IPV6_SEG}|(({IPV6_SEG}:){0,3}{IPV6_SEG})?::{IPV6_SEG}:{IPV6_SEG}|(({IPV6_SEG}:){0,4}{IPV6_SEG})?::{IPV6_SEG}|(({IPV6_SEG}:){0,5}{IPV6_SEG})?::{IPV6_SEG}|(({IPV6_SEG}:){0,6}{IPV6_SEG})?::)
IPV6_ADDRESS    {IPV6_FULL}|{IPV6_COMPRESSED}
IPV6_CIDR       {IPV6_ADDRESS}\/(12[0-8]|1[0-1][0-9]|[1-9][0-9]|[0-9])
IPV6_RANGE      {IPV6_ADDRESS}\-{IPV6_ADDRESS}

%x INDENT_STATE

%%

<INITIAL>{NEWLINE} {
    line_number++;
    at_line_start = true;
    BEGIN(INDENT_STATE);
    return TOKEN_NEWLINE;
}

<INDENT_STATE>{WHITESPACE} {
    /* Count spaces for indentation */
    if (at_line_start) {
        current_indent = yyleng;
        fprintf(stderr, "DEBUG: SCANNER Indent level %d\n", current_indent);
    }
}

<INDENT_STATE>{COMMENT} {
    /* Skip comments at the start of line and stay in INDENT_STATE */
}

<INDENT_STATE>{NEWLINE} {
    /* Skip empty lines, but still count line numbers */
    line_number++;
    current_indent = 0;  // Reset indent for empty lines
}

<INDENT_STATE>. {
    /* End of whitespace - process indentation changes */
    yyless(0); /* Put back the character we just read */
    
    /* Compare with previous indent level */
    if (current_indent > indent_stack.back()) {
        /* Indentation increased - emit INDENT token */
        indent_stack.push_back(current_indent);
        fprintf(stderr, "DEBUG: SCANNER Emitting INDENT token (level %d)\n", current_indent);
        at_line_start = false;
        BEGIN(INITIAL);
        return TOKEN_INDENT;
    } else if (current_indent < indent_stack.back()) {
        /* Indentation decreased - might need multiple DEDENT tokens */
        bool found_matching_indent = false;
        for (int i = indent_stack.size() - 1; i >= 0; i--) {
            if (current_indent == indent_stack[i]) {
                found_matching_indent = true;
                break;
            }
        }
        
        if (!found_matching_indent) {
            fprintf(stderr, "ERROR: Invalid dedentation level %d\n", current_indent);
            /* Invalid dedentation - indentation error */
            return TOKEN_UNKNOWN;
        }
        
        /* Pop one level and return a DEDENT token */
        indent_stack.pop_back();
        fprintf(stderr, "DEBUG: SCANNER Emitting DEDENT token (level %d)\n", current_indent);
        
        /* If we need more DEDENTs, queue them */
        while (current_indent < indent_stack.back()) {
            indent_stack.pop_back();
            token_queue.push_back(TOKEN_DEDENT);
            fprintf(stderr, "DEBUG: SCANNER Queuing DEDENT token (level %d)\n", indent_stack.back());
        }
        
        at_line_start = false;
        BEGIN(INITIAL);
        return TOKEN_DEDENT;
    } else {
        /* Same indentation level - no token needed */
        at_line_start = false;
        BEGIN(INITIAL);
    }
}

{WHITESPACE}    { /* Ignore whitespace within lines */ }
{COMMENT}       { /* Ignore single line comments */ }
{MULTILINE}     { 
                    /* Count newlines in multiline comment */
                    char *p = yytext;
                    while (*p) {
                        if (*p == '\n') line_number++;
                        p++;
                    }
                    /* Ignore multiline comment */
                }

":"             { return TOKEN_COLON; }
"="             { return TOKEN_EQUALS; }
"["             { return TOKEN_LEFT_BRACKET; }
"]"             { return TOKEN_RIGHT_BRACKET; }
"{"             { return TOKEN_LEFT_BRACE; }
"}"             { return TOKEN_RIGHT_BRACE; }
","             { return TOKEN_COMMA; }
"/"             { return TOKEN_SLASH; }
"-"             { return TOKEN_MINUS; }
"."             { return TOKEN_DOT; }
";"             { return TOKEN_SEMICOLON; }

"device"        { return TOKEN_DEVICE; }
"vendor"        { return TOKEN_VENDOR; }
"model"         { return TOKEN_MODEL; }
"interfaces"    { return TOKEN_INTERFACES; }
"ip"            { return TOKEN_IP; }
"routing"       { return TOKEN_ROUTING; }
"firewall"      { return TOKEN_FIREWALL; }
"system"        { return TOKEN_SYSTEM; }
"type"          { return TOKEN_TYPE; }
"admin_state"   { return TOKEN_ADMIN_STATE; }
"description"   { return TOKEN_DESCRIPTION; }
"ethernet"      { return TOKEN_ETHERNET; }
"speed"         { return TOKEN_SPEED; }
"duplex"        { return TOKEN_DUPLEX; }
"vlan"          { return TOKEN_VLAN; }
"vlan_id"       { return TOKEN_VLAN_ID; }
"interface"     { return TOKEN_INTERFACE; }
"address"       { return TOKEN_ADDRESS; }
"dhcp"          { return TOKEN_DHCP; }
"dhcp_client"   { return TOKEN_DHCP_CLIENT; }
"dhcp_server"   { return TOKEN_DHCP_SERVER; }
"static_route_default_gw" { return TOKEN_STATIC_ROUTE_DEFAULT_GW; }
"destination"   { return TOKEN_DESTINATION; }
"gateway"       { return TOKEN_GATEWAY; }
"chain"         { return TOKEN_CHAIN; }
"connection_state" { return TOKEN_CONNECTION_STATE; }
"action"        { return TOKEN_ACTION; }
"input"         { return TOKEN_INPUT; }
"output"        { return TOKEN_OUTPUT; }
"forward"       { return TOKEN_FORWARD; }
"srcnat"        { return TOKEN_SRCNAT; }
"masquerade"    { return TOKEN_MASQUERADE; }
"enabled"       { return TOKEN_ENABLED; }
"disabled"      { return TOKEN_DISABLED; }
"accept"        { return TOKEN_ACCEPT; }
"drop"          { return TOKEN_DROP; }
"reject"        { return TOKEN_REJECT; }
"out_interface" { return TOKEN_OUT_INTERFACE; }
"out-interface" { return TOKEN_OUT_INTERFACE; }
"in_interface"  { return TOKEN_IN_INTERFACE; }
"in-interface"  { return TOKEN_IN_INTERFACE; }
"src_address"   { return TOKEN_SRC_ADDRESS; }
"src-address"   { return TOKEN_SRC_ADDRESS; }
"dst_address"   { return TOKEN_DST_ADDRESS; }
"dst-address"   { return TOKEN_DST_ADDRESS; }
"src_port"      { return TOKEN_SRC_PORT; }
"src-port"      { return TOKEN_SRC_PORT; }
"dst_port"      { return TOKEN_DST_PORT; }
"dst-port"      { return TOKEN_DST_PORT; }
"to_addresses"  { return TOKEN_TO_ADDRESSES; }
"to-addresses"  { return TOKEN_TO_ADDRESSES; }
"to_ports"      { return TOKEN_TO_PORTS; }
"to-ports"      { return TOKEN_TO_PORTS; }

{IPV6_CIDR}     { yylval.str_val = strdup(yytext); return TOKEN_IPV6_CIDR; }
{IPV6_RANGE}    { yylval.str_val = strdup(yytext); return TOKEN_IPV6_RANGE; }
{IPV6_ADDRESS}  { yylval.str_val = strdup(yytext); return TOKEN_IPV6_ADDRESS; }
{IP_CIDR}       { yylval.str_val = strdup(yytext); return TOKEN_IP_CIDR; }
{IP_RANGE}      { yylval.str_val = strdup(yytext); return TOKEN_IP_RANGE; }
{IP_ADDRESS}    { yylval.str_val = strdup(yytext); return TOKEN_IP_ADDRESS; }
{BOOL}          { yylval.str_val = strdup(yytext); return TOKEN_BOOL; }
{INTERFACE_ID}  { 
                    fprintf(stderr, "DEBUG: SCANNER Found interface identifier: '%s'\n", yytext);
                    yylval.str_val = strdup(yytext); 
                    return TOKEN_IDENTIFIER; 
                }
{IDENTIFIER}    { yylval.str_val = strdup(yytext); return TOKEN_IDENTIFIER; }
{NUMBER}        { yylval.int_val = atoi(yytext); return TOKEN_NUMBER; }
{STRING}        { yylval.str_val = strdup(yytext); return TOKEN_STRING; }

.               { return TOKEN_UNKNOWN; }

<<EOF>> {
    /* At end of file, ensure all blocks are closed */
    return 0;  // Return 0 to trigger EOF handling in yylex()
}

%%
