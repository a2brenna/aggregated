#include "client.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <cassert>
#include <string.h>

#include "update.h"

namespace aggregate {

    Client::Client(const std::string &server_address, const std::string &server_port){
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints) );
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        struct addrinfo *r = nullptr;
        const auto addrinfo_status = getaddrinfo(server_address.c_str(), server_port.c_str(), &hints, &r);
		assert(addrinfo_status == 0);

        _server_fd = [](const struct addrinfo *r){
			const int fd = socket(AF_INET, SOCK_DGRAM, 0);
			assert(fd >= 0);

			for(auto s = r; s != nullptr; s = s->ai_next){

				const int c = ::connect(fd , s->ai_addr, s->ai_addrlen);

				if (c == 0) {
					return fd;
				}
			}

			return -1;
		}(r);
		assert(_server_fd >= 0);

    }

    void Client::_send(const std::string &key, const uint8_t &type, const Number &data){
        assert(key.size() < 256);
        assert( (type == TYPE_INC) || (type == TYPE_SET) );

        struct Update u;
        u.type = type;
        u.data = data;

        ::strncpy(u.key, key.c_str(), 256);

        const int send_status = send(_server_fd, &u, sizeof(u), MSG_NOSIGNAL);
        //assert(send_status == sizeof(u));

        return;
    }

    void Client::set(const std::string &key, const double &data){
        Number _data;
        _data.d = data;
        _send(key, TYPE_SET, _data);
        return;
    }

    void Client::inc(const std::string &key, const int64_t &data){
        Number _data;
        _data.i = data;
        _send(key, TYPE_INC, _data);
        return;
    }

}
