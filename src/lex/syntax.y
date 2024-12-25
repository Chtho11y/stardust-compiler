%{
    // #include "lex.yy.cpp"
    #include "parse.h"
    #include "ast.h"
    #include "context.h"
    #include "error.h"

    Locator CurrentCursor;

    bool EOF_FLAG = false;
    void yyerror(const char*);
    #define yyerrok1 (yyerrstatus=1)
    #define echo_error(s) \
        set_error_message(s)
    #define Lc(loc) Locator{(loc).line_st, (loc).line_ed, (loc).col_l, (loc).col_r}
    #define next_loc(loc) ((Lc(loc)).is_empty() ? CurrentCursor : get_next_locator(Lc(loc)))
%}
/* %glr-parser */
%union{
    Token* str;
    AstNode* node;
    LocatorBuffer token_id;
};

%token<str> INT NAME TYPENAME INVALID_NAME TTRUE TFALSE TNULL
%token<str> HEX BINARY STRING FLOAT CHAR 
%token<token_id> ADD SUB MUL DIV MOD DOT LT GT LE GE EQ NEQ AND OR XOR BITAND BITOR NOT
%token<token_id> ADDEQ SUBEQ MULEQ DIVEQ
%token<token_id> TFUNC TLET TSTRUCT TIF TELSE TWHILE TCONST TRETURN TBREAK TFOR TCONTIN TTYPE TTYPEOF TIMPL
%token<token_id> SEMI COLON LP RP LBRACE RBRACE ASSIGN ARROW COMMA LBRACKET RBRACKET
%token<token_id> UNCLOSED_COMMENT
%type<token_id> block_begin block_end

%type<node> program
%type<node> ext_decl single_decl
%type<node> var_decl func_decl struct_decl struct_impl
%type<node> struct_decl_ident generic_decl generic_params func_decl_ident func_decl_ret_type
%type<node> type_def
%type<node> ident ident_all ident_type_list ident_type_member ident_value_list  ident_value_member member_func_list
%type<node> const_desc 
%type<node> array_instance struct_instance 
%type<node> opt_type_desc type_desc type_item type_list type_name
%type<node> block block_ret block_no_ret
%type<node> stmts stmt ctrl_ret ctrl_no_ret return_stmt
%type<node> expr expr_unit expr_list item_list
%type<node> item
%type<node> literal


%left TIF
%left TELSE

%right ASSIGN ADDEQ SUBEQ MULEQ DIVEQ
%left COMMA
%left OR
%left AND
%left BITOR
%left XOR
%left BITAND
%left LT GT LE GE EQ NEQ
%left ADD SUB
%left MUL DIV MOD
%left NOT
%left DOT
%left RP
%left ARROW

// todo : add destructor
// %destructor { 
    
// } <token_id>

// %destructor {
//     delete $$;
// } <str>

// %destructor {
//     delete $$;
// } <node>
%%

program: {
        parser_context.push_block_env();
        auto& tps = get_prim_list();
        for(auto tp: tps)
            parser_context.set_type(tp->to_string());
        parser_context.set_type("int");
        parser_context.set_type("void");
    }
    ext_decl{
        program_root = new BlockNode(Program); 
        program_root->append($2);
        parser_context.pop_block_env();
    }
    ;

ext_decl:{
        $$ = new BlockNode(ExtDecl);
        inject_builtin_func((BlockNode*)$$);
    }
    | ext_decl SEMI {$$ = $1; $$->append_loc(Lc($2)); yyerrok;}
    | ext_decl single_decl {$$ = $1; $$->append($2); }
    | ext_decl RBRACE {
        $$ = $1;
        append_syntax_error("Expected block before }.", Lc($2));
        $$->append(new AstNode(Err, Lc($2)));
    }
    | ext_decl RBRACKET {
        $$ = $1;
        append_syntax_error("Expected expression before ]", Lc($2));
        $$->append(new AstNode(Err, Lc($2)));
    }
    | ext_decl RP {
        $$ = $1;
        append_syntax_error("Expected expression before )", Lc($2));
        $$->append(new AstNode(Err, Lc($2)));
    }
    | ext_decl error {
        $$ = $1;
        append_syntax_error("Invalid external declaration.", CurrentCursor);
        $$->append(new AstNode(Err, next_loc($$->loc)));
    }
    ;

single_decl: var_decl | func_decl | struct_decl | type_def | struct_impl;

/**************type**************/
opt_type_desc: 
    {$$ = new AstNode(TypeDesc, "#auto"); }
    |COLON type_desc {$$ = $2; $$->append_loc(Lc($1));}
    |type_desc {
        #ifdef AUTO_FIX
        $$ = $1;
        append_syntax_error("Missing :.", $$->loc, 1);
        #else
        $$ = $1;
        $$->str = "#err";
        append_syntax_error("Missing :.", $$->loc);
        #endif
    }
    |COLON error {
        $$ = new AstNode(TypeDesc, "#err");
        $$->append_loc(Lc($1));
        append_syntax_error("Invalid type description.", next_loc($1));
        // yyerrok;
    }
    ;

