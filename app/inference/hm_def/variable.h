
#include <string>

struct Variable {
    std::string name;

    Variable(std::string n) : name(n) {};

    inline bool operator<(const Variable& b) const {
        return this->name < b.name;
    }
} ;