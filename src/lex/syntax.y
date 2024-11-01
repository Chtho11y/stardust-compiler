%{
    // #include "lex.yy.cpp"
    #include "parse.h"
    #include "ast.h"
    void yyerror(const char*);
%}
%union{
    Token* str;
    AstNode* node;
    int token_id;
};

%token<str> INT NAME
%token<token_id> ADD SUB MUL DIV MOD DOT LT GT LE GE EQ NEQ AND OR XOR BITAND BITOR NOT
%token<token_id> TFUNC TLET TSTRUCT TIF TELSE
%token<token_id> SEMI COLON LP RP LBRACE RBRACE ASSIGN ARROW COMMA LBRACKET RBRACKET

%type<node> program
%type<node> ext_decl single_decl
%type<node> var_decl func_decl struct_decl
%type<node> struct_members struct_member
%type<node> const_desc opt_type_desc type_desc type_desc_no_func type_item type_list
%type<node> type_list_bk
%type<node> block block_no_ret block_ret
%type<node> stmts stmt control_stmt
%type<node> expr expr_with_comma func_params func_param func_args
%type<node> item
%type<node> ident literal

%right ASSIGN
%left ADD SUB
%left MUL DIV MOD
%%

program:
    ext_decl {program_root = create_node_from(Program, $1);}
    ;

ext_decl:
    {$$ = new AstNode(ExtDecl);}
    | ext_decl SEMI {$$ = $1;}
    |  ext_decl single_decl {$$ = $1; $$->append($2);} 
    ;

single_decl: var_decl | func_decl | struct_decl;

var_decl:
    const_desc ident opt_type_desc ASSIGN expr SEMI{
        $$ = new AstNode(VarDecl); 
        $$->append($2); 
        $$->append($3);
        $$->append($5);
     }
    | const_desc ident opt_type_desc SEMI{
        $$ = new AstNode(VarDecl); 
        $$->append($2); 
        $$->append($3);
     }
    ;

const_desc: TLET {$$ = nullptr;};

struct_decl:
    TSTRUCT ident LBRACE struct_members RBRACE{
        $$ = new AstNode(StructDecl);
        $$->append($2);
        $$->append($4);
    }

    | TSTRUCT ident LBRACE struct_members COMMA RBRACE{
        $$ = new AstNode(StructDecl);
        $$->append($2);
        $$->append($4);
    }
    ;

struct_members:
    struct_member{
        $$ = create_node_from(StructMem, $1);
    }
    | struct_members COMMA struct_member{
        $$ = $1;
        $$->append($3);
    }
    ;

struct_member:
    ident COLON type_desc{
        $$ = new AstNode(VarDecl);
        $$->append(new AstNode(ConstDesc, "1"));
        $$->append($1);
        $$->append($3);
    }
    ;

func_decl:
    TFUNC ident LP func_params RP block{
        $$ = new AstNode(FuncDecl);
        $$->append($2);
        $$->append($4);
        $$->append(new AstNode(TypeDesc, "#auto"));
        $$->append($6);
    }

    | TFUNC ident LP func_params RP ARROW type_desc block{
        $$ = new AstNode(FuncDecl);
        $$->append($2);
        $$->append($4);
        $$->append($7);
        $$->append($8);
    }
    ;

func_params:
    {$$ = new AstNode(FuncParams);}
    |func_params COMMA func_param{
        $$ = $1;
        $$->append($3);
    }
    |func_param{
        $$ = create_node_from(FuncParams, $1);
    }
    ;

func_param:
    ident COLON type_desc{
        $$ = new AstNode(VarDecl);
        $$->append(new AstNode(ConstDesc, "1"));
        $$->append($1);
        $$->append($3);
    }
    ;

block: block_no_ret | block_ret;

block_no_ret:
    LBRACE stmts RBRACE {$$ = $2;}
    ;

block_ret:
    LBRACE stmts expr_with_comma RBRACE {$$ = create_node_from(StmtsRet, $2); $$->append($3);}
    ;

stmts:
     {$$ = new AstNode(Stmts);}
    |stmts stmt{$$ = $1; $$->append($2);} 
    ;

stmt: var_decl
    | block_no_ret
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
    ;

ident: NAME {$$ = new AstNode(Identifier, *$1); free($1);};

literal:
      INT {$$ = new AstNode(IntLiteral, *$1); free($1);}
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
    |type_desc_no_func MUL {
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

expr: item
    | expr ASSIGN expr{
        $$ = new OperatorNode(op_type::Assign);
        $$->append($1);
        $$->append($3);
    } 
    | expr ADD expr{
        $$ = new OperatorNode(op_type::Add);
        $$->append($1);
        $$->append($3);
    }
    | expr SUB expr{
        $$ = new OperatorNode(op_type::Sub);
        $$->append($1);
        $$->append($3);
    }
    | expr MUL expr{
        $$ = new OperatorNode(op_type::Mul);
        $$->append($1);
        $$->append($3);
    }
    | expr DIV expr{
        $$ = new OperatorNode(op_type::Div);
        $$->append($1);
        $$->append($3);
    }
    | expr MOD expr{
        $$ = new OperatorNode(op_type::Mod);
        $$->append($1);
        $$->append($3);
    }
    ;

item: ident 
    | literal
    | block_ret
    | LP expr_with_comma RP {$$ = $2;}
    | item LBRACKET item RBRACKET {
        $$ = new OperatorNode(op_type::At);
        $$->append($1);
        $$->append($3);
    }
    | item LP func_args RP{
        $$ = new OperatorNode(op_type::Call);
        $$->append($1);
        $$->append($3);
    }
    | control_stmt
    ;

func_args:
    {$$ = new AstNode(FuncArgs);}
    | expr {$$ = create_node_from(FuncArgs, $1);}
    | func_args COMMA expr {$$ = $1; $$->append($3);}
    ;

%%
void yyerror(const char *s) {
    fprintf(stderr, "%s\n", s);
}
