#ifndef __AGGREGATE_CLIENT_H__
#define __AGGREGATE_CLIENT_H__

#include <string>
#include "update.h"

namespace aggregate {

    class Aggregator{

        public:

            virtual void set(const std::string &key, const double &data) = 0;
            virtual void inc(const std::string &key, const int64_t &data) = 0;

    };

    class Client : public Aggregator {

        public:

            Client(const std::string &server_address, const std::string &server_port);

            void set(const std::string &key, const double &data);
            void inc(const std::string &key, const int64_t &data);

        private:

            int _server_fd;

            void _send(const std::string &key, const uint8_t &type, const Number &data);

    };

    class Dummy : public Aggregator {

        public:

            void set(const std::string &key, const double &data);
            void inc(const std::string &key, const int64_t &data);

    };

}

#endif
