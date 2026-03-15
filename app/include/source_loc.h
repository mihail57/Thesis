
#ifndef SOURCE_LOC_H
#define SOURCE_LOC_H

#include <string>
#include <stdexcept>

struct SourceLoc {
    using pos_t = std::string::size_type;
    pos_t start;
    pos_t end;

    SourceLoc() : SourceLoc(0, 0) {};
    SourceLoc(pos_t start, pos_t end) : start(start), end(end) {};

    SourceLoc operator+(const SourceLoc& other) const {        
        return SourceLoc(start, other.end);
    }

    SourceLoc& operator+=(const SourceLoc& other) {
        end = other.end;
        return *this;
    }
};

#endif