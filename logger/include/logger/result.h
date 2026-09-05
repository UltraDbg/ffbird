#ifndef LOGGER_RESULT_H
#define LOGGER_RESULT_H

#ifndef __linux__
#error "logger requires Linux"
#endif

#include <string>

namespace logger {

template <typename T>
struct Result {
    bool ok;
    T value;
    std::string error;

    static Result<T> success(const T& v) {
        Result<T> r;
        r.ok = true;
        r.value = v;
        return r;
    }

    static Result<T> success(T&& v) {
        Result<T> r;
        r.ok = true;
        r.value = std::move(v);
        return r;
    }

    static Result<T> failure(const std::string& e) {
        Result<T> r;
        r.ok = false;
        r.error = e;
        return r;
    }

    static Result<T> failure(std::string&& e) {
        Result<T> r;
        r.ok = false;
        r.error = std::move(e);
        return r;
    }

    Result() : ok(false) {}
};

template <>
struct Result<void> {
    bool ok;
    std::string error;

    static Result<void> success() {
        Result<void> r;
        r.ok = true;
        return r;
    }

    static Result<void> failure(const std::string& e) {
        Result<void> r;
        r.ok = false;
        r.error = e;
        return r;
    }

    static Result<void> failure(std::string&& e) {
        Result<void> r;
        r.ok = false;
        r.error = std::move(e);
        return r;
    }

    Result() : ok(false) {}
};

}  // namespace logger

#endif  // LOGGER_RESULT_H
