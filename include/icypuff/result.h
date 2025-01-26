#pragma once

#include <variant>
#include <string_view>

namespace icypuff {

enum class ErrorCode {
  kOk = 0,
  kInvalidArgument,
  kUnknownCodec,
  // Add more error codes as needed
};

template<typename T>
class Result {
public:
  struct Error {
    ErrorCode code;
    std::string_view message;
  };

  // Construct with value
  constexpr Result(T value) : var_(std::move(value)) {}
  
  // Construct with error
  constexpr Result(ErrorCode code, std::string_view message) 
    : var_(Error{code, message}) {}

  // Check if result contains a value
  constexpr bool ok() const noexcept {
    return std::holds_alternative<T>(var_);
  }

  // Get the contained value. Must check ok() first.
  constexpr const T& value() const& noexcept {
    return std::get<T>(var_);
  }
  
  constexpr T&& value() && noexcept {
    return std::get<T>(std::move(var_));
  }

  // Get the error. Must check !ok() first.
  constexpr const Error& error() const& noexcept {
    return std::get<Error>(var_);
  }

private:
  std::variant<T, Error> var_;
};

} // namespace icypuff 