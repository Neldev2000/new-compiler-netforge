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
    #define YY_USER_ACTION { printf("Scanner: token at line %d: %s\n", line_number, yytext); }
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

":"             { printf("COLON\n"); return TOKEN_COLON; }
"="             { printf("EQUALS\n"); return TOKEN_EQUALS; }
"["             { printf("LEFT_BRACKET\n"); return TOKEN_LEFT_BRACKET; }
"]"             { printf("RIGHT_BRACKET\n"); return TOKEN_RIGHT_BRACKET; }
"{"             { printf("LEFT_BRACE\n"); return TOKEN_LEFT_BRACE; }
"}"             { printf("RIGHT_BRACE\n"); return TOKEN_RIGHT_BRACE; }
","             { printf("COMMA\n"); return TOKEN_COMMA; }
"/"             { printf("SLASH\n"); return TOKEN_SLASH; }
"-"             { printf("MINUS\n"); return TOKEN_MINUS; }
"."             { printf("DOT\n"); return TOKEN_DOT; }
";"             { printf("SEMICOLON\n"); return TOKEN_SEMICOLON; }

"device"        { printf("DEVICE\n"); return TOKEN_DEVICE; }
"vendor"        { printf("VENDOR\n"); return TOKEN_VENDOR; }
"interfaces"    { printf("INTERFACES\n"); return TOKEN_INTERFACES; }
"ip"            { printf("IP\n"); return TOKEN_IP; }
"routing"       { printf("ROUTING\n"); return TOKEN_ROUTING; }
"firewall"      { printf("FIREWALL\n"); return TOKEN_FIREWALL; }
"system"        { printf("SYSTEM\n"); return TOKEN_SYSTEM; }
"type"          { printf("TYPE\n"); return TOKEN_TYPE; }
"admin_state"   { printf("ADMIN_STATE\n"); return TOKEN_ADMIN_STATE; }
"description"   { printf("DESCRIPTION\n"); return TOKEN_DESCRIPTION; }
"ethernet"      { printf("ETHERNET\n"); return TOKEN_ETHERNET; }
"speed"         { printf("SPEED\n"); return TOKEN_SPEED; }
"duplex"        { printf("DUPLEX\n"); return TOKEN_DUPLEX; }
"vlan"          { printf("VLAN\n"); return TOKEN_VLAN; }
"vlan_id"       { printf("VLAN_ID\n"); return TOKEN_VLAN_ID; }
"interface"     { printf("INTERFACE\n"); return TOKEN_INTERFACE; }
"address"       { printf("ADDRESS\n"); return TOKEN_ADDRESS; }
"dhcp"          { printf("DHCP\n"); return TOKEN_DHCP; }
"dhcp_client"   { printf("DHCP_CLIENT\n"); return TOKEN_DHCP_CLIENT; }
"dhcp_server"   { printf("DHCP_SERVER\n"); return TOKEN_DHCP_SERVER; }
"static_route_default_gw" { printf("STATIC_ROUTE_DEFAULT_GW\n"); return TOKEN_STATIC_ROUTE_DEFAULT_GW; }
"destination"   { printf("DESTINATION\n"); return TOKEN_DESTINATION; }
"gateway"       { printf("GATEWAY\n"); return TOKEN_GATEWAY; }
"chain"         { printf("CHAIN\n"); return TOKEN_CHAIN; }
"connection_state" { printf("CONNECTION_STATE\n"); return TOKEN_CONNECTION_STATE; }
"action"        { printf("ACTION\n"); return TOKEN_ACTION; }
"input"         { printf("INPUT\n"); return TOKEN_INPUT; }
"output"        { printf("OUTPUT\n"); return TOKEN_OUTPUT; }
"forward"       { printf("FORWARD\n"); return TOKEN_FORWARD; }
"srcnat"        { printf("SRCNAT\n"); return TOKEN_SRCNAT; }
"masquerade"    { printf("MASQUERADE\n"); return TOKEN_MASQUERADE; }
"enabled"       { printf("ENABLED\n"); return TOKEN_ENABLED; }
"disabled"      { printf("DISABLED\n"); return TOKEN_DISABLED; }
"accept"        { printf("ACCEPT\n"); return TOKEN_ACCEPT; }
"drop"          { printf("DROP\n"); return TOKEN_DROP; }
"reject"        { printf("REJECT\n"); return TOKEN_REJECT; }

{IPV6_CIDR}     { printf("IPV6_CIDR: %s\n", yytext); yylval.str_val = strdup(yytext); return TOKEN_IPV6_CIDR; }
{IPV6_RANGE}    { printf("IPV6_RANGE: %s\n", yytext); yylval.str_val = strdup(yytext); return TOKEN_IPV6_RANGE; }
{IPV6_ADDRESS}  { printf("IPV6_ADDRESS: %s\n", yytext); yylval.str_val = strdup(yytext); return TOKEN_IPV6_ADDRESS; }
{IP_CIDR}       { printf("IP_CIDR: %s\n", yytext); yylval.str_val = strdup(yytext); return TOKEN_IP_CIDR; }
{IP_RANGE}      { printf("IP_RANGE: %s\n", yytext); yylval.str_val = strdup(yytext); return TOKEN_IP_RANGE; }
{IP_ADDRESS}    { printf("IP_ADDRESS: %s\n", yytext); yylval.str_val = strdup(yytext); return TOKEN_IP_ADDRESS; }
{BOOL}          { printf("BOOL: %s\n", yytext); yylval.str_val = strdup(yytext); return TOKEN_BOOL; }
{IDENTIFIER}    { printf("IDENTIFIER: %s\n", yytext); yylval.str_val = strdup(yytext); return TOKEN_IDENTIFIER; }
{NUMBER}        { printf("NUMBER: %s\n", yytext); yylval.int_val = atoi(yytext); return TOKEN_NUMBER; }
{STRING}        { printf("STRING: %s\n", yytext); yylval.str_val = strdup(yytext); return TOKEN_STRING; }

.               { printf("ERROR: unexpected character %s at line %d\n", yytext, line_number); return TOKEN_UNKNOWN; }

%% 