type_desc: 
      type_item
    | LP type_list RP ARROW type_desc {
        $$ = new AstNode(TypeDesc, "()");
        $$->append($2);
        $$->append($5);
        $$->append_loc(Lc($1));
        $$->append_loc(Lc($3));
        $$->append_loc(Lc($4));
    }
    | LP type_list RP ARROW error {
        $$ = new AstNode(TypeDesc, "#err");
        $$->append_loc($1);
        $$->append($2);
        $$->append_loc($3);
        $$->append_loc($4);
        append_syntax_error("Invalid return type.", next_loc($4));
    }
    | LP type_list ARROW error {
        $$ = new AstNode(TypeDesc, "#err");
        $$->append_loc($1);
        $$->append($2);
        $$->append_loc($3);
        append_syntax_error("Missing ).", next_loc($3));
    }
    | LP type_list ARROW type_desc{
        #ifdef AUTO_FIX
        $$ = new AstNode(TypeDesc, "()");
        #else
        $$ = new AstNode(TypeDesc, "#err");
        #endif
        $$->append_loc($1);
        $$->append($2);
        $$->append_loc($3);
        $$->append_loc($3);
        $$->append($4);
        #ifdef AUTO_FIX
        append_syntax_error("Missing ).", $4->loc, 1);
        #else 
        append_syntax_error("Missing ).", $4->loc);
        #endif
    }
    ;

type_item: 
    type_name {$$ = create_node_from(TypeDesc, $1);}
    | type_name LT type_list GT {
        $$ = new AstNode(TypeDesc, "<>");
        $$->append($1);
        $$->append($3);
        $$->append_loc($2);
        $$->append_loc($4);
    }
    | LP type_list RP{$$ = $2; $$->append_loc($1); $$->append_loc($3);}
    | type_item LBRACKET expr RBRACKET{
        $$ = new AstNode(TypeDesc, "[]");
        $$->append($1);
        $$->append($3);
        $$->append_loc($2);
        $$->append_loc($4);
    }
    | type_item MUL{
        $$ = new AstNode(TypeDesc, "*"); 
        $$->append($1);
        $$->append_loc($2);
    }
    | TTYPEOF LP expr RP {
        $$ = new AstNode(TypeDesc, "&");
        $$->append($3);
        $$->append_loc($1);
        $$->append_loc($2);
        $$->append_loc($4);
    }
    | LP type_list error {
        $$ = new AstNode(TypeDesc, "#err");
        $$->append_loc($1);
        $$->append($2);
        append_syntax_error("Missing )", next_loc($2->loc));
    }
    | type_item LBRACKET expr error {
        $$ = new AstNode(TypeDesc, "#err");
        $$->append($1);
        $$->append_loc($2);
        $$->append($3);
        append_syntax_error("Missing ]", next_loc($3->loc));
    }
    | TTYPEOF LP expr error {
        $$ = new AstNode(TypeDesc, "#err");
        $$->append_loc($1);
        $$->append_loc($2);
        $$->append($3);
        append_syntax_error("Missing )", next_loc($3->loc));
    }
    ;


type_list: 
    {$$ = new AstNode(TypeList); }
    |type_desc{$$ = create_node_from(TypeList, $1); }
    |type_list COMMA type_desc {$$ = $1; $$->append_loc($2); $$->append($3);}
    |type_list COMMA ident {
        $$ = $1;
        $$->type = Err;
        $$->append_loc($2);
        $$->append($3);
        append_syntax_error("Undeclared type.", $3->loc);
    }
    |type_list COMMA {
        $$ = $1;
        $$->append_loc($2);
        #ifndef AUTO_FIX
        $$->type = Err;
        append_syntax_error("Missing type description.", Lc($2));
        #else
        append_syntax_error("Missing type description.", Lc($2), 1);
        #endif
        // delete $1;
    }
    ;

type_name: 
    TYPENAME {
        $$ = new AstNode(Identifier, *$1); delete $1;
    }
    /* | TYPENAME LT type_list GT{
        $$ = new AstNode(TypeDesc, "<>");
        $$->append(new AstNode(Identifier, *$1)); delete $1;
        $$->append($3);
        $$->append_loc($2);
        $$->append_loc($4);
    } */
    ;


