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

%token<str> INT NAME TYPENAME INVALID_NAME TTRUE TFALSE
%token<str> HEX BINARY STRING FLOAT CHAR 
%token<token_id> ADD SUB MUL DIV MOD DOT LT GT LE GE EQ NEQ AND OR XOR BITAND BITOR NOT
%token<token_id> ADDEQ SUBEQ MULEQ DIVEQ
%token<token_id> TFUNC TLET TSTRUCT TIF TELSE TWHILE TCONST TRETURN TBREAK TFOR TCONTIN
%token<token_id> SEMI COLON LP RP LBRACE RBRACE ASSIGN ARROW COMMA LBRACKET RBRACKET
%token<token_id> UNCLOSED_COMMENT
%type<token_id> block_begin block_end

%type<node> program
%type<node> ext_decl single_decl
%type<node> var_decl func_decl struct_decl
%type<node> struct_decl_ident func_decl_ident func_decl_ret_type
%type<node> ident ident_all ident_type_list ident_type_member ident_value_list  ident_value_member
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

ext_decl: {
        $$ = new BlockNode(ExtDecl); 
        // $$->loc = CurrentCursor;
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

single_decl: var_decl | func_decl | struct_decl;

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
        $$->loc = $1->loc;
        $$->append(new AstNode(ConstDesc, "let"));
        $$->append($1);
        $$->append($3);
    }
    | ident_all COLON error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid type description.", $$->loc);
        delete $1;
        // yyerrok;
    }
    | ident type_desc {
        $$ = new AstNode(Err);
        $$->loc = next_loc($1->loc);
        append_syntax_error("Missing :.", $$->loc);
        delete $1;
        delete $2;
    }
    | ident error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($1->loc);
        append_syntax_error("Missing :.", $$->loc);
        delete $1;
        // yyerrok;
    }
    | COLON type_desc {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing identifier.", $$->loc);
        delete $2;
    }
    | type_desc {
        $$ = new AstNode(Err);
        $$->loc = $1->loc;
        append_syntax_error("Missing identifier and :.", $$->loc);
        delete $1;
    }
    | COLON {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing identifier and type_desc.", $$->loc);
    }
    ;

ident_value_list:
    {$$ = new AstNode(StructInstance); $$->loc = CurrentCursor;}
    | ident_value_member {
        $$ = create_node_from(StructInstance, $1); 
        $$->loc = $1->loc;
    }
    | ident_value_list COMMA ident_value_member{
        $$ = $1;
        $$->append($3);
    }
    | ident_value_list COMMA {
        $$ = new AstNode(Err);
        $$->loc = $2;
        append_syntax_error("Missing initializer list member.", $$->loc);
        delete $1;
    }
    ;

ident_value_member: 
    ident_all COLON expr_unit{
        $$ = new AstNode(StructInstanceMem);
        $$->append($1);
        $$->append($3);
        $$->loc = $1->loc;    
    }
    | ident_all COLON error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid value initializer.", $$->loc);
        delete $1;
        // yyerrok;
    }
    | ident_all error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($1->loc);
        append_syntax_error("Invalid value initializer.", $$->loc);
        delete $1;
        // yyerrok;
    }
    | COLON expr_unit {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Miising identifier.", $$->loc);
        delete $2;
    }
    // | expr_unit {

    // }
    | COLON {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing ideifier and value.", $$->loc);
    }
    ;

/**************struct declaration**************/
struct_decl_ident: 
    TSTRUCT ident_all{$$ = $2;}
    ;

struct_decl: 
    struct_decl_ident LBRACE ident_type_list RBRACE{
        $$ = create_node_from(StructDecl, $1);
        $$->loc = $1->loc;
        $3->type = StructMem;
        $$->append($3);
        parser_context.set_type($1->str);
    }
    // | TSTRUCT error 
    | struct_decl_ident LBRACE ident_type_list error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($3->loc);
        append_syntax_error("Missing }.", $$->loc);
        delete $1;
        delete $3;
        // yyerrok;
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
    TFUNC ident_all{$$ = $2;}
    | TFUNC {
        $$ = new AstNode(Err);
        $$->loc = next_loc($1);
        append_syntax_error("Missing function identifier.", $$->loc);
        
    };
