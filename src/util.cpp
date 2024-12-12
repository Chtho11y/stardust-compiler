#include "util.h"
#include <sstream>

namespace ansi{

std::string color(ANSIColor foreground, ANSIColor style) {
    std::ostringstream oss;
    oss << "\033[" << static_cast<int>(foreground);

    if (style != RESET) {
        oss << ";" << static_cast<int>(style);
    }

    oss << "m";
    return oss.str();
}

}

namespace util{

uint64_t str_to_int(const std::string& str){
    uint64_t res = 0;
    if(str.size() > 2){
        if(str[1] == 'x' || str[1] == 'X'){
            for(int i = 2; i < str.size(); ++i){
                auto d = str[i] - '0';
                if(d >= 10)
                    d = str[i] - 'A' + 10;
                if(d >= 16)
                    d = str[i] - 'a' + 10;
                res = res * 16 + d;
            }
        }else if(str[1] == 'b' || str[1] == 'B'){
            for(int i = 2; i < str.size(); ++i){
                res = res * 2 + (str[i] - '0');
            }      
        }
    }
    return atoi(str.c_str());
}

}