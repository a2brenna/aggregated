#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <string>

namespace aggregate {

    class Client{

        public:

            Client(const std::string &server_address, const std::string &server_port);

            void set(const std::string &key, const int64_t &data);
            void inc(const std::string &key, const int64_t &data);

        private:

            int _server_fd;

            void _send(const std::string &key, const uint8_t &type, const int64_t &data);

    };

};

#endif
