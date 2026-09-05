#ifndef LOGGER_RESULT_H
#define LOGGER_RESULT_H

// Deprecated: Result<T> has moved to utils/result.h
// This header is kept for backward compatibility and will be removed.
// New code should: #include "utils/result.h" and use utils::Result<T>.

#ifndef __linux__
#error "logger requires Linux"
#endif

#include "utils/result.h"

namespace logger {

// Backward compatibility alias — Result now lives in utils
template <typename T>
using Result = utils::Result<T>;

}  // namespace logger

#endif  // LOGGER_RESULT_H
