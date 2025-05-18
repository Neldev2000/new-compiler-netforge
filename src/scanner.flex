%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "datatype.hpp"
    #include "declaration.hpp"
    #include "expression.hpp"
    #include "statement.hpp"
    #include "parser.tab.h"

    int line_number = 1;
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

%%

{WHITESPACE}    { /* Ignore whitespace */ }
{NEWLINE}       { line_number++; }
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

%% 