func_decl_ret_type: 
    {$$ = new AstNode(TypeDesc, "#auto");}
    | ARROW type_desc{$$ = $2;}
    | ARROW error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($1);
        append_syntax_error("Invalid return type description.", $$->loc);
    };

func_decl: func_decl_ident LP ident_type_list RP func_decl_ret_type {
    parser_context.push_block_env();
    $3->type = FuncParams;
    for(auto ch: $3->ch){
        parser_context.set_var(ch->ch[1]->str);
    }
} block{
    parser_context.pop_block_env();
    $$ = create_node_from(FuncDecl, $1);
    $$->loc = $1->loc;
    $$->append($3);
    $$->append($5);
    $$->append($7);
}
| func_decl_ident LP ident_type_list func_decl_ret_type block {
    $$ = new AstNode(Err);
    $$->loc = next_loc($3->loc);
    append_syntax_error("Missing ).", $$->loc);
    delete $1;
    delete $3;
    delete $4;
    delete $5;
}
| func_decl_ident error {
    $$ = new AstNode(Err);
    $$->loc = next_loc($1->loc);
    append_syntax_error("Inavlid function declaration.", $$->loc);
    delete $1;
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
        $$->loc = locator_merge(Lc($1), Lc($4));
    }
    ;
    
block_no_ret: 
    block_begin stmts block_end{$$ = $2;}
    // | block_begin stmts error block_end{

    // } 
    // | block_begin error {}
    | block_begin stmts YYEOF {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Unclosed {.", $$->loc);
        delete $2;
    }
    ;

stmts:
    {$$ = new BlockNode(Stmts); $$->loc = CurrentCursor;}
    | stmts stmt {$$ = $1; $$->append($2);}
    | stmts error {
        $$ = $1;
        append_syntax_error("Invalid statement.", CurrentCursor);
    }
    ;

stmt: single_decl | ctrl_no_ret | return_stmt | block_no_ret
    | TBREAK SEMI{$$ = new AstNode(Stmt, "break"); $$->loc = $1; yyerrok;}
    | TCONTIN SEMI{$$ = new AstNode(Stmt, "continue"); $$->loc = $1; yyerrok;}
    | SEMI {$$ = new AstNode(Stmt); yyerrok;}
    | expr SEMI{$$ = $1; yyerrok;}
    | TBREAK error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($1);
        append_syntax_error("Missing ; after break.", $$->loc);
    }
    | TCONTIN error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($1);
        append_syntax_error("Miising ; after continue.", $$->loc);
    }
    | expr error{
        $$ = new AstNode(Err);
        $$->loc = next_loc($1->loc);
        append_syntax_error("Missing ; after expressioin.", $$->loc);
    }
    | TELSE block {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("else without if.", $$->loc);
        delete $2;
    }
    | TELSE error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("else without if", $$->loc);
    }
    | RP {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Expected expression before ')'.", $$->loc);
    }
    | RBRACKET{
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Expecetd expression before ']", $$->loc);
    }
    ;

