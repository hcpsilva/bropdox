#include "../include/Client.hpp"

int main(int argc, char* argv[])
{
    // int sockfd, n;
    // unsigned int length;
    // struct sockaddr_in serv_addr, from;
    // struct hostent* server;

    // char buffer[256];
    // if (argc < 2) {
    //     fprintf(stderr, "usage %s hostname\n", argv[0]);
    //     exit(0);
    // }

    // server = gethostbyname(argv[1]);
    // if (server == NULL) {
    //     fprintf(stderr, "ERROR, no such host\n");
    //     exit(0);
    // }

    // if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    //     printf("ERROR opening socket");
    // }

    // serv_addr.sin_family = AF_INET;
    // serv_addr.sin_port = htons(PORT);
    // serv_addr.sin_addr = *((struct in_addr*)server->h_addr);
    // bzero(&(serv_addr.sin_zero), 8);

    // printf("Enter the message: ");
    // bzero(buffer, 256);
    // fgets(buffer, 256, stdin);

    // n = sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr*)&serv_addr, sizeof(struct sockaddr_in));
    // if (n < 0) {
    //     printf("ERROR sendto");
    // }

    // length = sizeof(struct sockaddr_in);
    // n = recvfrom(sockfd, buffer, 256, 0, (struct sockaddr*)&from, &length);
    // if (n < 0) {
    //     printf("ERROR recvfrom");
    // }

    // printf("Got an ack: %s\n", buffer);

    // close(sockfd);
    // return 0;
    if (argc < 3 || argc > 4) {
        printf("Incorrect parameter usage, please refer to the following model:\n");
        printf("./mclient <userid> <address> <port>\n\n");

        return -1;
    }
    int port = atoi(argv[3]);
    char* host = argv[2];
    char* user = argv[1];

    /* hostent* server = gethostbyname(host);
    if (server == NULL) {
        printf("Host não encontrado.");
        return 0; //boo-hoo
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        printf("ERROR opening socket");

    server_address.sin_addr = *((struct in_addr*)server->h_addr);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    bzero(&(server_address.sin_zero), 8);

    struct file_info finfo("sync_dir_john/teste.txt");
    handshake_t hand(req::receive, user, finfo, 0);
    n = sendto(sockfd, &hand, sizeof(handshake_t), 0, (const struct sockaddr*)&server_address, sizeof(struct sockaddr_in)); */

    
    hostent* server = gethostbyname(host);
    if (server == NULL) {
        printf("Host não encontrado.");
        return 0; //boo-hoo
    }

    SocketHandler sock_hand(port, server);
    FileHandler file_hand(user);

    std::strcat(user, "\0");
    handshake_t hand(req::receive, user);
    sock_hand.send_packet(&hand, sizeof(handshake_t));

    data_buffer_t* syn_data = sock_hand.wait_packet(sizeof(syn_t));
    syn_t* syn = convert_to_syn(syn_data);

    SocketHandler req_sock_hand(syn->port, server);
    long int packet_size;
    packet_t** packets = file_hand.get_file("dropbox.png", packet_size);

    struct file_info finfo = file_hand.get_file_info("dropbox.png");
    file_data_t file_data(finfo, packet_size);

    req_sock_hand.send_packet(&file_data, sizeof(file_data_t));

    data_buffer_t* ack_data = req_sock_hand.wait_packet(sizeof(ack_t));
    ack_t* ack = convert_to_ack(ack_data);

    std::cout << packet_size << std::endl;
    for (int i = 0; i < packet_size; i++) {
        std::cout << packets[i] << std::endl;
        if (!req_sock_hand.send_packet(packets[i], sizeof(packet_t))) {
            printf("Failed sending packet number %d\n", i);
        }
        usleep(15);
    }
    printf("Finished trying to send the packets \n");

    data_buffer_t* ack_data2 = req_sock_hand.wait_packet(sizeof(ack_t));
    ack_t* ack2 = convert_to_ack(ack_data2);

    if (ack2->confirmation) {
        printf("Success!");
    }

    /* if (n < 0) {
        printf("ERROR sendto");
        return -1;
    }

    length = sizeof(struct sockaddr_in);
    n = recvfrom(sockfd, bufferf, 256, 0, (struct sockaddr*)&from, &length);
    if (n < 0)
        printf("ERROR recvfrom"); */

    /* printf("Got an ack: %s\n", bufferf);

    close(sockfd); */

    return 0;
}