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
//     std::cout << $$.col_l << ' ' << $$.col_r << ' ';
// } <token_id>
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

ext_decl:
    {$$ = new BlockNode(ExtDecl);}
    | ext_decl SEMI {$$ = $1;}
    | ext_decl single_decl {$$ = $1; $$->append($2); }
    | RP {}
    | RBRACE {}
    | RBRACKET {}
    | error {}
    ;

single_decl: var_decl | func_decl | struct_decl;

/**************type**************/
opt_type_desc: 
    {$$ = new AstNode(TypeDesc, "#auto");}
    |COLON type_desc {$$ = $2;};
    |type_desc {
        $$ = new AstNode(TypeDesc, "#err");
        $$->loc = $1->loc;
        append_syntax_error("Missing :.", $$->loc);
        delete $1;
        
    };
    |COLON error {
        $$ = new AstNode(TypeDesc, "#err");
        $$->loc = $1;
        append_syntax_error("Missing type description.", $$->loc);
        yyerrok;
    }
    ;

type_desc: 
      type_item
    | LP type_list RP ARROW type_desc {
        $$ = new AstNode(TypeDesc, "()");
        $$->append($2);
        $$->append($5);
    }
    | LP type_list RP ARROW error {
        $$ = new AstNode(TypeDesc, "#err");
        $$->loc = $4;
        append_syntax_error("Missing return type.", $$->loc);
        delete $2;
        yyerrok;
    }
    | LP type_list ARROW error {
        $$ = new AstNode(TypeDesc, "#err");
        $$->loc = $3;
        append_syntax_error("Missing ).", $$->loc);
        delete $2;
        yyerrok;
    }
    | LP type_list ARROW type_desc{
        $$ = new AstNode(TypeDesc, "#err");
        $$->loc = $3;
        append_syntax_error("Missing ).", $$->loc);
        delete $2;
        delete $4;
        yyerrok;
    }
    ;
    // | error {$$ = new AstNode(TypeDesc, "#err");}

type_item: 
    type_name {$$ = create_node_from(TypeDesc, $1);}
    | LP type_list RP{$$ = $2;}
    | type_item LBRACKET expr RBRACKET{
        $$ = new AstNode(TypeDesc, "[]");
        $$->append($1);
        $$->append($3);
    }
    | type_item MUL{
        $$ = new AstNode(TypeDesc, "*"); 
        $$->append($1);
    }
    | LP type_list error {
        // std::cout << yylval;
        // std :: cout << "test" << '\n';
        $$ = new AstNode(TypeDesc, "#err");
        $$->loc = $2->loc;
        append_syntax_error("Missing )", $$->loc);
        delete $2;
        yyerrok;
    }
    | type_item LBRACKET expr error {
        $$ = new AstNode(TypeDesc, "#err");
        $$->loc = $3->loc;
        append_syntax_error("Missing ]", $$->loc);
        delete $1;
        delete $3;
        yyerrok;
    }
    ;


type_list: 
    {$$ = new AstNode(TypeList);}
    |type_desc{$$ = create_node_from(TypeList, $1);}
    |type_list COMMA type_desc {$$ = $1; $$->append($3);}
    |type_list COMMA ident {
        $$ = new AstNode(TypeDesc, "#err");
        $$->loc = $3->loc;
        append_syntax_error("Undefined type.", $$->loc);
        delete $1;
        delete $3;
        yyerrok;
    }
    |type_list COMMA {
        $$ = new AstNode(TypeDesc, "#err");
        $$->loc = $2;
        append_syntax_error("Missing type description.", $$->loc);
        delete $1;
        yyerrok;
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
        $$->append($5);
        parser_context.set_var($2->str);
        $$->loc = $4;
    }
    | const_desc ident_all opt_type_desc SEMI{
        $$ = new AstNode(VarDecl); 
        $$->append($1);
        $$->append($2); 
        $$->append($3);
        parser_context.set_var($2->str);
        $$->loc = $2->loc;
    }
    | const_desc ident_all opt_type_desc ASSIGN error  {
        $$ = new AstNode(Err);
        $$->loc = $4;
        append_syntax_error("Invalid expression.", $$->loc);
        delete $1;
        delete $2;
        delete $3;
        yyerrok;
    }
    | const_desc ident_all opt_type_desc error {
        $$ = new AstNode(Err);
        $$->loc = $3->loc;
        append_syntax_error("Invalid assignment.", $$->loc);
        delete $1;
        delete $2;
        delete $3;
        yyerrok;
    }
    | const_desc COLON error  {
        $$ = new AstNode(Err);
        $$->loc = $2;
        append_syntax_error("Missing identifier.", $$->loc);
        delete $1;
        yyerrok;
    }   
    | const_desc ASSIGN error {
        $$ = new AstNode(Err);
        $$->loc = $2;
        append_syntax_error("Missing identifier.", $$->loc);
        delete $1;
        yyerrok;
    }
    | const_desc error{
        $$ = new AstNode(Err);
        $$->loc = $1->loc;
        append_syntax_error("Invalid variable declaration.", $$->loc);
        delete $1;
        yyerrok;
    }
    ;

