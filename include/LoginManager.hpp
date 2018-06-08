#pragma once

#include "RequestHandler.hpp"

#include <array>
#include <mutex>
#include <string>
#include <unordered_map>

struct client_data_t {
    RequestHandler handler;
    port_t port;
    bool initialized;

    client_data_t()
        : initialized(false)
    {
    }

    client_data_t(RequestHandler&& rh, unsigned int port_p)
        : handler(std::move(rh))
        , port(port_p)
    {
    }

    client_data_t& operator=(client_data_t&& move)
    {
        this->handler = std::move(move.handler);
        this->port = std::move(move.port);
        this->initialized = std::move(move.initialized);

        return *this;
    }

    bool is_logged_in()
    {
        return this->initialized;
    }
};

class LoginManager {
    std::unordered_map<std::string, std::array<client_data_t, MAX_CONCURRENT_USERS>> users;

    std::mutex mutable m_login;
    std::mutex mutable m_map;

public:
    /**
     * Tries to login the given user_id.
     * 
     * @param user_id the UserID
     * 
     * @return the new user device
     */
    device_t login(std::string const& user_id);

    /**
     * Tries to logout the given user_id.
     * 
     * @param user_id the UserID
     * @param device the current user device
     * 
     * @return success or failure
     */
    bool logout(std::string const& user_id, device_t device);

    /**
     * Retrieves a reference to the client data.
     * 
     * @param user_id the UserID
     * @param device the current user device
     * 
     * @return a reference to the data
     */
    client_data_t& get_client_data(std::string const& user_id, device_t device) const;

private:
    device_t reserve_device(std::string const& user_id);
};