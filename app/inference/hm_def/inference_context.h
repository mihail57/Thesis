
#ifndef INFERENCE_CONTEXT_H
#define INFERENCE_CONTEXT_H

#include "type_scheme.h"

class InferenceContext {
private:
    std::map<std::string, TypeScheme::ptr_t> _context;

public:
    using iterator = decltype(_context)::iterator;
    using const_iterator = decltype(_context)::const_iterator;
    using context_pair = iterator::value_type;

    auto begin() const { return _context.begin(); }
    auto cbegin() const { return _context.cbegin(); }
    auto end() const { return _context.end(); }
    auto cend() const { return _context.cend(); }

    bool has(const std::string& var);
    context_pair& get(const std::string& var);
    void set(const context_pair::first_type& var, const context_pair::second_type& scheme);
    InferenceContext with(const context_pair::first_type& var, const context_pair::second_type& scheme);
    TypeScheme::ptr_t generalize(Type::base_ptr_t& type);
};

#endif