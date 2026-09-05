#ifndef ANTICRASH_HANDLER_H
#define ANTICRASH_HANDLER_H

#ifndef __linux__
#error "anticrash requires Linux"
#endif

#include <string>

namespace anticrash {

bool install(const std::string& logFile);
void uninstall();

}  // namespace anticrash

#endif  // ANTICRASH_HANDLER_H