/**************variable declaration**************/
var_decl:
    const_desc ident_all opt_type_desc ASSIGN expr SEMI{
        $$ = new AstNode(VarDecl); 
        $$->append($1);
        $$->append($2); 
        $$->append($3);
        $$->append_loc($4);
        $$->append($5);
        $$->append_loc($6);
        parser_context.set_var($2->str);
    }
    | const_desc ident_all opt_type_desc SEMI{
        $$ = new AstNode(VarDecl); 
        $$->append($1);
        $$->append($2); 
        $$->append($3);
        $$->append_loc($4);
        parser_context.set_var($2->str);
    }
    | const_desc ident_all opt_type_desc ASSIGN error  {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $4);
        append_syntax_error("Invalid expression.", next_loc($4));
        delete $1;
        delete $2;
        delete $3;
    }
    | const_desc ident_all opt_type_desc error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $3->loc);
        append_syntax_error("Invalid assignment.", $$->loc);
        delete $1;
        delete $2;
        delete $3;
    }
    | const_desc COLON error  {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Missing identifier.", Lc($2));
    }   
    | const_desc ASSIGN error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Missing identifier.", Lc($2));
        delete $1;
        // yyerrok;
    }
    | const_desc error{
        $$ = new AstNode(Err);
        $$->loc = $1->loc;
        append_syntax_error("Invalid variable declaration.", $1->loc);
        delete $1;
        // yyerrok;
    }
    ;

const_desc: 
    TCONST {
        $$ = new AstNode(ConstDesc, "const");
        $$->loc = $1;
    }
    | TLET {
        $$ = new AstNode(ConstDesc, "let");
        $$->loc = $1;
    };

/**************identifier list**************/
// id: type, ...  |  id: value, ...
ident_type_list: 
    {$$ = new AstNode(Stmt); }
    | ident_type_member {
        $$ = create_node_from(Stmt, $1);
    }
    | ident_type_list COMMA ident_type_member{
        $$ = $1;
        $$->append_loc($2);
        $$->append($3);
    }
    | ident_type_list COMMA {
        $$ = $1;
        $$->append_loc($2);
        #ifndef AUTO_FIX
        $$->type = Err;
        append_syntax_error("Missing identifier list member.", $$->loc);
        #else
        append_syntax_error("Missing identifier list member.", $$->loc, 1);
        #endif
    }
    // | ident_type_list COMMA

ident_type_member: 
    ident_all COLON type_desc {
        $$ = new AstNode(VarDecl);
        $$->append(new AstNode(ConstDesc, "let"));
        $$->append($1);
        $$->append($3);
        $$->append_loc($2);
    }
    | ident_all COLON ident {
        $$ = new AstNode(VarDecl);
        $$->append(new AstNode(ConstDesc, "let"));
        $$->append($1);
        $$->append_loc($2);
        auto tp_node = new AstNode(TypeDesc, "#err");
        tp_node->loc = $3->loc;
        $$->append(tp_node);
        append_syntax_error("Undeclared type.", $3->loc);
        delete $3;
    }
    | ident_all COLON error {
        $$ = new AstNode(VarDecl);
        $$->append(new AstNode(ConstDesc, "let"));
        $$->append($1);
        $$->append_loc($2);
        auto tp_node = new AstNode(TypeDesc, "#err");
        tp_node->loc = $2;
        $$->append(tp_node);
        // $$ = new AstNode(Err);
        // $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid type description.", next_loc($2));
        // delete $1;
        // yyerrok;
    }
    | ident type_desc {
        #ifdef AUTO_FIX
        $$ = new AstNode(VarDecl);
        $$->append(new AstNode(ConstDesc, "let"));
        $$->append($1);
        $$->append($2);
        $$->append_loc($2->loc);
        #else
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2->loc);
        append_syntax_error("Missing :.", $2->loc);
        delete $1;
        delete $2;
        #endif
    }
    | ident error {
        $$ = new AstNode(VarDecl);
        $$->append(new AstNode(ConstDesc, "let"));
        $$->append($1);
        $$->append_loc($1->loc);
        auto tp_node = new AstNode(TypeDesc, "#err");
        tp_node->loc = $1->loc;
        $$->append(tp_node);
        // $$ = new AstNode(Err);
        // $$->loc = $1->loc;
        append_syntax_error("Missing :.", next_loc($1->loc));
        // delete $1;
        // yyerrok;
    }
    | COLON type_desc {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1, $2->loc);
        append_syntax_error("Missing identifier.", Lc($1));
        delete $2;
    }
    | type_desc {
        $$ = new AstNode(Err);
        $$->loc = $1->loc;
        append_syntax_error("Missing identifier and :.", $1->loc);
        delete $1;
    }
    | COLON {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing identifier and type_desc.", Lc($1));
    }
    ;

ident_value_list:
    {$$ = new AstNode(StructInstanceMems); }
    | ident_value_member {
        $$ = create_node_from(StructInstanceMems, $1); 
    }
    | ident_value_list COMMA ident_value_member{
        $$ = $1;
        $$->append($3);
        $$->append_loc($2);
    }
    | ident_value_list COMMA {
        $$ = $1;
        $$->append_loc($2);
        #ifdef AUTO_FIX
        append_syntax_error("Missing initializer list member.", $$->loc, 1);
        #else
        append_syntax_error("Missing initializer list member.", $$->loc);
        $$->type = Err;
        #endif
    }
    ;

