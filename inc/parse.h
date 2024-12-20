#pragma once

#include <string>
#include <vector>
#include <set>
#include <algorithm>

struct LocatorBuffer{
    int line_st, line_ed, col_l, col_r;
};

struct Locator;

Locator locator_merge(LocatorBuffer l1, LocatorBuffer l2);
Locator locator_merge(LocatorBuffer l1, Locator l2);
Locator locator_merge(Locator l1, LocatorBuffer l2);
Locator locator_merge(Locator l1, Locator l2);

struct Locator{
    int line_st = 0, line_ed = 0, col_l = 0, col_r = 0;

    Locator& operator =(const LocatorBuffer& buffer){
        line_st = buffer.line_st;
        line_ed = buffer.line_ed;
        col_l = buffer.col_l;
        col_r = buffer.col_r;
        return *this;
    }

    bool operator < (const Locator& rhs) const {
        return line_st == rhs.line_st ? col_l < rhs.col_l : line_st < rhs.line_st;
    }

    bool has_value()const {
        return line_st > 0 || col_r > 0;
    }
    bool is_empty() const {
        return line_st == 0 && line_ed == 0 && col_l == 0 && col_r == 0;
    }
    void merge(const Locator& loc) {
        if (!is_empty()) {
            if (!loc.is_empty())
                *this = locator_merge(*this, loc);
        }
        else 
            *this = loc;
    }
    void merge(const LocatorBuffer& loc) {
        if (!is_empty()) {
            Locator loc1;
            loc1 = loc;
            if (!loc1.is_empty())
                *this = locator_merge(*this, loc1);
        }
        else 
            *this = loc;
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