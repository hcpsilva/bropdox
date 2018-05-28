#ifndef BROPBOXUTIL_HPP
#define BROPBOXUTIL_HPP

#ifndef BOOST_ALL_DYN_LINK
#define BOOST_ALL_DYN_LINK
#endif

#define MAXNAME 255
#define MAXFILES 65536
#define PACKETSIZE 16384

#define MAX_FILE_LIST_SIZE 10

#define DAEMON_SLEEP 10000000
#define TIMEOUT 500000

#define MAXPORT 65535

#include <boost/filesystem.hpp>

#include <chrono>
#include <memory>
#include <string>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

namespace bf = boost::filesystem;

/******************************************************************************
 * Types
 */

enum class req {
    sync,
    send,
    receive,
    del,
    close
};

typedef unsigned char byte_t;

struct file_info {
    char name[MAXNAME * 2];
    char last_modified[MAXNAME];
    int size;

    file_info(std::string name_p, std::string sync_dir)
        : name{ '\0' }
        , last_modified{ '\0' }
        , size(0)
    {
        time_t last_time;

        if (bf::exists(sync_dir + name_p)) {
            last_time = bf::last_write_time(sync_dir + name_p);
            size = bf::file_size(sync_dir + name_p);
            std::strcpy(last_modified, asctime(gmtime(&last_time)));
        }

        std::strcpy(name, name_p.c_str());
    }

    file_info()
        : name{ '\0' }
        , last_modified{ '\0' }
        , size(0)
    {
    }

    bool operator<(file_info const& a) const
    {
        return name < a.name;
    }
};

typedef struct handshake {
    req req_type;
    char userid[MAXNAME];

    handshake(req request, char const* id)
        : req_type(request)
        , userid{ '\0' }
    {
        std::string aux(id);

        // Trivial
        // Proof is left as an exercise to the reader
        std::strcpy(userid, aux.substr(0, aux.find_first_of('\0')).c_str());
    }

    handshake() {}
} handshake_t;

typedef struct ack {
    bool confirmation;

    ack(bool conf)
        : confirmation(conf)
    {
    }

    ack() {}
} ack_t;

typedef struct syn {
    bool confirmation;
    in_port_t port;

    syn(bool conf, in_port_t port_p)
        : confirmation(conf)
        , port(port_p)
    {
    }

    syn() {}
} syn_t;

typedef struct file_data {
    struct file_info file;
    unsigned int num_packets;

    file_data(file_info file_p, unsigned int packets)
        : file(file_p)
        , num_packets(packets)
    {
    }

    file_data() {}
} file_data_t;

typedef struct file_info_list {
    file_data_t file_list[MAX_FILE_LIST_SIZE];
    bool has_next;
} file_info_list_t;

typedef struct packet {
    unsigned int num;
    byte_t data[PACKETSIZE];

    packet(unsigned int num_p)
        : num(num_p)
    {
    }

    packet() {}
} packet_t;

typedef struct {
    byte_t* pointer;
    size_t size;
} convert_helper_t;

/******************************************************************************
 * Headers
 */

int init_unix_socket(struct sockaddr_in& sock, in_port_t port) throw();
int init_unix_socket(struct sockaddr_in& sock, in_port_t port, hostent* server) throw();

std::unique_ptr<handshake_t> convert_to_handshake(byte_t* data);
std::unique_ptr<ack_t> convert_to_ack(byte_t* data);
std::unique_ptr<syn_t> convert_to_syn(byte_t* data);
std::unique_ptr<packet_t> convert_to_packet(byte_t* data);
std::unique_ptr<file_info_list_t> convert_to_file_list(byte_t* data);
std::unique_ptr<file_data_t> convert_to_file_data(byte_t* data);

/******************************************************************************
 * Exceptions
 */

class socket_bad_bind : public std::exception {
public:
    const char* what() const throw()
    {
        return "Bad socket bind\n";
    }
};

class socket_bad_opt : public std::exception {
public:
    const char* what() const throw()
    {
        return "Bad setsockopt\n";
    }
};

class socket_bad_create : public std::exception {
public:
    const char* what() const throw()
    {
        return "Bad socket creation\n";
    }
};

#endif // BROPBOXUTIL_HPP