ident_value_member: 
    ident_all COLON expr_unit{
        $$ = new AstNode(StructInstanceMem);
        $$->append($1);
        $$->append($3);
        $$->append_loc($2);
    }
    | ident_all COLON error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid value initializer.", next_loc($2));
    }
    | ident_all error {
        $$ = new AstNode(Err);
        $$->loc = $1->loc;
        append_syntax_error("Invalid value initializer.", next_loc($$->loc));
    }
    | COLON expr_unit {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1, $2->loc);
        append_syntax_error("Miising identifier.", Lc($1));
    }
    | COLON {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing ideifier and value.", Lc($1));
    }
    ;

/************template declaration**************/

generic_params:
    ident_all {$$ = new AstNode(GenericParams); $$->append($1);}
    | generic_params COMMA ident_all {$$ = $1; $$->append($3);}
    ;

generic_decl:
    LT generic_params GT {$$ = $2; $$->loc = $2->loc;}
    ;

/**************struct declaration**************/
struct_decl_ident: 
    TSTRUCT ident_all{$$ = $2; $$->append_loc($1); parser_context.set_type($2->str);}
    ;

struct_decl: 
    struct_decl_ident LBRACE ident_type_list RBRACE{
        $$ = create_node_from(StructDecl, $1);
        $3->type = StructMem;
        $$->append($3);
        $$->append_loc($2);
        $$->append_loc($4);
        parser_context.set_type($1->str);
    }
    |struct_decl_ident generic_decl {
        parser_context.push_block_env();
        for(auto ch: $2->ch){
            parser_context.set_type(ch->str);
        }
    } LBRACE ident_type_list RBRACE{
        $$ = create_node_from(StructDecl, $1);
        $5->type = StructMem;
        $$->append($5);
        $$->append_loc($4);
        $$->append_loc($6);
        parser_context.pop_block_env();
        parser_context.set_type($1->str);

        auto gen = new BlockNode(GenericBlock);
        gen->append($$);
        gen->append($2);
        $$ = gen;
    }
    // | TSTRUCT error 
    | struct_decl_ident LBRACE ident_type_list error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $3->loc);
        append_syntax_error("Missing }.", next_loc($3->loc));
        delete $1;
        delete $3;
    }
    | struct_decl_ident error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($1->loc);
        append_syntax_error("Invalid struct definition block.", $$->loc);
        delete $1;
        // yyerrok;
    }
    ;

/**************function declaration**************/
func_decl_ident: 
    TFUNC ident_all{$$ = $2; $$->append_loc($1);}
    | TFUNC {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing function identifier.", next_loc($1));
        
    };
func_decl_ret_type: 
    {$$ = new AstNode(TypeDesc, "#auto");}
    | ARROW type_desc{$$ = $2; $$->append_loc($1);} 
    | ARROW error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Invalid return type description.", next_loc($1));
    };

func_decl: func_decl_ident LP ident_type_list RP func_decl_ret_type {
    parser_context.push_block_env();
    $3->type = FuncParams;
    for(auto ch: $3->ch){
        if (ch->ch.size() > 1)
            parser_context.set_var(ch->ch[1]->str);
    }
} block{
    parser_context.pop_block_env();
    $$ = create_node_from(FuncDecl, $1);
    $$->append($3);
    $$->append($5);
    $$->append($7);
    $$->append_loc($2);
    $$->append_loc($4);
}
| func_decl_ident LP ident_type_list func_decl_ret_type {
    parser_context.push_block_env();
    $3->type = FuncParams;
    for(auto ch: $3->ch){
        if (ch->ch.size() > 1)
            parser_context.set_var(ch->ch[1]->str);
    }
} block {
    parser_context.pop_block_env();
    #ifndef AUTO_FIX
    $$ = new AstNode(Err);
    $$->loc = locator_merge($1->loc, $6->loc);
    append_syntax_error("Missing ).", next_loc($3->loc));
    delete $1;
    delete $3;
    delete $4;
    delete $6;
    #else
    $$ = create_node_from(FuncDecl, $1);
    $$->append($3);
    $$->append($4);
    $$->append($6);
    $$->append_loc($2);
    $$->append_loc($4->loc);
    append_syntax_error("Missing ).", next_loc($3->loc), 1);
    #endif
}
| func_decl_ident error {
    $$ = new AstNode(Err);
    $$->loc = $1->loc;
    append_syntax_error("Invalid function declaration.", $1->loc);
    delete $1;
}
;

/**************type define**************/
type_def:
    TTYPE ident_all ASSIGN type_desc SEMI{
        $$ = new AstNode(TypeDef);
        $$->loc = locator_merge($1, $5);
        $$->append($2);
        $$->append($4);
        $$->append_loc($1);
        $$->append_loc($3);
        $$->append_loc($5);
        parser_context.set_type($2->str);
    }
    | TTYPE error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Invalid type define.", Lc($1));
    }
