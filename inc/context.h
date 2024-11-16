#pragma once

#include "parse.h"
#include <vector>

void append_error(std::string);
void append_error(std::string, Locator);

using error_list = std::vector<std::pair<std::string, Locator>>;

error_list& get_error_list();
