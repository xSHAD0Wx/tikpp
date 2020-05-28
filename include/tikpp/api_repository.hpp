#ifndef TIKPP_API_REPOSITORY_HPP
#define TIKPP_API_REPOSITORY_HPP

#include "tikpp/basic_api.hpp"
#include "tikpp/commands/getall.hpp"

#include <boost/asio/async_result.hpp>
#include <boost/system/error_code.hpp>

#include <iostream>
#include <memory>
#include <type_traits>

namespace tikpp {

template <typename Model, typename ApiPtr>
struct api_repository {
    explicit api_repository(ApiPtr api) : api_ {std::move(api)} {
    }

    template <typename CompletionToken>
    void load_all(CompletionToken &&token) {
        using signature_type = void(const boost::system::error_code &,
                                    std::shared_ptr<std::vector<Model>> &&);
        using result_type =
            boost::asio::async_result<std::decay_t<CompletionToken>,
                                      signature_type>;
        using handler_type = typename result_type::completion_handler_type;

        handler_type handler {std::forward<CompletionToken>(token)};
        result_type  result {handler};

        auto req = api_->template make_request<tikpp::commands::getall>(
            Model::api_path);

        api_->async_send(
            std::move(req), [handler {std::move(handler)},
                             ret = std::make_shared<std::vector<Model>>()](
                                const auto &err, auto &&resp) {
                if (err) {
                    handler(err, decltype(ret) {nullptr});
                    return false;
                }

                if (resp.empty()) {
                    handler(boost::system::error_code {}, std::move(ret));
                    return false;
                }

                ret->emplace_back(Model::create(resp));
                return true;
            });

        return result.get();
    }

  private:
    ApiPtr api_;
};

template <typename Model, typename ApiPtr>
inline auto make_repository(ApiPtr api) -> api_repository<Model, ApiPtr> {
    return api_repository<Model, ApiPtr> {api};
}

} // namespace tikpp

#endif