;

/**************block and statements**************/
block: block_ret | block_no_ret;

block_begin: LBRACE {parser_context.push_block_env();};
block_end: RBRACE {parser_context.pop_block_env(); yyerrok;};

block_ret:
    block_begin stmts expr block_end{
        $$ = $2;
        $$->type = StmtsRet;
        $$->append($3);
        $$->append_loc($1);
        $$->append_loc($4);
    }
    ;
    
block_no_ret: 
    block_begin stmts block_end{
        $$ = $2;
        $$->append_loc($1);
        $$->append_loc($3);
    }
    | block_begin stmts YYEOF {
        #ifndef AUTO_FIX
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1, $2->loc);
        append_syntax_error("Unclosed {.", Lc($1));
        delete $2;
        #else
        $$ = $2;
        $$->append_loc($1);
        $$->append_loc(next_loc($2->loc));
        append_syntax_error("Unclosed {.", Lc($1), 1);
        #endif
    }
    ;

stmts:
    {$$ = new BlockNode(Stmts); }
    | stmts stmt {$$ = $1; $$->append($2);}
    | stmts error {
        $$ = $1;
        append_syntax_error("Invalid statement.", next_loc($1->loc));
    }
    ;

stmt: single_decl | ctrl_no_ret | return_stmt | block_no_ret
    | TBREAK SEMI{$$ = new AstNode(Stmt, "break"); $$->append_loc($1); $$->append_loc($2); yyerrok;}
    | TCONTIN SEMI{$$ = new AstNode(Stmt, "continue"); $$->append_loc($1); $$->append_loc($2); yyerrok;}
    | SEMI {$$ = new AstNode(Stmt); $$->append_loc($1); yyerrok;}
    | expr SEMI{$$ = $1; $$->append_loc($2); yyerrok;}
    | TBREAK error {
        #ifndef AUTO_FIX
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing ; after break.", Lc($1));
        #else
        $$ = new AstNode(Stmt, "break");
        $$->loc = $1;
        $$->append_loc($1);
        append_syntax_error("Missing ; after break.", Lc($1), 1);
        yyerrok;
        #endif
    }
    | TCONTIN error {
        #ifndef AUTO_FIX
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Miising ; after continue.", Lc($1));
        #else
        $$ = new AstNode(Stmt, "continue");
        $$->loc = $1;
        $$->append_loc($1);
        append_syntax_error("Miising ; after continue.", Lc($1), 1);
        #endif
    }
    | expr error{
        #ifndef AUTO_FIX
        $$ = new AstNode(Err);
        $$->loc = $1->loc;
        append_syntax_error("Missing ; after expression.", $1->loc);
        #else
        $$ =$1;
        $$->append_loc($1);
        append_syntax_error("Missing ; after expression.", $1->loc, 1);
        #endif
        
    }
    | TELSE block {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1, $2->loc);
        append_syntax_error("else without if.", Lc($1));
        delete $2;
    }
    | TELSE error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("else without if", Lc($1));
    }
    | RP {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Expected expression before ')'.", Lc($1));
    }
    | RBRACKET{
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Expecetd expression before ']", Lc($1));
    }
    ;

