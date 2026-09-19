#include "utils.hpp"

namespace everward::simulation {
namespace utils {

// Implementation of is_string_empty function
[[nodiscard]] inline bool is_string_empty(const std::string& str) {
    return str.empty();
}

} // namespace utils
} // namespace everward::simulation
