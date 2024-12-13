#include "context.h"



ParserContext parser_context;



Locator locator_merge(LocatorBuffer l1, LocatorBuffer l2){
    Locator a, b;
    a = l1, b = l2;
    return locator_merge(a, b);
}

Locator locator_merge(LocatorBuffer l1, Locator l2){
    Locator a;
    a = l1;
    return locator_merge(a, l2);
}

Locator locator_merge(Locator l1, LocatorBuffer l2){
    Locator b;
    b = l2;
    return locator_merge(l1, b);
}

Locator locator_merge(Locator l1, Locator l2){
    Locator loc = l1;
    if(loc.line_st > l2.line_st){
        loc.line_st = l2.line_st;
        loc.col_l = l2.col_l;
    }else if(loc.line_st == l2.line_st && loc.col_l > l2.col_l){
        loc.line_st = l2.line_st;
        loc.col_l = l2.col_l;
    }
    if(loc.line_ed < l2.line_ed){
        loc.line_ed = l2.line_ed;
        loc.col_r = l2.col_r;
    }else if(loc.line_ed == l2.line_ed && loc.col_r < l2.col_r){
        loc.line_ed = l2.line_ed;
        loc.col_r = l2.col_r;
    }
    return loc;
}

std::set<std::pair<Locator, std::string>> LocatorSet;

void insert_locator(Locator loc, std::string val) {
    LocatorSet.insert(std::make_pair(loc, val));
}

Locator get_next_locator(Locator loc) {
    auto it = LocatorSet.lower_bound(std::make_pair((Locator){loc.line_ed, loc.line_ed, loc.col_r + 1, loc.col_r}, std::string()));
    if (it != LocatorSet.end()) {
        return it->first;
    }
    return (Locator){loc.line_ed, loc.line_ed, loc.col_r + 1, loc.col_r};
}