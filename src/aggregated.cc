#include <iostream>
#include <boost/program_options.hpp>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <signal.h>
#include <sys/signalfd.h>
#include <sys/select.h>

#include <string>
#include <map>

#include "update.h"

std::string CONFIG_UDP_IP = "127.0.0.1";
std::string CONFIG_UDP_PORT = "9101";
std::string CONFIG_HTTP_IP = "127.0.0.1";
std::string CONFIG_HTTP_PORT = "9101";

const size_t MAX_SAFE_UDP = 508;

namespace po = boost::program_options;

int main(int argc, char *argv[]){

    std::map<std::string, int64_t> data;

    // clang-format off
    po::options_description desc("Options");
    desc.add_options()
        ("help", "Produce help message")
        ("udp_ip" , po::value<std::string>(&CONFIG_UDP_IP), "IP Address to listen for incoming statistcs")
        ("udp_port" , po::value<std::string>(&CONFIG_UDP_PORT), "Port to listen for incoming statistics")
        ("http_ip" , po::value<std::string>(&CONFIG_HTTP_IP), "IP Address to listen for incoming Prometheus scrapes")
        ("http_port" , po::value<std::string>(&CONFIG_HTTP_PORT), "Port to listen for incoming Prometheus scrapes")
        ;
    // clang-format on

	po::variables_map vm;
    const auto command_line_parameters = po::parse_command_line(argc, argv, desc);
    po::store(command_line_parameters, vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }

    const int udp_fd = [](const std::string &udp_ip, const std::string &udp_port){

        const int port_num = std::stoi(udp_port, nullptr, 10);
        assert(port_num >= 1024);

        struct addrinfo *r = nullptr;
        const int addrinfo_status = getaddrinfo(udp_ip.c_str(), udp_port.c_str(), nullptr, &r);
        assert(addrinfo_status == 0);

        const int fd = socket(AF_INET, SOCK_DGRAM, 0);
        assert(fd >= 0);

        const bool bound = [](const struct addrinfo *r, const int &fd){
			for(auto s = r; s != nullptr; s = s->ai_next){
					const int b = bind(fd, s->ai_addr, s->ai_addrlen);
				if (b == 0) {
					return true;
				}
				else{
					continue;
				}
			}
			return false;
        }(r, fd);
        assert(bound);

        freeaddrinfo(r);
        return fd;
    }(CONFIG_UDP_IP, CONFIG_UDP_PORT);
    assert(udp_fd >= 0);

    const int http_fd = [](const std::string &http_ip, const std::string &http_port){

        const int port_num = std::stoi(http_port, nullptr, 10);
        assert(port_num >= 1024);

        struct addrinfo *r = nullptr;
        const int addrinfo_status = getaddrinfo(http_ip.c_str(), http_port.c_str(), nullptr, &r);
        assert(addrinfo_status == 0);

        const int fd = socket(AF_INET, SOCK_STREAM, 0);
        assert(fd >= 0);

		const int yes = 1;
		const auto sa = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		const auto sb = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int));
		assert((sa == 0) && (sb == 0));

        const bool bound = [](const struct addrinfo *r, const int &fd){
			for(auto s = r; s != nullptr; s = s->ai_next){
					const int b = bind(fd, s->ai_addr, s->ai_addrlen);
				if (b == 0) {
					return true;
				}
				else{
					continue;
				}
			}
			return false;
        }(r, fd);
        assert(bound);

		const int listening = ::listen(fd, SOMAXCONN);
		assert(listening >= 0);

        freeaddrinfo(r);
        return fd;
    }(CONFIG_UDP_IP, CONFIG_UDP_PORT);
    assert(http_fd >= 0);

    const bool signals_masked = [](){
        sigset_t mask;

        const auto f = sigfillset(&mask);
        if(f != 0){
            return false;
        }

        const auto m = sigprocmask(SIG_SETMASK, &mask, nullptr);
        if(m != 0){
            return false;
        }

        return true;
    }();
    assert(signals_masked);

    const int signal_fd = []() {
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGINT);
        return signalfd(-1, &mask, 0);
    }();
    assert(signal_fd >= 0);

    const auto maxfds = std::max(std::max(signal_fd, http_fd), udp_fd) + 1;

    const fd_set fds = [](const int &http_fd, const int &signal_fd, const int &udp_fd) {
        fd_set fds;
        FD_ZERO(&fds); //might be unnecessary
        FD_SET(http_fd, &fds);
        FD_SET(signal_fd, &fds);
        FD_SET(udp_fd, &fds);
        return fds;
    }(http_fd, signal_fd, udp_fd);

    while(true){
        const auto fd_to_handle = [](const int &http_fd, const int &udp_fd, const int &signal_fd, const int &maxfds, fd_set fds) {
            const auto s = select(maxfds, &fds, nullptr, nullptr, nullptr);
            assert(s != -1);

            if (FD_ISSET(udp_fd, &fds)) {
                return udp_fd;
            }
            else if (FD_ISSET(http_fd, &fds)) {
                return http_fd;
            }
            else if (FD_ISSET(signal_fd, &fds)) {
                return signal_fd;
            }
            else {
                return -1;
            }
        }(http_fd, udp_fd, signal_fd, maxfds, fds);

        if(fd_to_handle == http_fd){
            const int client_fd = accept(http_fd, nullptr, NULL);
            char buff[4096];
            int request_size = read(client_fd, buff, 4096);
            assert(request_size > 0);
            assert(request_size < 4096);

            const std::string request(buff, request_size);

            const std::string response = [](const std::map<std::string, int64_t> &data){
                std::string response = "# start\n";
                for(const auto &d: data){
                    std::string comment_string = d.first;
                    const auto brace_pos = comment_string.find("{");
                    comment_string.erase(brace_pos, std::string::npos);
                    response.append("# TYPE " + comment_string + " untyped\n");
                    response.append(d.first + " " + std::to_string(d.second) + "\n");
                }
                response.append("# end\n");
                return response;
            }(data);

            std::string response_header = "HTTP/1.1 200 OK\r\n";
            response_header.append("Content-Type: text/plain; version=0.0.4\r\n");
            response_header.append("Content-Length: " + std::to_string(response.size()) + "\r\n");
            response_header.append("\r\n");

            int w1 = write(client_fd, response_header.c_str(), response_header.size());
            assert(w1 == response_header.size());

            int w2 = write(client_fd, response.c_str(), response.size());
            assert(w2 == response.size());

            close(client_fd);
        }
        else if(fd_to_handle == udp_fd){

            Update update;

            const int incoming_bytes = read(fd_to_handle, &update, sizeof(Update));

            if(incoming_bytes == sizeof(Update)){

                //TODO: check for spaces?

                update.key[255] = '\0';
                const std::string name(update.key);

                if(update.type == TYPE_SET){
                    std::cerr << "Setting: " << name << " " << update.data << std::endl;
                    data[name] = update.data;
                }
                else if(update.type == TYPE_INC){
                    std::cerr << "Currently: " << name << " " << data[name] << std::endl;
                    std::cerr << "Incrementing: " << name << " " << update.data << std::endl;
                    data[name] += update.data;
                }
                else{
                    assert(false);
                }
            }
            else{
                continue;
            }

        }
        else if(fd_to_handle == signal_fd){
            assert(false);
        }
        else{
            assert(false);
        }

        for(const auto &d: data){
            std::cout << d.first << " " << d.second << std::endl;
        }
    }

    return 0;
}
