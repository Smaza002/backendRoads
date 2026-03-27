#include "config/environment.hpp"

#include <cstdlib>

namespace config {

std::optional<std::string> ProcessEnvironment::get(std::string_view key) const {
    const auto value = std::getenv(std::string{key}.c_str());
    if (value == nullptr) {
        return std::nullopt;
    }

    return std::string{value};
}

}  // namespace config
