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