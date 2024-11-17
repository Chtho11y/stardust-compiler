#pragma once

#include <string>
#include <vector>

struct Locator{
    int line_st = 0, line_ed = 0, row_l = 0, row_r = 0;

    bool has_value()const {
        return row_l != row_r;
    }
};

extern Locator CurrentCursor;

struct Token{
    std::string val;
    int token_id;
    Locator loc;
};


#ifdef __cplusplus
    #define C_FIELD_BEGIN extern "C"{
    #define C_FIELD_END }
#else
    #define C_FIELD_BEGIN
    #define C_FIELD_END
#endif

C_FIELD_BEGIN

int yylex();
void set_input(FILE* file);

C_FIELD_END