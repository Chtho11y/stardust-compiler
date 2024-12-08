%{
    // #include "lex.yy.cpp"
    #include "parse.h"
    #include "ast.h"
    #include "context.h"
    Locator CurrentCursor;
    bool EOF_FLAG = false;
    void yyerror(const char*);
    #define yyerrok1 (yyerrstatus=1)
    #define echo_error(s) \
        set_error_message(s)
%}
%union{
    Token* str;
    AstNode* node;
    int token_id;
};

%token<str> INT NAME INVALID_NAME TTRUE TFALSE
%token<str> HEX BINARY STRING FLOAT CHAR 
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
    | struct_decl_ident LBRACE struct_members error SEMI{
        $$ = new AstNode(Err);
        echo_error("Missing }");
        yyerrok;
    }
    // | struct_decl_ident LBRACE struct_members error E0F{
    //     $$ = new AstNode(Err);
    //     echo_error("Missing }");
    //     yyerrok;
    // }
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
        yyerrok;
    }
    | struct_members COMMA {
        $$ = $1;
        yyerrok;
    }
    | struct_members error {
        $$ = $1;
        if (yychar == YYEOF) {
            echo_error("Missing }");
        }
        else 
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
        yyerrok;
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
        echo_error("Missing ]!");
        yyerrok;
    }
    ;

struct_instance_member:
    ident COLON expr {
        $$ = new AstNode(StructInstanceMem);
        $$->append($1);
        $$->append($3);
    }
    | ident error {
        $$ = new AstNode(Err);
        echo_error("Invalid struct instance member!");
    }
    ;

