%option noyywrap

%{
    #include "ast.h"
    #include "parse.h"
    #include "syntax.hpp"
    #include "context.h"
    
    // global variables
    int last;
    int lines;

    int make_token(int type, bool set_val = false);
%}

letter [a-zA-Z]
letter_ {letter}|_
digit [0-9]

%%

\n {++lines; last = 0;}
"//"[^\n]* {/*skip*/}
[/][*][^*]*[*]+([^*/][^*]*[*]+)*[/] { //multi-line comments
    int L = strlen(yytext);
    for (int i = 0; i < L; i++) {
        if (yytext[i] == '\n')
            ++lines, last = 0;
        else
            last += 1;
    }
    
}
[\t\r ]+ {last += yyleng;}

continue {return make_token(TCONTIN);}
func {return make_token(TFUNC);}
struct {return make_token(TSTRUCT);}
let {return make_token(TLET);}
if  {return make_token(TIF);}
for {return make_token(TFOR);}
else {return make_token(TELSE);}
while {return make_token(TWHILE);}
const {return make_token(TCONST);}
break {return make_token(TBREAK);}
return {return make_token(TRETURN);}
type {return make_token(TTYPE);}
typeof {return make_token(TTYPEOF);}
impl {return make_token(TIMPL);}


true {return make_token(TTRUE, true);}
false {return make_token(TFALSE, true);}
{letter_}({letter_}|{digit})* {return make_token(NAME, true);}
{digit}({digit})* {return make_token(INT, true);}

0[xX][0-9a-fA-F]+ {return make_token(HEX, true);}
0[bB][01]+ {return make_token(BINARY, true);}
{digit}*"."{digit}+([eE][+-]?{digit}+)? {return make_token(FLOAT, true);}
{digit}+[eE][+-]?{digit}+ {return make_token(FLOAT, true);}
\"([^\"\\]|\\["\\ntbfr])*\" {return make_token(STRING, true);}
'([^'\\]|\\['\\ntbfr])' {return make_token(CHAR, true);}
({letter_}|{digit})+ {
    auto token_id = make_token(INVALID_NAME, true);
    append_lexeme_error("Invalid identifier.", CurrentCursor);
    return token_id;
}

"->" {return make_token(ARROW);}
":" {return make_token(COLON);}
"(" {return make_token(LP);}
")" {return make_token(RP);}
"[" {return make_token(LBRACKET);}
"]" {return make_token(RBRACKET);}
"{" {return make_token(LBRACE);}
"}" {return make_token(RBRACE);}
"+=" {return make_token(ADDEQ);}
"-=" {return make_token(SUBEQ);}
"*=" {return make_token(MULEQ);}
"/=" {return make_token(DIVEQ);}
"+" {return make_token(ADD);}
"-" {return make_token(SUB);}
"*" {return make_token(MUL);}
"/" {return make_token(DIV);}
"%" {return make_token(MOD);}
";" {return make_token(SEMI);}
"," {return make_token(COMMA);}
"." {return make_token(DOT);}
">" {return make_token(GT);}
"<" {return make_token(LT);}
">=" {return make_token(GE);}
"<=" {return make_token(LE);}
"==" {return make_token(EQ);}
"!=" {return make_token(NEQ);}
"&&" {return make_token(AND);}
"||" {return make_token(OR);}
"&" {return make_token(BITAND);}
"|" {return make_token(BITOR);}
"^" {return make_token(XOR);}
"=" {return make_token(ASSIGN);}
[/][*].* {append_lexeme_error("Unclosed comments!", CurrentCursor);}
. {append_lexeme_error(std::string("Undefined symbol: ") + yytext, CurrentCursor);}
%%

/*
0[^xXbB0-7]? {append_error("Invalid numeric literal: " + std::string(yytext), CurrentCursor);}
\"([^\"\\]|\\["\\ntbfr])* {append_error("Unclosed string literal: " + std::string(yytext), CurrentCursor);}
'([^'\\]|\\['\\ntbfr])* {append_error("Unclosed character literal: " + std::string(yytext), CurrentCursor);}
'([^'\\]|\\['\\ntbfr])[^']+' {append_error("Invalid character literal: " + std::string(yytext), CurrentCursor);}
*/
int make_token(int type, bool set_val){

    CurrentCursor = Locator{lines, lines, last, last + yyleng - 1};
    insert_locator(CurrentCursor, std::string(yytext));
    if(set_val){
        std::string val(yytext);

        if(type == NAME && !parser_context.get_ignore()){
            if(parser_context.is_type(val))
            type = TYPENAME;
        }

        auto res = new Token{yytext, type, CurrentCursor};
        yylval.str = res;
    }
    else {
        // auto res = new Token{nullptr, type, {lines, lines, last, last + yyleng - 1}};
        // yylval.str = res;
        yylval.token_id = {lines, lines, last, last + yyleng - 1};
    }
    last += yyleng;
    return type;
}

void set_input(FILE* file){
    yyrestart(file);
}