const_desc: 
    TCONST {
        $$ = new AstNode(ConstDesc, "const");
    }
    | TLET {
        $$ = new AstNode(ConstDesc, "let");
    };

/**************identifier list**************/
// id: type, ...  |  id: value, ...
ident_type_list: 
    {$$ = new AstNode(Stmt);}
    | ident_type_member {
        $$ = create_node_from(Stmt, $1);
    }
    | ident_type_list COMMA ident_type_member{
        $$ = $1;
        $$->append($3);
    }
    | ident_type_list COMMA {

    }
    // | ident_type_list COMMA

ident_type_member: 
    ident_all COLON type_desc {
        $$ = new AstNode(VarDecl);
        $$->append(new AstNode(ConstDesc, "let"));
        $$->append($1);
        $$->append($3);
    }
    | ident_all COLON error {

    }
    | ident error {

    }
    | COLON type_desc {

    }
    | type_desc {

    }
    | COLON {

    }
    ;

ident_value_list:
    {$$ = new AstNode(StructInstance);}
    | ident_value_member {$$ = create_node_from(StructInstance, $1);}
    | ident_value_list COMMA ident_value_member{
        $$ = $1;
        $$->append($3);
    }
    | ident_value_list COMMA {

    }
    ;

ident_value_member: 
    ident_all COLON expr_unit{
        $$ = new AstNode(StructInstanceMem);
        $$->append($1);
        $$->append($3);    
    }
    | ident_all COLON error {

    }
    | ident_all error {

    }
    | COLON expr_unit {

    }
    // | expr_unit {

    // }
    | COLON {

    }
    ;

/**************struct declaration**************/
struct_decl_ident: 
    TSTRUCT ident_all{$$ = $2;}
    ;

struct_decl: 
    struct_decl_ident LBRACE ident_type_list RBRACE{
        $$ = create_node_from(StructDecl, $1);
        $3->type = StructMem;
        $$->append($3);
        parser_context.set_type($1->str);
    }
    // | TSTRUCT error 
    | struct_decl_ident LBRACE ident_type_list error {

    }
    | struct_decl_ident error {

    }
    ;

/**************function declaration**************/
func_decl_ident: 
    TFUNC ident_all{$$ = $2;}
    | TFUNC {};
func_decl_ret_type: 
    {$$ = new AstNode(TypeDesc, "#auto");}
    | ARROW type_desc{$$ = $2;}
    | ARROW error {};

func_decl: func_decl_ident LP ident_type_list RP func_decl_ret_type {
    parser_context.push_block_env();
    $3->type = FuncParams;
    for(auto ch: $3->ch){
        parser_context.set_var(ch->ch[1]->str);
    }
} block{
    parser_context.pop_block_env();
    $$ = create_node_from(FuncDecl, $1);
    $$->append($3);
    $$->append($5);
    $$->append($7);
}
| func_decl_ident LP ident_type_list func_decl_ret_type block {

}
| func_decl_ident error {

}
;

/**************block and statements**************/
block: block_ret | block_no_ret;

block_begin: LBRACE {parser_context.push_block_env();};
block_end: RBRACE {parser_context.pop_block_env();};

