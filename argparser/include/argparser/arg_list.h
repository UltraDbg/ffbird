#ifndef ARGPARSER_ARG_LIST_H
#define ARGPARSER_ARG_LIST_H

#ifndef __linux__
#error "argparser requires Linux"
#endif

#include <string>
#include <vector>

namespace argparser {

class ArgList {
public:
    ArgList(int argc, const char** argv) : args_(), index_(0) {
        for (int i = 0; i < argc; ++i) {
            args_.push_back(argv[i] ? std::string(argv[i]) : std::string());
        }
    }

    explicit ArgList(const std::vector<std::string>& args) : args_(args), index_(0) {}

    bool hasNext() const {
        return index_ < args_.size();
    }

    std::string next() {
        if (!hasNext()) return std::string();
        return args_[index_++];
    }

    const char* nextOrNull() {
        if (!hasNext()) return nullptr;
        const std::string& s = args_[index_++];
        return s.c_str();
    }

    std::string peek() const {
        if (!hasNext()) return std::string();
        return args_[index_];
    }

    size_t remaining() const {
        return hasNext() ? args_.size() - index_ : 0;
    }

    void reset() {
        index_ = 0;
    }

private:
    std::vector<std::string> args_;
    size_t index_;
};

}  // namespace argparser

#endif  // ARGPARSER_ARG_LIST_H