struct_instance_members: {
        $$ = new AstNode(StructInstanceMems);
    }
    |
    struct_instance_member {
        $$ = new AstNode(StructInstanceMems);
        $$->append($1);
        yyerrok;
    }
    | struct_instance_members COMMA {yyerrok;} struct_instance_member {
        $$ = $1;
        $$->append($4);
        yyerrok;
    }
    | struct_instance_members COMMA {
        $$ = $1;
        yyerrok;
    }
    | struct_instance_members error {
        $$ = new AstNode(Err);
        if (yychar == YYEOF)
            echo_error("Missing }");
        else 
            echo_error("Invalid struct instance element!");
        // yyerrok;
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
block: block_no_ret 
    | block_ret
    ;

block_no_ret:
    LBRACE stmts RBRACE {$$ = $2;} 
    | LBRACE stmts error RBRACE{
        echo_error("Invalid ending of block!");
        $$ = $2;
        yyerrok;
    }
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
    | stmts stmt {$$ = $1; $$->append($2);}
    | stmts error SEMI{
        $$ = $1;
        set_error_message("Invalid statement!");
        yyerrok;
    } 
    // | stmts error {
    //     $$ = $1;
    //     set_error_message("Invalid statement!");
    //     // yyerrok;
    // } 
    ;

return_stmt: 
    TRETURN SEMI{ $$ = new AstNode(Stmt, "return");}
    | TRETURN expr_with_comma SEMI{
        $$ = new AstNode(Stmt, "return");
        $$->append($2);
    }
    | TRETURN error {
        $$ = new AstNode(Err);
        echo_error("Invalid return!");
        yyerrok;
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
    | TIF error {
        $$ = new AstNode(Err);
        echo_error("Invalid if!");
        yyerrok;
    }
    | TWHILE error {
        $$ = new AstNode(Err);
        echo_error("Inavlid while");
        yyerrok;
    }
    | TELSE error{
        $$ = new AstNode(Err);
        echo_error("Unmatched else!");
        yyerrok;
    }// todo: detailed control stmt error
    ;

ident: 
    NAME {$$ = new AstNode(Identifier, *$1); free($1);};
    | INVALID_NAME error{echo_error(std::string("Invalid identifier: ") + $1->val); free($1);}

literal:
      INT {$$ = new AstNode(IntLiteral, *$1); free($1);}
    | TTRUE {$$ = new AstNode(BoolLiteral, *$1); free($1);}
    | TFALSE{$$ = new AstNode(BoolLiteral, *$1); free($1);}
    | HEX {$$ = new AstNode(IntLiteral, *$1); free($1);}
    | BINARY {$$ = new AstNode(IntLiteral, *$1); free($1);}
    | FLOAT {$$ = new AstNode(IntLiteral, *$1); free($1);}
    | CHAR {$$ = new AstNode(IntLiteral, *$1); free($1);}
    | STRING {$$ = new AstNode(IntLiteral, *$1); free($1);}
    ;

opt_type_desc:
    {$$ = new AstNode(TypeDesc, "#auto");}
    |COLON type_desc {$$ = $2;};
    | COLON error {
        $$ = new AstNode(Err);
        echo_error("Invalid type!");
        yyerrok;
    }

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
    | expr_with_comma {yyerrok;} COMMA expr{
        $$ = new OperatorNode(op_type::Comma);
        $$->append($1);
        $$->append($4);
        yyerrok;
    }
    | expr_with_comma error {
        $$ = $1;
        echo_error("Invalid comma expression!");
    }
    ;

expr: sub_item
    | ADD expr %prec NOT{$$ = new OperatorNode(op_type::Pos, $2);}
    | ADD error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    | SUB expr %prec NOT{$$ = new OperatorNode(op_type::Neg, $2);}
    | SUB error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    | NOT expr{$$ = new OperatorNode(op_type::Not, $2);}
    | NOT error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    | expr ASSIGN expr  {$$ = new OperatorNode(op_type::Assign, $1, $3);} 
    | expr ASSIGN error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    | expr ADD expr     {$$ = new OperatorNode(op_type::Add, $1, $3);}
    | expr ADD error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    | expr SUB expr     {$$ = new OperatorNode(op_type::Sub, $1, $3);}
    | expr SUB error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    | expr MUL expr     {$$ = new OperatorNode(op_type::Mul, $1, $3);}
    | expr MUL error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    | expr DIV expr     {$$ = new OperatorNode(op_type::Div, $1, $3);}
    | expr DIV error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    | expr MOD expr     {$$ = new OperatorNode(op_type::Mod, $1, $3);}
    | expr MOD error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    | expr BITAND expr  {$$ = new OperatorNode(op_type::BitAnd, $1, $3);}
    | expr BITAND error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    | expr BITOR expr   {$$ = new OperatorNode(op_type::BitOr, $1, $3);}
    | expr BITOR error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    | expr XOR expr     {$$ = new OperatorNode(op_type::Xor, $1, $3);}
    | expr XOR error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    | expr EQ expr      {$$ = new OperatorNode(op_type::Eq, $1, $3);}
    | expr EQ error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    | expr NEQ expr     {$$ = new OperatorNode(op_type::Neq, $1, $3);}
    | expr NEQ error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    | expr LE expr      {$$ = new OperatorNode(op_type::Le, $1, $3);}
    | expr LE error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    | expr GE expr      {$$ = new OperatorNode(op_type::Ge, $1, $3);}
    | expr GE error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    | expr LT expr      {$$ = new OperatorNode(op_type::Lt, $1, $3);}
    | expr LT error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    | expr GT expr      {$$ = new OperatorNode(op_type::Gt, $1, $3);}
    | expr GT error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    | expr AND expr      {$$ = new OperatorNode(op_type::And, $1, $3);}
    | expr AND error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    | expr OR expr      {$$ = new OperatorNode(op_type::Or, $1, $3);}
    | expr OR error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    | MUL item {$$ = new OperatorNode(op_type::DeRef, $2);}
    | MUL error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    | BITAND item {$$ = new OperatorNode(op_type::Ref, $2);}
    | BITAND error {$$ = new AstNode(Err); echo_error("Invalid expression!");}
    ;

sub_item:
     item
    |sub_item TAS type_desc{$$ = new OperatorNode(op_type::Convert, $1, $3);}
    |sub_item TAS error {$$ = new AstNode(Err); echo_error("Invalid type conversion"); }
    ;

item: ident 
    | literal
    | array_instance
    | struct_instance
    | block_ret
    | control_stmt
    | LP expr_with_comma RP {$$ = $2;}
    | LP expr_with_comma error SEMI {
        echo_error("Missing )");
        $$ = $2;
        yyerrok;
    }
    | item LBRACKET expr_with_comma RBRACKET %prec DOT{
        $$ = new OperatorNode(op_type::At);
        $$->append($1);
        $$->append($3);
    }
    // | item LBRACKET expr_with_comma SEMI {
    //     echo_error("Missing ]");
    //     $$ = $3;
    //     yyerrok;
    // }
    | item LP func_args RP %prec DOT{
        $$ = new OperatorNode(op_type::Call);
        $$->append($1);
        $$->append($3);
    }
    | item LP func_args SEMI {
        echo_error("Missing )");
        $$ = new AstNode(Err);
        yyerrok;
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
    | func_args {yyerrok;} COMMA expr {$$ = $1; $$->append($4); yyerrok;}
    | func_args error {
        $$ = $1;
        echo_error("Invalid function argument!");
    }
    ;

%%
void yyerror(const char *s) {
    // fprintf(stderr, "[Syntax error]: %d %d\n", CurrentCursor.line_st, CurrentCursor.col_l);
    append_error(CurrentCursor);
    // fprintf(stderr, "[Syntax error]: %s (row %d line %d).\n", loc.row_l, loc.line_st);
    // fprintf(stderr, "%s\n", s);
}