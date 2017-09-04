#include <iostream>
#include <boost/program_options.hpp>
#include <string>
#include <cassert>
#include "client.h"

std::string IP = "127.0.0.1";
std::string PORT = "9101";

std::string KEY = "";
bool INCREMENT = false;
bool SET = false;
int64_t DATA = 0;

namespace po = boost::program_options;

int main(int argc, char *argv[]){

    po::options_description desc("Options");
    desc.add_options()
        ("help", "Produce help message")
        ("ip" , po::value<std::string>(&IP), "IP Address to connect to")
        ("port" , po::value<std::string>(&PORT), "Port to connect to")
        ("increment" , po::bool_switch(&INCREMENT), "Increment a value")
        ("set" , po::bool_switch(&SET), "Set a value")
        ("key" , po::value<std::string>(&KEY), "Key to update")
        ("data" , po::value<int64_t>(&DATA), "Data")
        ;

	po::variables_map vm;
    const auto command_line_parameters = po::parse_command_line(argc, argv, desc);
    po::store(command_line_parameters, vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }

    if (vm.count("data") != 1) {
        std::cout << desc << std::endl;
        return -1;
    }

    assert(KEY != "");
    assert(INCREMENT != SET);

    aggregate::Client c(IP, PORT);

    if(INCREMENT){
        c.inc(KEY, DATA);
    }
    else if(SET){
        c.set(KEY, DATA);
    }
    else{
        assert(false);
    }

	return 0;
}