ctrl_no_ret: 
     TIF expr block_no_ret {
        $$ = new AstNode(IfStmt);
        $$->append($2);
        $$->append($3);
        $$->append_loc($1);
     }
    |TIF expr block_no_ret TELSE block_no_ret {
        $$ = new AstNode(IfStmt);
        $$->append($2);
        $$->append($3);
        $$->append($5);
        $$->append_loc($1);
        $$->append_loc($4);
    }
    |TIF expr block_no_ret TELSE ctrl_no_ret {
        $$ = new AstNode(IfStmt);
        $$->append($2);
        $$->append($3);
        $$->append($5);
        $$->append_loc($1);
        $$->append_loc($4);
    }
    |TWHILE expr block{
        $$ = new AstNode(WhileStmt);
        $$->append($2);
        $$->append($3);
        $$->append_loc($1);
    }
    |TFOR stmt expr SEMI expr block {
        $$ = new BlockNode(ForStmt);
        $$->append($2);
        $$->append($3);
        $$->append($5);
        $$->append($6);
        $$->append_loc($1);
        $$->append_loc($4);
    }
    |TFOR stmt SEMI expr block {
        $$ = new BlockNode(ForStmt);
        $$->append($2);
        $$->append(new AstNode(Stmt));
        $$->append($4);
        $$->append($5);
        $$->append_loc($1);
        $$->append_loc($3);
    }
    |TIF expr block_ret { //todo: add auto-fix
        // #ifndef AUTO_FIX
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1, $3->loc);
        append_syntax_error("Expected if-block with no return value.", $3->loc);
        delete $2;
        delete $3;
        // #else
        // $$ = new AstNode(IfStmt);
        // $$->append($2);
        // $$->append($3);
        // $$->ap
        // #endif
    }
    |TIF block_no_ret error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1, $2->loc);
        append_syntax_error("Missing expression.", $2->loc);
        delete $2;

    }
    | TIF expr error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1, $2->loc);
        append_syntax_error("Invalid if-block.", next_loc($2->loc));
        delete $2;
    }
    | TIF expr block_no_ret TELSE block_ret { //todo: add auto-fix
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1, $5->loc);
        append_syntax_error("Expected else-block with no return value.", $5->loc);
        delete $2;
        delete $3;
        delete $5;
    }
    | TIF expr block_no_ret TELSE ctrl_ret {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1, $5->loc);
        append_syntax_error("Expected conditional statements with no return value.", $5->loc);
        delete $2;
        delete $3;
        delete $5;
    }
    | TIF expr block_no_ret TELSE error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1, $4);
        append_syntax_error("Invalid else-block.", next_loc($4));
        delete $2;
        delete $3;
    }
    // | TIF error TELSE block_no_ret {

    // }
    // | TIF error TELSE error {

    // }
    | TIF error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Invalid if statement.", next_loc($1));
    }
    | TWHILE block_no_ret{
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1, $2->loc);
        append_syntax_error("Missing expression.", $2->loc);
        delete $2;

    }
    | TWHILE expr error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1, $2->loc);
        append_syntax_error("Invalid while-block.", next_loc($2->loc));
        delete $2;
    }
    | TWHILE error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Invalid while statement.", next_loc($1));
    }
    | TFOR error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Invalid for statement.", next_loc($1));
    }
    // todo : finish for-loop error handling
    ;

ctrl_ret: 
     TIF expr block_ret TELSE block_ret {
        $$ = new AstNode(IfStmt);
        $$->append($2);
        $$->append($3);
        $$->append($5);
        $$->append_loc($1);
        $$->append_loc($4);
     };
    |TIF expr block_ret TELSE ctrl_ret {
        $$ = new AstNode(IfStmt);
        $$->append($2);
        $$->append($3);
        $$->append($5);
        $$->append_loc($1);
        $$->append_loc($4);
    }
    |TIF expr block_ret TELSE block_no_ret {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1, $5->loc);
        append_syntax_error("Expected else-block with return value.", $5->loc);
        delete $2;
        delete $3;
        delete $5;
    }
    |TIF expr block_ret TELSE ctrl_no_ret {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1, $5->loc);
        append_syntax_error("Expected conditinal statements with no return value.", $5->loc);
        delete $2;
        delete $3;
        delete $5;
    }
    ;

return_stmt: 
    TRETURN SEMI {$$ = new AstNode(Stmt, "return"); $$->append_loc($1); $$->append_loc($2); yyerrok;}
    | TRETURN expr SEMI {
        $$ = new AstNode(Stmt, "return");
        $$->append($2);
        $$->append_loc($1);
        $$->append_loc($3);
        yyerrok;
    }
    | TRETURN error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing ; after return.", Lc($1));
    }
    ;

/**************struct member function**************/
member_func_list:
    func_decl {
        $$ = new AstNode(MemFuncList);
        $$->append($1);
    }
    | member_func_list func_decl {
        $$ = $1;
        $$->append($2);
    }
    | member_func_list error {
        $$ = $1;
        append_syntax_error("Invalid function declaration", next_loc($1->loc));
    }
;
struct_impl:
    TIMPL {
        parser_context.push_block_env();
        parser_context.set_var("self");
    } type_desc LBRACE member_func_list RBRACE { 
        parser_context.pop_block_env();
        $$ = new AstNode(StructImpl);
        $$->append($3);
        $$->append($5);
        $$->append_loc($1);
        $$->append_loc($4);
        $$->append_loc($6);
    }
    | TIMPL error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Invalid implement block.", Lc($1));
    }
;

/**************expr**************/
expr:
    expr_list
    | expr_unit
    ;

item_list:
    {$$ = new AstNode(ExprList); }
    |expr_unit {$$ = create_node_from(ExprList, $1); } 
    |expr_list;

expr_list:
    expr_unit COMMA expr_unit {
        $$ = new AstNode(ExprList);
        $$->append($1);
        $$->append($3);
        $$->append_loc($2);
    }
    | expr_list COMMA expr_unit{
        $$ = $1;
        $$->append($3);
        $$->append_loc($2);
    };

