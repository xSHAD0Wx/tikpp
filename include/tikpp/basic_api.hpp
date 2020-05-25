#ifndef TIKPP_BASIC_API_HPP
#define TIKPP_BASIC_API_HPP

#include "tikpp/detail/operations/async_read_response.hpp"
#include "tikpp/error_code.hpp"
#include "tikpp/request.hpp"
#include "tikpp/response.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/error_code.hpp>

#include <atomic>
#include <cassert>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>

namespace tikpp {

enum class api_version { v1, v2 };

enum class api_state {
    closed,
    connecting,
    connected,
};

template <typename AsyncStream>
class basic_api : public std::enable_shared_from_this<basic_api<AsyncStream>> {

    using connect_handler =
        std::function<void(const boost::system::error_code &)>;
    using read_handler = std::function<bool(const boost::system::error_code &,
                                            tikpp::response &&)>;

  public:
    void async_open(const std::string &host,
                    std::uint16_t      port,
                    connect_handler && handler) {
        if (state() == api_state::connecting) {
            return handler(boost::asio::error::in_progress);
        }

        assert(state() == api_state::closed);

        boost::system::error_code ec {};
        auto address = boost::asio::ip::address::from_string(host, ec);

        if (ec) {
            return handler(ec);
        }

        state(api_state::connecting);
        sock_.async_connect({address, port},
                            [self = this->shared_from_this(),
                             handler {std::move(handler)}](const auto &err) {
                                assert(self->state() == api_state::connecting);

                                if (!err) {
                                    self->state(api_state::connected);
                                    self->start();
                                } else {
                                    self->state(api_state::closed);
                                }

                                handler(err);
                            });
    }

    inline void close() {
        assert(is_open());
        sock_.close();
        state(api_state::closed);
    }

    [[nodiscard]] inline auto make_request(std::string command)
        -> std::shared_ptr<tikpp::request> {
        return std::make_shared<tikpp::request>(std::move(command),
                                                current_tag_.fetch_add(1));
    }

    void send(std::shared_ptr<request> req, read_handler &&cb) {
        assert(req != nullptr);

        io_.post([self = this->shared_from_this(), req {std::move(req)},
                  cb {std::move(cb)}]() mutable {
            self->send_queue_.emplace_back(
                std::make_pair(std::move(req), std::move(cb)));
            self->send_next();
        });
    }

    [[nodiscard]] inline auto state() const noexcept -> api_state {
        return state_.load();
    }

    inline void state(api_state state) {
        state_.store(state);
    }

    [[nodiscard]] inline auto version() -> api_version {
        return version_;
    }

    [[nodiscard]] inline auto socket() noexcept -> AsyncStream & {
        return sock_;
    }

    [[nodiscard]] inline auto is_open() const noexcept -> bool {
        return state() != api_state::closed && sock_.is_open();
    }

    static inline auto create(boost::asio::io_context &io, api_version ver)
        -> std::shared_ptr<basic_api<AsyncStream>> {
        return std::shared_ptr<basic_api<AsyncStream>>(
            new basic_api<AsyncStream> {io, ver});
    }

  protected:
    explicit basic_api(boost::asio::io_context &io, api_version version)
        : io_ {io},
          sock_ {io},
          version_ {version},
          state_ {api_state::closed},
          current_tag_ {0} {
    }

    void send_next() {
        assert(!send_queue_.empty());

        auto [req, cb] = std::move(send_queue_.front());
        send_queue_.pop_front();

        if (!is_open()) {
            cb(boost::asio::error::not_connected, {});
            return;
        }

        auto buf = std::make_shared<std::vector<std::uint8_t>>();
        req->encode(*buf);

        boost::asio::async_write(
            sock_, boost::asio::buffer(*buf),
            [self = this->shared_from_this(), buf, req {std::move(req)},
             cb {std::move(cb)}](const auto &err, const auto &sent) mutable {
                if (!self->is_open()) {
                    cb(boost::asio::error::not_connected, {});
                    return;
                }

                if (err) {
                    self->close();
                    cb(err, {});
                } else {
                    self->read_cbs_.emplace(
                        std::make_pair(req->tag(), std::move(cb)));
                }

                if (!self->send_queue_.empty()) {
                    self->send_next();
                }
            });
    }

    inline void start() {
        assert(is_open());
        read_next_response();
    }

    void read_next_response() {
        if (!is_open()) {
            return;
        }

        tikpp::detail::operations::async_read_response(
            sock_, [self = this->shared_from_this()](const auto &err,
                                                     auto &&     resp) mutable {
                if (!self->is_open()) {
                    return;
                }

                if (err) {
                    return self->on_error(err);
                }

                self->on_response(std::move(resp));
            });
    }

    void on_response(tikpp::response &&resp) {
        if (read_cbs_.find(resp.tag().value()) == read_cbs_.end()) {
            on_error(tikpp::error_code::invalid_response_tag);
        } else if (!read_cbs_[resp.tag().value()]({}, std::move(resp))) {
            read_cbs_.erase(resp.tag().value());
        }

        read_next_response();
    }

    void on_error(const boost::system::error_code &err) {
        // handle error
        close();
        std::cout << "Error: " << err.message() << std::endl;
    }

  private:
    boost::asio::io_context &io_;
    AsyncStream              sock_;

    const api_version      version_;
    std::atomic<api_state> state_;

    std::atomic_uint32_t                                          current_tag_;
    std::deque<std::pair<std::shared_ptr<request>, read_handler>> send_queue_;
    std::map<std::uint32_t, read_handler>                         read_cbs_;
};

} // namespace tikpp

#endif