ctrl_no_ret: 
     TIF expr block_no_ret {
        $$ = new AstNode(IfStmt);
        $$->append($2);
        $$->append($3);
        $$->loc = $1;
     }
    |TIF expr block_no_ret TELSE block_no_ret {
        $$ = new AstNode(IfStmt);
        $$->append($2);
        $$->append($3);
        $$->append($5);
        $$->loc = $1;
    }
    |TIF expr block_no_ret TELSE ctrl_no_ret {
        $$ = new AstNode(IfStmt);
        $$->append($2);
        $$->append($3);
        $$->append($5);
        $$->loc = $1;
    }
    |TWHILE expr block{
        $$ = new AstNode(WhileStmt);
        $$->append($2);
        $$->append($3);
        $$->loc = $1;
    }
    |TFOR stmt expr SEMI expr block {
        $$ = new BlockNode(ForStmt);
        $$->append($2);
        $$->append($3);
        $$->append($5);
        $$->append($6);
        $$->loc = $1;
    }
    |TFOR stmt SEMI expr block {
        $$ = new BlockNode(ForStmt);
        $$->append($2);
        $$->append(new AstNode(Stmt));
        $$->append($4);
        $$->append($5);
        $$->loc = $1;
    }
    |TIF expr block_ret {
        $$ = new AstNode(Err);
        $$->loc = $3->loc;
        append_syntax_error("Expected if-block with no return value.", $$->loc);
        delete $2;
        delete $3;
    }
    |TIF block_no_ret error {
        $$ = new AstNode(Err);
        $$->loc = $2->loc;
        append_syntax_error("Missing expression.", $$->loc);
        delete $2;

    }
    | TIF expr error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2->loc);
        append_syntax_error("Invalid if-block.", $$->loc);
        delete $2;
    }
    | TIF expr block_no_ret TELSE block_ret {
        $$ = new AstNode(Err);
        $$->loc = $5->loc;
        append_syntax_error("Expected else-block with no return value.", $$->loc);
        delete $2;
        delete $3;
        delete $5;
    }
    | TIF expr block_no_ret TELSE ctrl_ret {
        $$ = new AstNode(Err);
        $$->loc = $5->loc;
        append_syntax_error("Expected conditional statements with no return value.", $$->loc);
        delete $2;
        delete $3;
        delete $5;
    }
    | TIF expr block_no_ret TELSE error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($4);
        append_syntax_error("Invalid else-block.", $$->loc);
        delete $2;
        delete $3;
    }
    // | TIF error TELSE block_no_ret {

    // }
    // | TIF error TELSE error {

    // }
    | TIF error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($1);
        append_syntax_error("Invalid if statement.", $$->loc);
    }
    | TWHILE block_no_ret{
        $$ = new AstNode(Err);
        $$->loc = $2->loc;
        append_syntax_error("Missing expression.", $$->loc);
        delete $2;

    }
    | TWHILE expr error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2->loc);
        append_syntax_error("Invalid while-block.", $$->loc);
        delete $2;
    }
    | TWHILE error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($1);
        append_syntax_error("Invalid while statement.", $$->loc);
    }
    | TFOR error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($1);
        append_syntax_error("Invalid for statement.", $$->loc);
    }
    // todo : finish for-loop error handling
    ;

ctrl_ret: 
     TIF expr block_ret TELSE block_ret {
        $$ = new AstNode(IfStmt);
        $$->append($2);
        $$->append($3);
        $$->append($5);
        $$->loc = $1;
     };
    |TIF expr block_ret TELSE ctrl_ret {
        $$ = new AstNode(IfStmt);
        $$->append($2);
        $$->append($3);
        $$->append($5);
        $$->loc = $1;
    }
    |TIF expr block_ret TELSE block_no_ret {
        $$ = new AstNode(Err);
        $$->loc = $5->loc;
        append_syntax_error("Expected else-block with return value.", $$->loc);
        delete $2;
        delete $3;
        delete $5;
    }
    |TIF expr block_ret TELSE ctrl_no_ret {
        $$ = new AstNode(Err);
        $$->loc = $5->loc;
        append_syntax_error("Expected conditinal statements with no return value.", $$->loc);
        delete $2;
        delete $3;
        delete $5;
    }
    ;

return_stmt: 
    TRETURN SEMI {$$ = new AstNode(Stmt, "return"); $$->loc = $1;}
    | TRETURN expr SEMI {
        $$ = new AstNode(Stmt, "return");
        $$->append($2);
        $$->loc = $1;
    }
    | TRETURN error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($1);
        append_syntax_error("Missing ; after return.", $$->loc = $1);
    }
    ;

/**************expr**************/
expr:
    expr_list
    | expr_unit
    ;

item_list:
    {$$ = new AstNode(ExprList); $$->loc = CurrentCursor;}
    |expr_unit {$$ = create_node_from(ExprList, $1); $$->loc = CurrentCursor;} 
    |expr_list;

expr_list:
    expr_unit COMMA expr_unit {
        $$ = new AstNode(ExprList);
        $$->append($1);
        $$->append($3);
        $$->loc = $1->loc;
    }
    | expr_list COMMA expr_unit{
        $$ = $1;
        $$->append($3);
    };

