%{
    // #include "lex.yy.cpp"
    #include "parse.h"
    #include "ast.h"
    #include "context.h"
    Locator CurrentCursor;
    void yyerror(const char*);
    #define yyerrok1 (yyerrstatus=1)
    #define echo_error(s) \
        set_error_message(s);
%}
%union{
    Token* str;
    AstNode* node;
    int token_id;
};

%token<str> INT NAME INVALID_NAME TTRUE TFALSE
%token<token_id> ADD SUB MUL DIV MOD DOT LT GT LE GE EQ NEQ AND OR XOR BITAND BITOR NOT
%token<token_id> TFUNC TLET TSTRUCT TIF TELSE TWHILE TCONST TRETURN TBREAK
%token<token_id> SEMI COLON LP RP LBRACE RBRACE ASSIGN ARROW COMMA LBRACKET RBRACKET
%token<token_id> UNCLOSED_COMMENT

%type<node> program
%type<node> ext_decl single_decl
%type<node> var_decl func_decl struct_decl
%type<node> struct_decl_ident func_decl_ident func_decl_ret_type
%type<node> struct_members struct_member
%type<node> array_instance array_objects struct_instance struct_instance_members struct_instance_member
%type<node> const_desc opt_type_desc type_desc type_desc_no_func type_item type_list
%type<node> type_list_bk
%type<node> block block_no_ret block_ret
%type<node> stmts stmt control_stmt return_stmt
%type<node> expr expr_with_comma func_params func_param func_args func_params_pr
%type<node> item sub_item
%type<node> ident literal

%right ASSIGN
%left OR
%left AND
%left BITOR
%left XOR
%left BITAND
%left LT GT LE GE EQ NEQ
%left ADD SUB
%left MUL DIV MOD
%left NOT
%left TAS
%left DOT
%%

program:
    ext_decl {program_root = new BlockNode(Program); program_root->append($1);}
    ;

ext_decl:
    {$$ = new BlockNode(ExtDecl);}
    | ext_decl SEMI {$$ = $1; yyerrok;}
    | ext_decl {yyerrok;} single_decl {$$ = $1; $$->append($3); yyerrok;} 
    | ext_decl error {$$ = $1;  echo_error("Invalid ext declartion!"); }
    ;

single_decl: var_decl | func_decl | struct_decl
    ;

