
#include "type_scheme.h"

class TypingContext {
private:
    std::map<Variable, std::shared_ptr<TypeScheme>> _context;

public:
    using iterator = decltype(_context)::iterator;
    using const_iterator = decltype(_context)::const_iterator;
    using context_pair = iterator::value_type;

    auto begin() const { return _context.begin(); }
    auto cbegin() const { return _context.cbegin(); }
    auto end() const { return _context.end(); }
    auto cend() const { return _context.cend(); }

    bool has(const Variable& var) {
        return _context.find(var) != end();
    }

    context_pair& get(const Variable& var) {
        return *_context.find(var);
    }

    void set(const context_pair::first_type& var, const context_pair::second_type& scheme) {
        _context.insert(std::make_pair(var, scheme));
    }

    TypingContext& with(const context_pair::first_type& var, const context_pair::second_type& scheme) {
        set(var, scheme);
        return *this;
    }

    std::shared_ptr<TypeScheme> generalize(std::shared_ptr<Type>& type) {
        std::vector<std::shared_ptr<TypeVar>> type_vars;
        type->add_vars(type_vars);

        std::vector<std::shared_ptr<TypeVar>> new_binded;
        new_binded.reserve(type_vars.size());

        for(const auto& tv : type_vars) {
            if(std::find_if(
                cbegin(), cend(),
                [tv](const TypingContext::const_iterator::value_type& s) {
                    return s.first.name == tv->name;
                }
            ) == cend())
                new_binded.push_back(tv);
        }

        new_binded.shrink_to_fit();

        return std::shared_ptr<TypeScheme>(
            new PolyTypeScheme(new_binded, type)
        );
    }
};