expr_unit: 
    item
    | LP type_desc RP expr_unit %prec NOT{$$ = new OperatorNode(op_type::Convert, $4, $2, $1); $$->loc = $1;}
    | ADD expr_unit %prec NOT     {$$ = new OperatorNode(op_type::Pos, $2, $1); $$->loc = $1;}
    | ADD error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($1);
        append_syntax_error("Invalid expression.", $$->loc);
    }
    | SUB expr_unit %prec NOT     {$$ = new OperatorNode(op_type::Neg, $2, $1); $$->loc = $1;}
    | SUB error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($1);
        append_syntax_error("Invalid expression.", $$->loc);
    }
    | NOT expr_unit               {$$ = new OperatorNode(op_type::Not, $2, $1); $$->loc = $1;}
    | NOT error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($1);
        append_syntax_error("Invalid expression.", $$->loc);
    }
    | expr_unit ASSIGN expr_unit  {$$ = new OperatorNode(op_type::Assign, $1, $3, $2); $$->loc = $1->loc;} 
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
    | expr_unit ADDEQ expr_unit  {$$ = new OperatorNode(op_type::AddEq, $1, $3, $2); $$->loc = $1->loc;} 
    | ADDEQ error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit ADDEQ error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
    }
    | expr_unit SUBEQ expr_unit  {$$ = new OperatorNode(op_type::SubEq, $1, $3, $2); $$->loc = $1->loc;} 
    | SUBEQ error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit SUBEQ error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
    }
    | expr_unit MULEQ expr_unit  {$$ = new OperatorNode(op_type::MulEq, $1, $3, $2); $$->loc = $1->loc;} 
    | MULEQ error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit MULEQ error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
    }
    | expr_unit DIVEQ expr_unit  {$$ = new OperatorNode(op_type::DivEq, $1, $3, $2); $$->loc = $1->loc;} 
    | DIVEQ error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit DIVEQ error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
    }
    | expr_unit ADD expr_unit     {$$ = new OperatorNode(op_type::Add, $1, $3, $2); $$->loc = $1->loc;}
    | expr_unit ADD error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
    }
    | expr_unit SUB expr_unit     {$$ = new OperatorNode(op_type::Sub, $1, $3, $2); $$->loc = $1->loc;}
    | expr_unit SUB error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
    }
    | expr_unit MUL expr_unit     {$$ = new OperatorNode(op_type::Mul, $1, $3, $2); $$->loc = $1->loc;}
    | expr_unit MUL error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
    }
    | expr_unit DIV expr_unit     {$$ = new OperatorNode(op_type::Div, $1, $3, $2); $$->loc = $1->loc;}
    | DIV error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit DIV error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
    }
    | expr_unit MOD expr_unit     {$$ = new OperatorNode(op_type::Mod, $1, $3, $2); $$->loc = $1->loc;}
    | MOD error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit MOD error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
    }
    | expr_unit BITAND expr_unit  {$$ = new OperatorNode(op_type::BitAnd, $1, $3, $2); $$->loc = $1->loc;}
    | expr_unit BITAND error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
    }
    | expr_unit BITOR expr_unit   {$$ = new OperatorNode(op_type::BitOr, $1, $3, $2); $$->loc = $1->loc;}
    | BITOR error{
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit BITOR error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
    }
    | expr_unit XOR expr_unit     {$$ = new OperatorNode(op_type::Xor, $1, $3, $2); $$->loc = $1->loc;}
    | XOR error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit XOR error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
    }
    | expr_unit EQ expr_unit      {$$ = new OperatorNode(op_type::Eq, $1, $3, $2); $$->loc = $1->loc;}
    | EQ error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit EQ error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
    }
    | expr_unit NEQ expr_unit     {$$ = new OperatorNode(op_type::Neq, $1, $3, $2); $$->loc = $1->loc;}
    | NEQ error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit NEQ error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
    }
    | expr_unit LE expr_unit      {$$ = new OperatorNode(op_type::Le, $1, $3, $2); $$->loc = $1->loc;}
    | LE error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit LE error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
    }
    | expr_unit GE expr_unit      {$$ = new OperatorNode(op_type::Ge, $1, $3, $2); $$->loc = $1->loc;}
    | GE error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit GE error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
    }
    | expr_unit LT expr_unit      {$$ = new OperatorNode(op_type::Lt, $1, $3, $2); $$->loc = $1->loc;}
    | LT error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit LT error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
    }
    | expr_unit GT expr_unit      {$$ = new OperatorNode(op_type::Gt, $1, $3, $2); $$->loc = $1->loc;}
    | GT error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit GT error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
    }
    | expr_unit AND expr_unit     {$$ = new OperatorNode(op_type::And, $1, $3, $2); $$->loc = $1->loc;}
    | AND error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit AND error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
    }
    | expr_unit OR expr_unit      {$$ = new OperatorNode(op_type::Or, $1, $3, $2); $$->loc = $1->loc;}
    | OR error {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing expression.", $$->loc);
    }
    | expr_unit OR error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
    }
    | MUL expr_unit%prec NOT      {$$ = new OperatorNode(op_type::DeRef, $2, $1); $$->loc = $1;}
    | MUL error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($1);
        append_syntax_error("Invalid expression.", $$->loc);
    }
    | BITAND expr_unit %prec NOT  {$$ = new OperatorNode(op_type::Ref, $2, $1); $$->loc = $1; }
    | BITAND error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($1);
        append_syntax_error("Invalid expression.", $$->loc);
    }
    ;

