#ifndef ARGPARSER_ARG_H
#define ARGPARSER_ARG_H

#ifndef __linux__
#error "argparser requires Linux"
#endif

#include "argparser/arg_parser.h"
#include "argparser/arg_list.h"

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <type_traits>

namespace argparser {

// handleValue overloads

inline void handleValue(const std::string& s, std::string& out) {
    out = s;
}

inline void handleValue(const std::string& s, int& out) {
    try {
        size_t pos = 0;
        int v = std::stoi(s, &pos);
        if (pos != s.size()) throw std::invalid_argument("Invalid integer: " + s);
        out = v;
    } catch (const std::out_of_range&) {
        throw std::invalid_argument("Integer out of range: " + s);
    } catch (const std::invalid_argument&) {
        throw std::invalid_argument("Invalid integer: " + s);
    }
}

inline void handleValue(const std::string& s, float& out) {
    try {
        size_t pos = 0;
        float v = std::stof(s, &pos);
        if (pos != s.size()) throw std::invalid_argument("Invalid float: " + s);
        out = v;
    } catch (const std::out_of_range&) {
        throw std::invalid_argument("Float out of range: " + s);
    } catch (const std::invalid_argument&) {
        throw std::invalid_argument("Invalid float: " + s);
    }
}

inline void handleValue(const std::string& s, double& out) {
    try {
        size_t pos = 0;
        double v = std::stod(s, &pos);
        if (pos != s.size()) throw std::invalid_argument("Invalid double: " + s);
        out = v;
    } catch (const std::out_of_range&) {
        throw std::invalid_argument("Double out of range: " + s);
    } catch (const std::invalid_argument&) {
        throw std::invalid_argument("Invalid double: " + s);
    }
}

inline void handleValue(const std::string& s, bool& out) {
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "true" || lower == "on" || lower == "yes" || lower == "1") {
        out = true;
    } else if (lower == "false" || lower == "off" || lower == "no" || lower == "0") {
        out = false;
    } else {
        throw std::invalid_argument("Invalid bool value: " + s);
    }
}

template <typename T>
void handleValue(const std::string& s, std::vector<T>& out) {
    T v;
    handleValue(s, v);
    out.push_back(v);
}

template <typename T>
class Arg {
public:
    Arg(ArgParser& parser, const std::string& longName, const std::string& shortName,
        const std::string& desc, const T& def = T())
        : value_(def) {
        // Use type trait to branch for bool
        addToParser(parser, longName, shortName, desc, typename std::is_same<T, bool>::type());
    }

    const T& get() const {
        return value_;
    }

    operator const T&() const {
        return value_;
    }

private:
    void addToParser(ArgParser& parser, const std::string& longName, const std::string& shortName,
                     const std::string& desc, std::false_type) {
        parser.addArg(longName, shortName, desc, [this](ArgList& list) {
            if (!list.hasNext()) {
                throw std::invalid_argument("Missing value for argument");
            }
            std::string token = list.next();
            T tmp = value_;
            handleValue(token, tmp);
            value_ = tmp;
        });
    }

    void addToParser(ArgParser& parser, const std::string& longName, const std::string& shortName,
                     const std::string& desc, std::true_type) {
        parser.addArg(longName, shortName, desc, [this](ArgList& list) {
            if (!list.hasNext()) {
                value_ = true;
                return;
            }
            std::string peek = list.peek();
            if (!peek.empty() && peek[0] == '-') {
                value_ = true;
                return;
            }
            std::string token = list.next();
            bool b = false;
            handleValue(token, b);
            value_ = b;
        });
    }

    T value_;
};

}  // namespace argparser

#endif  // ARGPARSER_ARG_H