expr_unit: 
    item
    | LP type_desc RP expr_unit %prec NOT{
        $$ = new OperatorNode(op_type::Convert, $4, $2, $1); 
        $$->loc = locator_merge($1, $4->loc);
        $$->append_loc($1);
        $$->append_loc($3);
    }
    | ADD expr_unit %prec NOT     {
        $$ = new OperatorNode(op_type::Pos, $2, $1); }
    | ADD error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Invalid expression.", next_loc($1));
    }
    | SUB expr_unit %prec NOT     {$$ = new OperatorNode(op_type::Neg, $2, $1); }
    | SUB error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Invalid expression.", next_loc($1));
    }
    | NOT expr_unit               {$$ = new OperatorNode(op_type::Not, $2, $1); }
    | NOT error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Invalid expression.", next_loc($1));
    }
    | expr_unit ASSIGN expr_unit  {$$ = new OperatorNode(op_type::Assign, $1, $3, $2); } 
    | ASSIGN error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit ASSIGN error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
    }
    | expr_unit ADDEQ expr_unit  {$$ = new OperatorNode(op_type::AddEq, $1, $3, $2); } 
    | ADDEQ error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit ADDEQ error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid expression.", next_loc($2));
        delete $1;
    }
    | expr_unit SUBEQ expr_unit  {$$ = new OperatorNode(op_type::SubEq, $1, $3, $2); } 
    | SUBEQ error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit SUBEQ error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid expression.", next_loc($2));
        delete $1;
    }
    | expr_unit MULEQ expr_unit  {$$ = new OperatorNode(op_type::MulEq, $1, $3, $2); } 
    | MULEQ error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit MULEQ error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid expression.", next_loc($2));
        delete $1;
    }
    | expr_unit DIVEQ expr_unit  {$$ = new OperatorNode(op_type::DivEq, $1, $3, $2); } 
    | DIVEQ error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit DIVEQ error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid expression.", next_loc($2));
        delete $1;
    }
    | expr_unit ADD expr_unit     {$$ = new OperatorNode(op_type::Add, $1, $3, $2); }
    | expr_unit ADD error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid expression.", next_loc($2));
        delete $1;
    }
    | expr_unit SUB expr_unit     {$$ = new OperatorNode(op_type::Sub, $1, $3, $2); }
    | expr_unit SUB error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid expression.", next_loc($2));
        delete $1;
    }
    | expr_unit MUL expr_unit     {$$ = new OperatorNode(op_type::Mul, $1, $3, $2); }
    | expr_unit MUL error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid expression.", next_loc($2));
        delete $1;
    }
    | expr_unit DIV expr_unit     {$$ = new OperatorNode(op_type::Div, $1, $3, $2); }
    | DIV error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit DIV error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid expression.", next_loc($2));
        delete $1;
    }
    | expr_unit MOD expr_unit     {$$ = new OperatorNode(op_type::Mod, $1, $3, $2); }
    | MOD error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit MOD error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid expression.", next_loc($2));
        delete $1;
    }
    | expr_unit BITAND expr_unit  {$$ = new OperatorNode(op_type::BitAnd, $1, $3, $2); }
    | expr_unit BITAND error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid expression.", next_loc($2));
        delete $1;
    }
    | expr_unit BITOR expr_unit   {$$ = new OperatorNode(op_type::BitOr, $1, $3, $2); }
    | BITOR error{
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit BITOR error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid expression.", next_loc($2));
        delete $1;
    }
    | expr_unit XOR expr_unit     {$$ = new OperatorNode(op_type::Xor, $1, $3, $2); }
    | XOR error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit XOR error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid expression.", next_loc($2));
        delete $1;
    }
    | expr_unit EQ expr_unit      {$$ = new OperatorNode(op_type::Eq, $1, $3, $2); }
    | EQ error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit EQ error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid expression.", next_loc($2));
        delete $1;
    }
    | expr_unit NEQ expr_unit     {$$ = new OperatorNode(op_type::Neq, $1, $3, $2); }
    | NEQ error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit NEQ error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid expression.", next_loc($2));
        delete $1;
    }
    | expr_unit LE expr_unit      {$$ = new OperatorNode(op_type::Le, $1, $3, $2); }
    | LE error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit LE error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid expression.", next_loc($2));
        delete $1;
    }
    | expr_unit GE expr_unit      {$$ = new OperatorNode(op_type::Ge, $1, $3, $2); }
    | GE error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit GE error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid expression.", next_loc($2));
        delete $1;
    }
    | expr_unit LT expr_unit      {$$ = new OperatorNode(op_type::Lt, $1, $3, $2); }
    | LT error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit LT error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid expression.", next_loc($2));
        delete $1;
    }
    | expr_unit GT expr_unit      {$$ = new OperatorNode(op_type::Gt, $1, $3, $2); }
    | GT error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit GT error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid expression.", next_loc($2));
        delete $1;
    }
    | expr_unit AND expr_unit     {$$ = new OperatorNode(op_type::And, $1, $3, $2); }
    | AND error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit AND error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid expression.", next_loc($2));
        delete $1;
    }
    | expr_unit OR expr_unit      {$$ = new OperatorNode(op_type::Or, $1, $3, $2); }
    | OR error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit OR error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid expression.", next_loc($2));
        delete $1;
    }
    | MUL expr_unit%prec NOT      {$$ = new OperatorNode(op_type::DeRef, $2, $1); }
    | MUL error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Invalid expression.", next_loc($1));
    }
    | BITAND expr_unit %prec NOT  {$$ = new OperatorNode(op_type::Ref, $2, $1); }
    | BITAND error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Invalid expression.", next_loc($1));
    }
    ;

