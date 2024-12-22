#pragma once

#include "var_type.h"

void append_error(Locator);
void append_error(std::string);
void set_error_message(std::string);
void append_error(std::string, Locator);
using error_list = std::vector<std::pair<std::string, Locator>>;

error_list& get_error_list();

//Type C: semantic error
void append_multidef_error(std::string desc, std::string id, Locator loc);
void append_nodef_error(std::string desc, std::string id, Locator loc);

void append_mismatch_op_error(std::string desc, var_type_ptr tp, Locator loc);
void append_nomatch_op_error(std::string desc, Locator loc);
void append_assign_error(std::string desc, Locator loc);
void append_ref_error(std::string desc, Locator loc);

void append_convert_error(var_type_ptr from, var_type_ptr to, Locator loc);
void append_force_convert_error(var_type_ptr from, var_type_ptr to, Locator loc);

void append_invalid_access_error(var_type_ptr tp, std::string mem, Locator loc);

void append_misplace_error(std::string stmt, std::string req_context, Locator loc);
void append_array_len_error(Locator loc);

void append_infer_failed_error(std::string desc, Locator loc);

void append_call_error(var_type_ptr fn, var_type_ptr args, Locator loc);

void append_invalid_decl_error(std::string desc, Locator loc);
void append_generic_error(std::string desc, Locator loc);

void append_invalid_impl_error(std::string desc, Locator loc);

void append_lexeme_error(std::string desc, Locator loc);

void append_syntax_error(std::string desc, Locator loc, bool is_fixed = false);

void append_prim_shadowed_warning(std::string id, Locator loc);