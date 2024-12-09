#include "error.h"
#include "context.h"
#include <sstream>

std::string str(){
    return "";
}

template<class Tp, class ...Args>
std::string str(Tp val, Args... args){
    std::stringstream ss;
    ss << val;
    return ss.str() + str(args...);
}

template<class ...Args>
std::string str(var_type_ptr tp, Args... args){
    return  "\'" + tp->to_string() + "\'" + str(args...);
}

void append_multidef_error(std::string desc, std::string nam, Locator loc){
    std::string id = "Error C01";
    std::string info = str(id, ": ", desc, " \'", nam, "\' is defined twice.");
    append_error(info, loc);
}

void append_nodef_error(std::string desc, std::string nam, Locator loc){
    std::string id = "Error C02";
    std::string info = str(id, ": ", desc, " \'", nam, "\' is not declared.");
    append_error(info, loc);
}


void append_mismatch_op_error(std::string desc, var_type_ptr tp, Locator loc){
    std::string id = "Error C03";
    std::string info = str(id, ": ", "Expression should be ", desc, " type, but it is ", tp, '.');
    append_error(info, loc);
}

void append_nomatch_op_error(std::string desc, Locator loc){
    std::string id = "Error C04";
    append_error(id + ": " + desc, loc);
}

void append_assign_error(std::string desc, Locator loc){
    std::string id = "Error C05";
    append_error(id + ": " + desc, loc);
}

void append_ref_error(std::string desc, Locator loc){
    std::string id = "Error C06";
    append_error(id + ": " + desc, loc);
}

void append_convert_error(var_type_ptr from, var_type_ptr to, Locator loc){
    std::string id = "Error C07";
    std::string info = str(id, ": Failed to convert ", from ," to ", to, " implicitly.");
    append_error(info, loc);
}

void append_force_convert_error(var_type_ptr from, var_type_ptr to, Locator loc){
    std::string id = "Error C08";
    std::string info = str(id, ": Failed to convert ", from, " to ", to, ".");
    append_error(info, loc);
}

void append_invalid_access_error(var_type_ptr tp, std::string mem, Locator loc){
    std::string id = "Error C09";
    std::string info = str(id, ": Type ", tp, " has no member named \'", mem, "\'.");
    append_error(info, loc);
}

void append_misplace_error(std::string stmt, std::string req_context, Locator loc){
    std::string id = "Error C10";
    std::string info = str(id, ": ", stmt, " statement out of ", req_context, " body.");
    append_error(info, loc);
}

void append_array_len_error(Locator loc){
    append_error("Error C11: Array length should be a int literal.", loc);
}

void append_infer_failed_error(std::string desc, Locator loc){
    std::string id = "Error C12";
    append_error(id + ": " + desc, loc);
}

void append_call_error(var_type_ptr fn, var_type_ptr args, Locator loc){
    std::string id = "Error C13";
    std::string info = str(id, ": Cannot call function ", fn, " with args: ", args, ".");
    append_error(info, loc);
}

void append_invalid_decl_error(std::string desc, Locator loc){
    std::string id = "Error C14";
    append_error(id + ": " + desc, loc);
}

void append_syntax_error (std::string desc, Locator loc) {
    std::string id = "Error B";
    append_error(id + ": " + desc, loc); 
}

void append_lexeme_error(std::string desc, Locator loc) {
    std::string id = "Error A";
    append_error(id + ": " + desc, loc);
}

void append_prim_shadowed_warning(std::string id, Locator loc) {
    
    append_error("Warning: identifier \"" + id + "\" shadows primtive type.", loc);
}