item: ident | literal | array_instance | struct_instance | block_ret | ctrl_ret
    | LP expr RP {$$ = $2; $$->append_loc($1); $$->append_loc($3);}
    | item DOT ident {
        $$ = new OperatorNode(op_type::Access);
        $$->append($1);
        $$->append($3);
        $$->append_loc($2);
    }
    | item LBRACKET expr RBRACKET {
        $$ = new OperatorNode(op_type::At);
        $$->append($1);
        $$->append($3);
        $$->append_loc($2);
        $$->append_loc($4);
    }
    | item LP item_list RP {
        $$ = new OperatorNode(op_type::Call);
        $$->append($1);
        $3->type = FuncArgs;
        $$->append($3);
        $$->append_loc($2);
        $$->append_loc($4);
    }
    | LP expr error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1, $2->loc);
        append_syntax_error("Missing ).", next_loc($2->loc));
        delete $2;
    }
    | DOT ident {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1, $2->loc);
        append_syntax_error("Missing instance identifier.", Lc($1));
        delete $2;
    }
    | item DOT error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Invalid instance member", next_loc($2));
        delete $1;
    }
    | item LBRACKET RBRACKET {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $3);
        append_syntax_error("Missing expression.", Lc($3));
        delete $1;
    }
    | item LBRACKET expr error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $3->loc);
        append_syntax_error("Missing ]", next_loc($3->loc));
        delete $1;
        delete $3;
    }
    | item LP item_list error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $3->loc);
        append_syntax_error("Missing )", next_loc($3->loc));
        delete $1;
        delete $3;
    }
    | type_name DOT error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $2);
        append_syntax_error("Identifier cannot be typename.", $1->loc);
        delete $1;
    }
    | type_name LP item_list error {
         $$ = new AstNode(Err);
        $$->loc = locator_merge($1->loc, $3->loc);
        append_syntax_error("Identifier cannot be typename.", $1->loc);
        delete $1;
        delete $3;
    }
    ;

ident_all: ident | type_name;

ident: {
    parser_context.set_ignore();
    } NAME {
        $$ = new AstNode(Identifier, *$2); 
        delete $2;
        parser_context.get_ignore();
    }
    | {
        parser_context.set_ignore();
    } INVALID_NAME {
        $$ = new AstNode(Identifier, *$2);
        delete $2;
        parser_context.get_ignore();
    }
    ;

/**************literal**************/

literal: 
      INT   {$$ = new AstNode(IntLiteral, *$1); delete $1;}
    | TTRUE {$$ = new AstNode(BoolLiteral, *$1); delete $1;}
    | TFALSE{$$ = new AstNode(BoolLiteral, *$1); delete $1;}
    | HEX   {$$ = new AstNode(IntLiteral, *$1); delete $1;}
    | BINARY{$$ = new AstNode(IntLiteral, *$1); delete $1;}
    | FLOAT {$$ = new AstNode(DoubleLiteral, *$1); delete $1;}
    | CHAR  {$$ = new AstNode(CharLiteral, *$1); delete $1;}
    | STRING{$$ = new AstNode(StringLiteral, *$1); delete $1;}
    | TNULL {$$ = new AstNode(PointerLiteral, *$1); delete $1;}

/**************instance**************/
array_instance: 
    LBRACKET item_list RBRACKET {
        $$ = $2;
        $$->type = ArrayInstance;
        $$->append_loc($1);
        $$->append_loc($3);
    }
    | LBRACKET item_list error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1, $2->loc);
        append_syntax_error("Missing ]", next_loc($2->loc));
        delete $2;
    }
    ;

struct_instance: 
    type_name LBRACE ident_value_list RBRACE {
        $$ = new AstNode(StructInstance);
        $$->append($3);
        $$->append($1);
        $$->append_loc($2);
        $$->append_loc($4);
    }
    /* | TSTRUCT LBRACE ident_value_list error {
        $$ = new AstNode(Err);
        $$->loc = locator_merge($1, $3->loc);
        append_syntax_error("Missing }", next_loc($3->loc));
        delete $3;
    }
    | TSTRUCT error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Invalid struct instance initializer.", Lc($1));
    } */
    ;
%%

void yyerror(const char *s) {
    // fprintf(stderr, "[Syntax error]: %d %d\n", CurrentCursor.line_st, CurrentCursor.col_l);
    // append_error(CurrentCursor);
    // fprintf(stderr, "[Syntax error]: %s (row %d line %d).\n", loc.row_l, loc.line_st);
    // fprintf(stderr, "%s\n", s);
}