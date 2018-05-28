#include "../include/Client.hpp"

Client::Client(char* uid)
    : userid(uid)
{
    this->file_handler = new FileHandler(this->userid);
}

void Client::command_line_interface()
{
    std::string input;
    unsigned int length;
    std::vector<std::string> tokens;

    while (true) {
        std::cout << "$ ";
        std::getline(std::cin, input);

        length = input.length();
        if (length > 0) {
            boost::split(tokens, input, boost::is_any_of(" "));

            this->log("Parsing input");
            this->parse_input(tokens);
        }
    }
}

bool Client::parse_input(std::vector<std::string> tokens)
{
    std::string command(tokens[0]);

    if (command == "login") {
        std::string address(tokens[1]);
        std::string port_s(tokens[2]);
        in_port_t port = atoi(port_s.c_str());

        return this->login_server(address.c_str(), port);
    } else if (command == "upload") {
        std::string file_path(tokens[1]);

        // We start by sending a handshake containing the request to the server
        if (!this->send_handshake(req::receive)) {
            return false;
        }

        return this->send_file(file_path.c_str());
    } else if (command == "download") {
        std::string file_path(tokens[1]);

        // We start by sending a handshake containing the request to the server
        if (!this->send_handshake(req::send)) {
            return false;
        }

        return this->get_file(file_path.c_str());
    } else if (command == "delete") {
        std::string file_path(tokens[1]);

        // We start by sending a handshake containing the request to the server
        if (!this->send_handshake(req::del)) {
            return false;
        }

        return this->delete_file(file_path.c_str());
    } else if (command == "exit") {
        return this->close_session();
    } else {
        std::cout << "usage:\n"
                  << "login <hostname> <port>\n"
                  << "upload <file path>\n"
                  << "download <file>\n"
                  << "exit\n";
        return false;
    }

    this->log("Unknown command");
    return false;
}

bool Client::login_server(char const* host, int port)
{
    this->server = gethostbyname(host);
    if (server == NULL) {
        this->log("Server not found");
        return false;
    }

    try {
        this->sock_handler_server = new SocketHandler(port, this->server);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        this->log("Failed to connect with the server");
        return false;
    }

    this->log("Logged to server");

    //TODO: Sync the client with all the server's files
    //TODO: Start sync daemon

    return true;
}

bool Client::sync_client()
{
    return false;
}

bool Client::send_file(char const* file)
{
    ack_t* ack;
    packet_t** packets;
    long int file_size_in_packets;
    byte_t *returned_ack;

    // Get the file data and file size in packets
    packets = this->file_handler->get_file(file, file_size_in_packets);

    // We then send the file data to the server
    file_data_t file_data(this->file_handler->get_file_info(file), file_size_in_packets);
    this->sock_handler_req->send_packet(&file_data, sizeof(file_data_t));

    // And wait for the server's ACK
    returned_ack = this->sock_handler_req->wait_packet(sizeof(ack_t));
    ack = convert_to_ack(returned_ack);

    // If it's 'false' we abort
    if (!ack->confirmation) {
        this->log("Bad ACK received");
        return false;
    }

    delete ack;
    delete[] returned_ack;

    // Packet sending loop
    for (int i = 0; i < file_size_in_packets; i++) {
        this->sock_handler_req->send_packet(packets[i], sizeof(packet_t));
        usleep(15);
    }
    this->log("Finished sending the file");

    // The Client then procedes to wait for the RequestHandler's ack, which will contain the
    // number of packets that he received.
    // This number of received packets will indicate a possible missing packet in the transmission,
    // calling for a repeat of the send_file() operation.
    returned_ack = this->sock_handler_req->wait_packet(sizeof(ack_t));
    ack = convert_to_ack(returned_ack);

    // If it's 'false' we try again
    if (!ack->confirmation) {
        this->log("Bad ACK received, sending file again...");
        this->send_file(file);
    } else {
        this->log("Success uploading the file");
    }


    delete ack;
    delete[] returned_ack;
    delete this->sock_handler_req;

    return true;
}

bool Client::get_file(char const* file)
{
    packet_t* received = NULL;
    byte_t* received_packet;
    unsigned int received_packet_number = 0, packets_to_be_received;
    file_data_t* received_finfo;

    file_data_t file_data(this->file_handler->get_file_info(file), 0);
    this->sock_handler_req->send_packet(&file_data, sizeof(file_data_t));

    received_packet = this->sock_handler_req->wait_packet(sizeof(file_data_t));
    received_finfo = convert_to_file_data(received_packet);
    delete[] received_packet;

    if (received_finfo->num_packets == 0) {
        this->log("Bad file info received");
        return false;
    }

    packets_to_be_received = received_finfo->num_packets;

    // Uses said number of packets to declare an array of byte_t pointers
    // pointing to the received data
    byte_t* recv_file[packets_to_be_received];

    // Packet receiving loop
    for (unsigned int i = 0; i < packets_to_be_received; i++) {
        received_packet = this->sock_handler_req->wait_packet(sizeof(packet_t));

        // If the received packet is NULL, we do nothing
        if (received_packet != nullptr) {
            received = convert_to_packet(received_packet);
            //TODO: Copy the received data array to the recv_file array
            recv_file[received->num] = received->data;
            received_packet_number++;
        }

        delete[] received_packet;
    }

    // After receiving all packets, we send an ack with true if we received all the packets or
    // false if we didn't.
    // If the number doesnt match the expected number, the server should do something about it.
    // Also, we do nothing if the number doesnt match.
    if (received_packet_number == packets_to_be_received) {
        this->log("Success receiving the file");

        ack_t ack(true);
        this->sock_handler_req->send_packet(&ack, sizeof(ack_t));

        this->file_handler->write_file(file, recv_file, packets_to_be_received);
    } else {
        this->log("Failure receiving the file");

        ack_t ack(false);
        this->sock_handler_req->send_packet(&ack, sizeof(ack_t));
    }

    delete this->sock_handler_req;
    return true;
}

bool Client::delete_file(char const* file)
{
    byte_t* returned_ack;
    ack_t* ack;
    
    if (!this->file_handler->delete_file(file)) {
        this->log("File does not exist");
        return false;
    }

    file_data_t file_data(this->file_handler->get_file_info(file), 0);
    this->sock_handler_req->send_packet(&file_data, sizeof(file_data_t));

    returned_ack = this->sock_handler_req->wait_packet(sizeof(ack_t));
    ack = convert_to_ack(returned_ack);

    // If it's 'false' we try again
    if (!ack->confirmation) {
        this->log("Failed deleting the file");
        return false;
    } else {
        this->log("Success deleting the file");
        return true;
    }
}

bool Client::close_session()
{
    return false;
}

bool Client::send_handshake(req request)
{
    byte_t* syn_data;
    syn_t* syn;

    // Sends a handshake to the server containing the request type
    handshake_t hand(request, this->userid.c_str());
    this->sock_handler_server->send_packet(&hand, sizeof(handshake_t));
    this->log("Sent handshake to server");

    // Waits the SYN data containing the port
    syn_data = this->sock_handler_server->wait_packet(sizeof(syn_t));
    syn = convert_to_syn(syn_data);

    // If the SYN is bad, we abort the process
    if (!syn->confirmation) {
        this->log("Received a bad SYN");
        return false;
    }

    try {
        this->sock_handler_req = new SocketHandler(syn->port, this->server);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        this->log("Failed establishing communications with the new socket");
        return false;
    }

    return true;
}

void Client::log(char const* message)
{
    printf("Client [UID: %s]: %s\n", this->userid.c_str(), message);
}