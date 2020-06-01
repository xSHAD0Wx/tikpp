#ifndef TIKPP_COMMANDS_GET_ALL_HPP
#define TIKPP_COMMANDS_GET_ALL_HPP

#include "tikpp/models/converters.hpp"
#include "tikpp/request.hpp"

#include "fmt/format.h"
#include <boost/algorithm/string.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace tikpp::commands {

template <typename Model>
struct getall : tikpp::request {
    getall(std::uint32_t tag)
        : request {std::string {Model::api_path} + command_suffix, tag} {
        tikpp::models::proplist_collector<std::vector<std::string>>
            collector {};

        Model m {};
        m.convert(collector);

        auto proplist = boost::join(collector.proplist, ",");
        add_param(".proplist", std::move(proplist));
    }

    getall(std::uint32_t tag, std::vector<std::string> q) : getall(tag) {
        query(std::move(q));
    }

    static constexpr auto command_suffix = "/getall";
};

} // namespace tikpp::commands

#endif