block_ret:
    block_begin stmts expr block_end{
        $$ = $2;
        $$->type = StmtsRet;
        $$->append($3);
    }
    ;
    
block_no_ret: 
    block_begin stmts block_end{$$ = $2;}
    // | block_begin stmts error block_end{

    // } 
    // | block_begin error {}
    | block_begin stmts YYEOF {

    }
    ;

stmts:
    {$$ = new BlockNode(Stmts);}
    | stmts stmt {$$ = $1; $$->append($2);}
    | stmts error {$$ = $1;}
    ;

stmt: single_decl | ctrl_no_ret | return_stmt | block_no_ret
    | TBREAK SEMI{$$ = new AstNode(Stmt, "break"); $$->loc = $1;}
    | TCONTIN SEMI{$$ = new AstNode(Stmt, "continue"); $$->loc = $1;}
    | SEMI {$$ = new AstNode(Stmt);}
    | expr SEMI{$$ = $1;}
    | TBREAK error {}
    | TCONTIN error {}
    | expr error{}
    | TELSE block {}
    | TELSE error {}
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
    }
    |TFOR stmt expr SEMI expr block {
        $$ = new BlockNode(ForStmt);
        $$->append($2);
        $$->append($3);
        $$->append($5);
        $$->append($6);
    }
    |TFOR stmt SEMI expr block {
        $$ = new BlockNode(ForStmt);
        $$->append($2);
        $$->append(new AstNode(Stmt));
        $$->append($4);
        $$->append($5);
    }

    |TIF block_no_ret error {

    }
    | TIF expr error {

    }
    | TIF expr block_no_ret TELSE block_ret {

    }
    | TIF expr block_no_ret TELSE ctrl_ret {

    }
    | TIF expr block_no_ret TELSE error {

    }
    // | TIF error TELSE block_no_ret {

    // }
    // | TIF error TELSE error {

    // }
    | TIF error {
        
    }
    | TWHILE block_no_ret{

    }
    | TWHILE expr error {

    }
    | TWHILE error {

    }
    | TFOR error {

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

    }
    |TIF expr block_ret TELSE ctrl_no_ret {

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

    }
    ;

/**************expr**************/
expr:
    expr_list
    | expr_unit
    ;

item_list:
    {$$ = new AstNode(ExprList);}
    |expr_unit {$$ = create_node_from(ExprList, $1);} 
    |expr_list;

expr_list:
    expr_unit COMMA expr_unit {
        $$ = new AstNode(ExprList);
        $$->append($1);
        $$->append($3);
    }
    | expr_list COMMA expr_unit{
        $$ = $1;
        $$->append($3);
    };

