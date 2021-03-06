#ifndef TIKPP_ERROR_CODE_HPP
#define TIKPP_ERROR_CODE_HPP

#include <boost/system/error_code.hpp>

#include <string>

namespace tikpp {

namespace detail {

class error_category : public boost::system::error_category {
  public:
    [[nodiscard]] inline auto name() const noexcept -> const char * override {
        return "tikpp";
    }

    [[nodiscard]] inline auto message(int c) const -> std::string override;

    [[nodiscard]] inline auto default_error_condition(int c) const noexcept
        -> boost::system::error_condition override;
};

} // namespace detail

enum class [[nodiscard]] error_code : int {
    success = 0,       invalid_response,
    untagged_response, unknown_response_type,
    fatal_response,    no_such_item,
    invalid_argument,  interrupted,
    script_failure,    general_failure,
    api_failure,       tty_failure,
    return_value,      item_already_exists,
    unknown_parameter, login_failure,
    list_end,          unknown_error_category,
    unknown_error};

inline auto operator==(const boost::system::error_code &lhs,
                       error_code                       rhs) noexcept -> bool {
    return lhs.value() == static_cast<int>(rhs);
}

auto error_category() -> const boost::system::error_category &;

auto make_error_code(tikpp::error_code e) -> boost::system::error_code;

auto make_error_condition(tikpp::error_code e) -> std::error_condition;

} // namespace tikpp

namespace boost::system {

template <>
struct is_error_code_enum<tikpp::error_code> : std::true_type {};

template <>
struct is_error_condition_enum<tikpp::error_code> : std::true_type {};

} // namespace boost::system

#endif