var_decl:
    const_desc ident opt_type_desc ASSIGN expr SEMI{
        $$ = new AstNode(VarDecl); 
        $$->append($1);
        $$->append($2); 
        $$->append($3);
        $$->append($5);
    }
    | const_desc ident opt_type_desc SEMI{
        $$ = new AstNode(VarDecl); 
        $$->append($1);
        $$->append($2); 
        $$->append($3);
    }
    | const_desc error SEMI{
        $$ = new AstNode(Err);
        echo_error("Invalid variable declaration!");
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

struct_decl_ident:
    TSTRUCT ident {
        $$ = new AstNode(StructDecl);
        $$->append($2);
    }
    | TSTRUCT error {
        $$ = new AstNode(StructDecl);
        $$->append(new AstNode(Err));
        echo_error("Missing struct identifier!");
        yyerrok;
    }


struct_decl:
    struct_decl_ident LBRACE struct_members RBRACE {
        $$ = new AstNode(StructDecl);
        $$->append($1);
        $$->append($3);
        yyerrok;
    }
    | struct_decl_ident LBRACE struct_members error SEMI {
        $$ = new AstNode(Err);
        echo_error("Missing }");
        yyerrok;
    }
    | struct_decl_ident error {
        $$ = new AstNode(Err);
        echo_error("Invalid struct declaration!");
        yyerrok;
    }

    // | TSTRUCT ident LBRACE struct_members COMMA RBRACE{
    //     $$ = new AstNode(StructDecl);
    //     $$->append($2);
    //     $$->append($4);
    // }
    ;

struct_members: {
        $$ = new AstNode(StructMem);
    }
    | 
    struct_member {
        $$ = create_node_from(StructMem, $1);
        yyerrok;
    }
    | struct_members COMMA {yyerrok;} struct_member{
        $$ = $1;
        $$->append($4);
    }
    | struct_members COMMA {
        $$ = $1;
        yyerrok;
    }
    | struct_members error {
        $$ = $1;
        echo_error("Invalid struct member!");
        // yyerrok;
    }
    ;

struct_member:
    ident COLON type_desc{
        $$ = new AstNode(VarDecl);
        $$->append($1);
        $$->append($3);
    }
    | ident error {
        $$ = new AstNode(Err);
        echo_error("Invalid struct member!");
    }
    ;

func_decl_ident:
    TFUNC ident {
        $$ = new AstNode(FuncDecl);
        $$->append($2);
    }
    | TFUNC error {
        $$ = new AstNode(FuncDecl);
        $$->append(new AstNode(Err));
        echo_error("Missing function identifier!");
        yyerrok;
    }
    ;
func_decl_ret_type: {
        $$ = new AstNode(TypeDesc, "#auto");
    }
    | ARROW type_desc {
        $$ = $2;
    }
    | ARROW error {
        $$ = new AstNode(Err);
        echo_error("Invalid function return type");
        yyerrok;
    }
    ;
func_decl:
    // func_decl_ident LP func_params RP block{
    //     $$ = $1;
    //     $$->append($3);
    //     $$->append(new AstNode(TypeDesc, "#auto"));
    //     $$->append($5);
    // }

    // | func_decl_ident LP func_params RP ARROW type_desc block{
    //     $$ = $1;
    //     $$->append($3);
    //     $$->append($6);
    //     $$->append($7);
    // }
    func_decl_ident func_params_pr func_decl_ret_type block {
        $$ = $1;
        $$->append($2);
        $$->append($3);
        $$->append($4);
        yyerrok;
    }
    | func_decl_ident func_params_pr func_decl_ret_type error {
        $$ = new AstNode(Err);
        echo_error("Invalid function body");
        yyerrok;
    }
    | func_decl_ident LP func_params error block {
        $$ = new AstNode(Err);
        echo_error("Missing )");
    }
    | func_decl_ident LP func_params error SEMI {
        $$ = new AstNode(Err);
        echo_error("Missing )");
    }
    | func_decl_ident error {
        $$ = new AstNode(Err);
        echo_error("Invalid function declaration!");
        yyerrok;
    } //todo: more detailed function error message!
    ;

func_params_pr:
    LP func_params RP {$$ = $2; yyerrok;}
    // | LP func_params error {
    //     $$ = new AstNode(Err);
    //     echo_error("Missing )");
    // }
    ;
func_params:
    {$$ = new AstNode(FuncParams);}
    |func_params COMMA {yyerrok;} func_param{
        $$ = $1;
        $$->append($4);
        yyerrok;
    }
    |func_param{
        $$ = create_node_from(FuncParams, $1);
        yyerrok;
    }
    | func_params error {
        $$ = new AstNode(Err);
        echo_error("Invalid function parameter!");
        // yyerrok;
    }
    ;

func_param:
    ident COLON type_desc{
        $$ = new AstNode(VarDecl);
        $$->append($1);
        $$->append($3);
    }
    | ident error {
        $$ = new AstNode(Err);
        echo_error("Invalid function parameter!");
    }
    ;

array_objects:
    {$$ = new AstNode(ArrayInstance);}
    | expr {
        $$ = new AstNode(ArrayInstance);
        $$->append($1);
        yyerrok;
    }
    | array_objects COMMA {yyerrok;} expr {
        $$ = $1;
        $$->append($4);
    }
    | array_objects error {
        $$ = $1;
        echo_error("Invalid array element!");
        // yyerrok;
    }
    ;

array_instance: 
    LBRACKET array_objects RBRACKET {
        $$ = $2;
        yyerrok;
    }
    | LBRACKET array_objects error SEMI {
        echo_error("Missing ]");
        yyerrok;
    }
    ;

struct_instance_member:
    ident COLON expr {
        $$ = new AstNode(StructInstanceMem);
        $$->append($1);
        $$->append($3);
    }
    ;

struct_instance_members: {
        $$ = new AstNode(StructInstanceMems);
    }
    |
    struct_instance_member {
        $$ = new AstNode(StructInstanceMems);
        $$->append($1);
    }
    | struct_instance_members COMMA struct_instance_member {
        $$ = $1;
        $$->append($3);
    }
    | struct_instance_members COMMA {
        $$ = $1;
    }
    | error {
        $$ = new AstNode(Err);
        set_error_message("Invalid struct instance element!");
        yyerrok;
    }
    ;
struct_instance: 
    TSTRUCT LBRACE struct_instance_members RBRACE {
        $$ = new AstNode(StructInstance);
        $$->append($3);
    }
    | TSTRUCT error {
        $$ = new AstNode(Err);
        set_error_message("Invalid struct instance!");
        yyerrok;
    }
    ;
block: block_no_ret | block_ret;

block_no_ret:
    LBRACE stmts RBRACE {$$ = $2;}
    ;

block_ret:
    LBRACE stmts expr_with_comma RBRACE {
        $$ = $2;
        $$->type = StmtsRet;
        $$->append($3);
    }
    ;

stmts:
     {$$ = new BlockNode(Stmts);}
    | stmts stmt{$$ = $1; $$->append($2);}
    | error {
        $$ = new AstNode(Err);
        set_error_message("Invalid statement!");
    } 
    ;

return_stmt: 
    TRETURN SEMI{$$ = new AstNode(Stmt, "return");}
    | TRETURN expr_with_comma SEMI{
        $$ = new AstNode(Stmt, "return");
        $$->append($2);
    }

stmt: var_decl
    | block_no_ret
    | TBREAK SEMI{$$ = new AstNode(Stmt, "break");}
    | return_stmt {$$ = $1;}
    | SEMI {$$ = new AstNode(Stmt);}
    | expr_with_comma SEMI {$$ = $1;}
    ;

control_stmt:
    TIF expr_with_comma block{
        $$ = new AstNode(IfStmt);
        $$->append($2);
        $$->append($3);
    }
    | TIF expr_with_comma block TELSE block{
        $$ = new AstNode(IfStmt);
        $$->append($2);
        $$->append($3);
        $$->append($5);        
    }
    | TIF expr_with_comma block TELSE control_stmt {
        $$ = new AstNode(IfStmt);
        $$->append($2);
        $$->append($3);
        $$->append($5);
    }
    | TWHILE expr_with_comma block{
        $$ = new AstNode(WhileStmt);
        $$->append($2);
        $$->append($3);       
    }
    ;

ident: 
    NAME {$$ = new AstNode(Identifier, *$1); free($1);};
    | INVALID_NAME error{set_error_message(std::string("Invalid identifier: ") + $1->val); free($1);}

literal:
      INT {$$ = new AstNode(IntLiteral, *$1); free($1);}
    | TTRUE {$$ = new AstNode(BoolLiteral, *$1); free($1);}
    | TFALSE{$$ = new AstNode(BoolLiteral, *$1); free($1);}
    ;

opt_type_desc:
    {$$ = new AstNode(TypeDesc, "#auto");}
    |COLON type_desc {$$ = $2;};

type_desc: type_desc_no_func 
    | type_list_bk ARROW type_desc{
        $$ = new AstNode(TypeDesc, "()");
        $$->append($1);
        $$->append($3);
    }
    ;
    
type_desc_no_func: type_item 
    |type_desc_no_func MUL{
        $$ = create_node_from(TypeDesc, $1); 
        $$->str = "*";
    } 
    |type_desc_no_func LBRACKET expr RBRACKET{
        $$ = new AstNode(TypeDesc, "[]");
        $$->append($1);
        $$->append($3);
    } 
    ;

type_list_bk:LP type_list RP {$$ = $2;};

type_list:
    {$$ = new AstNode(TypeList);}
    |type_desc{$$ = create_node_from(TypeList, $1);}
    |type_list COMMA type_desc {$$ = $1; $$->append($3);}
    ;

type_item:
    ident {$$ = create_node_from(TypeDesc, $1);}
    | type_list_bk 
    ;

expr_with_comma:
    expr
    | expr COMMA expr_with_comma{
        $$ = new OperatorNode(op_type::Comma);
        $$->append($1);
        $$->append($3);
   }
    ;

expr: sub_item
    | ADD expr %prec NOT{$$ = new OperatorNode(op_type::Pos, $2);}
    | SUB expr %prec NOT{$$ = new OperatorNode(op_type::Neg, $2);}
    | NOT expr{$$ = new OperatorNode(op_type::Not, $2);}
    | expr ASSIGN expr  {$$ = new OperatorNode(op_type::Assign, $1, $3);} 
    | expr ADD expr     {$$ = new OperatorNode(op_type::Add, $1, $3);}
    | expr SUB expr     {$$ = new OperatorNode(op_type::Sub, $1, $3);}
    | expr MUL expr     {$$ = new OperatorNode(op_type::Mul, $1, $3);}
    | expr DIV expr     {$$ = new OperatorNode(op_type::Div, $1, $3);}
    | expr MOD expr     {$$ = new OperatorNode(op_type::Mod, $1, $3);}
    | expr BITAND expr  {$$ = new OperatorNode(op_type::BitAnd, $1, $3);}
    | expr BITOR expr   {$$ = new OperatorNode(op_type::BitOr, $1, $3);}
    | expr XOR expr     {$$ = new OperatorNode(op_type::Xor, $1, $3);}
    | expr EQ expr      {$$ = new OperatorNode(op_type::Eq, $1, $3);}
    | expr NEQ expr     {$$ = new OperatorNode(op_type::Neq, $1, $3);}
    | expr LE expr      {$$ = new OperatorNode(op_type::Le, $1, $3);}
    | expr GE expr      {$$ = new OperatorNode(op_type::Ge, $1, $3);}
    | expr LT expr      {$$ = new OperatorNode(op_type::Lt, $1, $3);}
    | expr GT expr      {$$ = new OperatorNode(op_type::Gt, $1, $3);}
    | expr AND expr      {$$ = new OperatorNode(op_type::And, $1, $3);}
    | expr OR expr      {$$ = new OperatorNode(op_type::Or, $1, $3);}
    | MUL item {$$ = new OperatorNode(op_type::DeRef, $2);}
    | BITAND item {$$ = new OperatorNode(op_type::Ref, $2);}
    ;

sub_item:
     item
    |sub_item TAS type_desc{$$ = new OperatorNode(op_type::Convert, $1, $3);}
    ;

item: ident 
    | literal
    | array_instance
    | struct_instance
    | block_ret
    | control_stmt
    | LP expr_with_comma RP {$$ = $2;}
    | item LBRACKET expr_with_comma RBRACKET %prec DOT{
        $$ = new OperatorNode(op_type::At);
        $$->append($1);
        $$->append($3);
    }
    | item LP func_args RP %prec DOT{
        $$ = new OperatorNode(op_type::Call);
        $$->append($1);
        $$->append($3);
    }
    | item DOT ident{
        $$ = new OperatorNode(op_type::Access);
        $$->append($1);
        $$->append($3);
    }
    ;

func_args:
    {$$ = new AstNode(FuncArgs);}
    | expr {$$ = create_node_from(FuncArgs, $1);}
    | func_args COMMA expr {$$ = $1; $$->append($3);}
    ;

%%
void yyerror(const char *s) {
    // fprintf(stderr, "[Syntax error]: %d %d\n", CurrentCursor.line_st, CurrentCursor.col_l);
    append_error(CurrentCursor);
    // fprintf(stderr, "[Syntax error]: %s (row %d line %d).\n", loc.row_l, loc.line_st);
    // fprintf(stderr, "%s\n", s);
}