expr_unit: 
    item
    | LP type_desc RP expr_unit %prec NOT{$$ = new OperatorNode(op_type::Convert, $4, $2, $1);}
    | ADD expr_unit %prec NOT     {$$ = new OperatorNode(op_type::Pos, $2, $1);}
    | ADD error {}
    | SUB expr_unit %prec NOT     {$$ = new OperatorNode(op_type::Neg, $2, $1);}
    | SUB error {}
    | NOT expr_unit               {$$ = new OperatorNode(op_type::Not, $2, $1);}
    | NOT error {}
    | expr_unit ASSIGN expr_unit  {$$ = new OperatorNode(op_type::Assign, $1, $3, $2);} 
    | ASSIGN error {
        $$ = new AstNode(Err);
    }
    | expr_unit ASSIGN error {}
    | expr_unit ADDEQ expr_unit  {$$ = new OperatorNode(op_type::AddEq, $1, $3, $2);} 
    | ADDEQ error {}
    | expr_unit ADDEQ error {}
    | expr_unit SUBEQ expr_unit  {$$ = new OperatorNode(op_type::SubEq, $1, $3, $2);} 
    | SUBEQ error {}
    | expr_unit SUBEQ error {}
    | expr_unit MULEQ expr_unit  {$$ = new OperatorNode(op_type::MulEq, $1, $3, $2);} 
    | MULEQ error {}
    | expr_unit MULEQ error {}
    | expr_unit DIVEQ expr_unit  {$$ = new OperatorNode(op_type::DivEq, $1, $3, $2);} 
    | DIVEQ error {}
    | expr_unit DIVEQ error {}
    | expr_unit ADD expr_unit     {$$ = new OperatorNode(op_type::Add, $1, $3, $2);}
    | expr_unit ADD error {}
    | expr_unit SUB expr_unit     {$$ = new OperatorNode(op_type::Sub, $1, $3, $2);}
    | expr_unit SUB error {}
    | expr_unit MUL expr_unit     {$$ = new OperatorNode(op_type::Mul, $1, $3, $2);}
    | expr_unit MUL error {}
    | expr_unit DIV expr_unit     {$$ = new OperatorNode(op_type::Div, $1, $3, $2);}
    | DIV error {}
    | expr_unit DIV error {}
    | expr_unit MOD expr_unit     {$$ = new OperatorNode(op_type::Mod, $1, $3, $2);}
    | MOD error {}
    | expr_unit MOD error {}
    | expr_unit BITAND expr_unit  {$$ = new OperatorNode(op_type::BitAnd, $1, $3, $2);}
    | expr_unit BITAND error {}
    | expr_unit BITOR expr_unit   {$$ = new OperatorNode(op_type::BitOr, $1, $3, $2);}
    | BITOR error{}
    | expr_unit BITOR error {}
    | expr_unit XOR expr_unit     {$$ = new OperatorNode(op_type::Xor, $1, $3, $2);}
    | XOR error {}
    | expr_unit XOR error {}
    | expr_unit EQ expr_unit      {$$ = new OperatorNode(op_type::Eq, $1, $3, $2);}
    | EQ error {}
    | expr_unit EQ error {}
    | expr_unit NEQ expr_unit     {$$ = new OperatorNode(op_type::Neq, $1, $3, $2);}
    | NEQ error {}
    | expr_unit NEQ error {}
    | expr_unit LE expr_unit      {$$ = new OperatorNode(op_type::Le, $1, $3, $2);}
    | LE error {}
    | expr_unit LE error {}
    | expr_unit GE expr_unit      {$$ = new OperatorNode(op_type::Ge, $1, $3, $2);}
    | GE error {}
    | expr_unit GE error {}
    | expr_unit LT expr_unit      {$$ = new OperatorNode(op_type::Lt, $1, $3, $2);}
    | LT error {}
    | expr_unit LT error {}
    | expr_unit GT expr_unit      {$$ = new OperatorNode(op_type::Gt, $1, $3, $2);}
    | GT error {}
    | expr_unit GT error {}
    | expr_unit AND expr_unit     {$$ = new OperatorNode(op_type::And, $1, $3, $2);}
    | AND error {}
    | expr_unit AND error {}
    | expr_unit OR expr_unit      {$$ = new OperatorNode(op_type::Or, $1, $3, $2);}
    | OR error {}
    | expr_unit OR error {}
    | MUL expr_unit%prec NOT      {$$ = new OperatorNode(op_type::DeRef, $2, $1);}
    | MUL error {}
    | BITAND expr_unit %prec NOT  {$$ = new OperatorNode(op_type::Ref, $2, $1);}
    | BITAND error {}
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
    | LP expr error {}
    | DOT ident {}
    | item DOT error {}
    | item LBRACKET RBRACKET {}
    | item LBRACKET expr error {}
    | item LP item_list error {}
    ;

ident_all: ident | type_name;

ident: {
    parser_context.set_ignore();
    } NAME {
        $$ = new AstNode(Identifier, *$2); 
        delete $2;
        parser_context.get_ignore();
    };

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

    }
    ;

struct_instance: 
    TSTRUCT LBRACE ident_value_list RBRACE {
        $$ = $3;
        $$->loc = $1;
    }
    | TSTRUCT LBRACE ident_value_list error {}
    | TSTRUCT error {

    }
    ;
%%

void yyerror(const char *s) {
    // fprintf(stderr, "[Syntax error]: %d %d\n", CurrentCursor.line_st, CurrentCursor.col_l);
    // append_error(CurrentCursor);
    // fprintf(stderr, "[Syntax error]: %s (row %d line %d).\n", loc.row_l, loc.line_st);
    // fprintf(stderr, "%s\n", s);
}