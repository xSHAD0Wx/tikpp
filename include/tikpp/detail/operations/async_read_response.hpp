#ifndef TIKPP_DETAIL_OPERATIONS_ASYNC_READ_RESPONSE_HPP
#define TIKPP_DETAIL_OPERATIONS_ASYNC_READ_RESPONSE_HPP

#include "tikpp/detail/async_result.hpp"
#include "tikpp/detail/operations/async_read_word.hpp"

#include "tikpp/error_code.hpp"
#include "tikpp/response.hpp"

#include <boost/system/error_code.hpp>

#include <string>
#include <type_traits>
#include <vector>

namespace tikpp::detail::operations {

template <typename AsyncReadStream, typename Handler>
struct async_read_response_op final {
    inline async_read_response_op(AsyncReadStream &sock, Handler &&handler)
        : sock_ {sock}, handler_ {std::forward<Handler>(handler)} {
    }

    void operator()(const boost::system::error_code &err, std::string &&word) {
        if (err) {
            return handler_(err, tikpp::response {});
        }

        if (word.empty()) {
            if (!tikpp::response::is_valid_response(words_)) {
                return handler_(
                    tikpp::make_error_code(tikpp::error_code::invalid_response),
                    tikpp::response {});
            }

            tikpp::response resp {words_};

            if (resp.type() == tikpp::response_type::fatal) {
                handler_(
                    tikpp::make_error_code(tikpp::error_code::fatal_response),
                    tikpp::response {});
            } else if (!resp.tag().has_value()) {
                handler_(tikpp::make_error_code(
                             tikpp::error_code::untagged_response),
                         tikpp::response {});
            } else {
                handler_(boost::system::error_code {}, std::move(resp));
            }

            return;
        }

        words_.emplace_back(std::move(word));
        async_read_word(sock_, std::move(*this));
    }

    inline void initiate() {
        async_read_word(sock_, std::move(*this));
    }

  private:
    AsyncReadStream &        sock_;
    Handler                  handler_;
    std::vector<std::string> words_;
};

template <typename AsyncReadStream, typename CompletionToken>
decltype(auto) async_read_response(AsyncReadStream & sock,
                                   CompletionToken &&token) {
    GENERATE_COMPLETION_HANDLER(
        void(const boost::system::error_code &, tikpp::response &&), token,
        handler, result);

    async_read_response_op<AsyncReadStream, handler_type> {sock,
                                                           std::move(handler)}
        .initiate();

    return result.get();
}

} // namespace tikpp::detail::operations

#endif
