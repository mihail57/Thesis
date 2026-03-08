
#ifndef ERROR_H
#define ERROR_H

#include "source_loc.h"

struct TypingError {
    std::string text;
    SourceLoc at;
};

#endif