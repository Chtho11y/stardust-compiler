%option noyywrap

%{
    #include "ast.h"
    #include "parse.h"
    #include "syntax.hpp"
    
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
[\t\r ]+ {last += yyleng;}

func {return make_token(TFUNC);}
struct {return make_token(TSTRUCT);}
let {return make_token(TLET);}
if  {return make_token(TIF);}
else {return make_token(TELSE);}


{letter_}({letter_}|{digit})* {return make_token(NAME, true);}
{digit}({digit})* {return make_token(INT, true);}

"->" {return make_token(ARROW);}
":" {return make_token(COLON);}
"(" {return make_token(LP);}
")" {return make_token(RP);}
"[" {return make_token(LBRACKET);}
"]" {return make_token(RBRACKET);}
"{" {return make_token(LBRACE);}
"}" {return make_token(RBRACE);}
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
"=" {return make_token(ASSIGN);}
%%

int make_token(int type, bool set_val){
    if(set_val){
        auto res = new Token{yytext, type, {lines, lines, last, last + yyleng - 1}};
        yylval.str = res;
    }
    last += yyleng;
    return type;
}

void set_input(FILE* file){
    yyrestart(file);
}