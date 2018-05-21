#include "../include/SocketHandler.hpp"

SocketHandler::SocketHandler(in_port_t port, hostent* server)
{
    struct timeval timeout = {0, TIMEOUT};
    
    this->sockfd = init_unix_socket(this->peer_address, port, server);
    this->peer_len = sizeof(struct sockaddr_in);

    setsockopt(this->sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval));
}

SocketHandler::SocketHandler(in_port_t port, sockaddr_in caddress)
{
    struct timeval timeout = {0, TIMEOUT};
    
    this->peer_address = caddress;
    this->sockfd = init_unix_socket(this->handler_address, port);
    this->peer_len = sizeof(struct sockaddr_in);

    setsockopt(this->sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval));

    if (bind(this->sockfd, (struct sockaddr*)&(this->handler_address), sizeof(struct sockaddr)) < 0) {
        this->log("Error while binding the socket, please try again...");
    }
}

SocketHandler::SocketHandler(in_port_t port)
{
    this->sockfd = init_unix_socket(this->handler_address, port);
    this->peer_len = sizeof(struct sockaddr_in);

    if (bind(sockfd, (struct sockaddr*)&(this->handler_address), sizeof(struct sockaddr)) < 0) {
        this->log("Error while binding the socket, please try again...");
    }
}

data_buffer_t* SocketHandler::wait_packet(size_t size)
{
    data_buffer_t* buffer = new data_buffer_t[size];

    int desc = recvfrom(this->sockfd, (void*)buffer, size, 0, (struct sockaddr*)&(this->peer_address), &(this->peer_len));
    if (desc < 0) {
        this->log("Error while receiving packet...");
        delete []buffer;
        return nullptr;
    }

    //! Caller must delete this object later
    return buffer;
}

bool SocketHandler::send_packet(void* data, size_t size)
{
    int desc = sendto(this->sockfd, data, size, 0, (struct sockaddr*)&(this->peer_address), sizeof(struct sockaddr_in));
    if (desc < 0) {
        this->log("Error while sending packet...");
        return false;
    }

    return true;
}

sockaddr_in SocketHandler::get_last_peeraddr()
{
    return this->peer_address;
}

SocketHandler::~SocketHandler()
{
    close(this->sockfd);
}

void SocketHandler::log(char const* message)
{
    printf("SocketHandler [sockfd: %d]: %s\n",
        this->sockfd,
        message
    );
}