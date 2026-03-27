#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace config {

class Environment {
public:
    virtual ~Environment() = default;
    virtual std::optional<std::string> get(std::string_view key) const = 0;
};

class ProcessEnvironment final : public Environment {
public:
    std::optional<std::string> get(std::string_view key) const override;
};

}  // namespace config