item: ident | literal | array_instance | struct_instance | block_ret | ctrl_ret
    | LP expr RP {$$ = $2;}
    | item DOT ident {
        $$ = new OperatorNode(op_type::Access);
        $$->append($1);
        $$->append($3);
        $$->loc = $2;
    }
    | item LBRACKET expr RBRACKET {
        $$ = new OperatorNode(op_type::At);
        $$->append($1);
        $$->append($3);
        $$->loc = $2;
    }
    | item LP item_list RP {
        $$ = new OperatorNode(op_type::Call);
        $$->append($1);
        $3->type = FuncArgs;
        $$->append($3);
        $$->loc = $2;
    }
    | LP expr error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2->loc);
        append_syntax_error("Missing ).", $$->loc);
        delete $2;
    }
    | DOT ident {
        $$ = new AstNode(Err);
        $$->loc = $1;
        append_syntax_error("Missing instance identifier.", $$->loc);
        delete $2;
    }
    | item DOT error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2);
        append_syntax_error("Invalid instance member", $$->loc);
        delete $1;
    }
    | item LBRACKET RBRACKET {
        $$ = new AstNode(Err);
        $$->loc = $3;
        append_syntax_error("Missing expression.", $$->loc);
        delete $1;
    }
    | item LBRACKET expr error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($3->loc);
        append_syntax_error("Missing ]", $$->loc);
        delete $1;
        delete $3;
    }
    | item LP item_list error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($3->loc);
        append_syntax_error("Missing )", $$->loc);
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

/**************instance**************/
array_instance: 
    LBRACKET item_list RBRACKET {
        $$ = $2;
        $$->type = ArrayInstance;
    }
    | LBRACKET item_list error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($2->loc);
        append_syntax_error("Missing ]", $$->loc);
        delete $2;
    }
    ;

struct_instance: 
    TSTRUCT LBRACE ident_value_list RBRACE {
        $$ = $3;
        $$->loc = $1;
    }
    | TSTRUCT LBRACE ident_value_list error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($3->loc);
        append_syntax_error("Missing }", $$->loc);
        delete $3;
    }
    | TSTRUCT error {
        $$ = new AstNode(Err);
        $$->loc = next_loc($1);
        append_syntax_error("Invalid struct instance initializer.", $$->loc);
    }
    ;
%%

void yyerror(const char *s) {
    // fprintf(stderr, "[Syntax error]: %d %d\n", CurrentCursor.line_st, CurrentCursor.col_l);
    // append_error(CurrentCursor);
    // fprintf(stderr, "[Syntax error]: %s (row %d line %d).\n", loc.row_l, loc.line_st);
    // fprintf(stderr, "%s\n", s);
}