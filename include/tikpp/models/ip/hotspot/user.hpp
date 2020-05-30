#ifndef TIKPP_MODELS_IP_HOTSPOT_USER_HPP
#define TIKPP_MODELS_IP_HOTSPOT_USER_HPP

#include "tikpp/models/model.hpp"

#include <string>

namespace tikpp::models::ip::hotspot {

struct user final : tikpp::models::model {
    static constexpr auto api_path = "/ip/hotspot/user";

    std::string name;
    std::string password;
    std::string profile;

    bool disabled;

    template <template <typename> typename Converter, typename Map>
    inline void convert(Converter<Map> &c) {
        model::convert<Converter, Map>(c);

        c["name"] % name;
        c["password"] % password;
        c["profile"] % profile;
        c["disabled"] % disabled;
    }
};

} // namespace tikpp::models::ip::hotspot

#endif
