#ifndef ARGPARSER_ARG_PARSER_H
#define ARGPARSER_ARG_PARSER_H

#ifndef __linux__
#error "argparser requires Linux"
#endif

#include "argparser/arg_list.h"
#include "logger/result.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace argparser {

struct HelpEntry {
    std::string longName;
    std::string shortName;
    std::string desc;
};

class ArgParser {
public:
    ArgParser() {}

    void addArg(const std::string& longName, const std::string& shortName,
                const std::string& desc,
                std::function<void(ArgList&)> handler) {
        if (!longName.empty()) {
            handlers_[longName] = handler;
        }
        if (!shortName.empty()) {
            handlers_[shortName] = handler;
        }
        HelpEntry e;
        e.longName = longName;
        e.shortName = shortName;
        e.desc = desc;
        helpEntries_.push_back(e);
    }

    logger::Result<void> parse(int argc, const char** argv) {
        ArgList list(argc, argv);
        // skip program name
        if (list.hasNext()) {
            list.next();
        }
        while (list.hasNext()) {
            std::string token = list.next();
            if (token == "--help" || token == "-h") {
                printHelp();
                continue;
            }
            auto it = handlers_.find(token);
            if (it == handlers_.end()) {
                return logger::Result<void>::failure("Unknown argument: " + token);
            }
            try {
                it->second(list);
            } catch (const std::invalid_argument& ex) {
                return logger::Result<void>::failure(ex.what());
            } catch (const std::exception& ex) {
                return logger::Result<void>::failure(ex.what());
            } catch (...) {
                return logger::Result<void>::failure("Unknown error handling argument: " + token);
            }
        }
        return logger::Result<void>::success();
    }

    void printHelp() const {
        std::vector<HelpEntry> sorted = helpEntries_;
        std::sort(sorted.begin(), sorted.end(), [](const HelpEntry& a, const HelpEntry& b) {
            if (a.longName != b.longName) return a.longName < b.longName;
            return a.shortName < b.shortName;
        });
        std::cout << "Options:\n";
        for (size_t i = 0; i < sorted.size(); ++i) {
            const HelpEntry& e = sorted[i];
            std::cout << "  ";
            if (!e.shortName.empty()) {
                std::cout << e.shortName << ", ";
            } else {
                std::cout << "    ";
            }
            std::cout << e.longName;
            if (!e.desc.empty()) {
                std::cout << "\t" << e.desc;
            }
            std::cout << "\n";
        }
        // also show help flag itself if not registered
        bool hasHelp = false;
        for (size_t i = 0; i < sorted.size(); ++i) {
            if (sorted[i].longName == "--help" || sorted[i].shortName == "-h") {
                hasHelp = true;
                break;
            }
        }
        if (!hasHelp) {
            std::cout << "  -h, --help\tShow this help\n";
        }
    }

    const std::vector<HelpEntry>& getHelpEntries() const {
        return helpEntries_;
    }

private:
    std::unordered_map<std::string, std::function<void(ArgList&)> > handlers_;
    std::vector<HelpEntry> helpEntries_;
};

}  // namespace argparser

#endif  // ARGPARSER_ARG_PARSER_H
