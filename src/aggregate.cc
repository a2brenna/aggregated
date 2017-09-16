#include <iostream>
#include <boost/program_options.hpp>
#include <string>
#include <cassert>
#include "client.h"

std::string IP = "127.0.0.1";
std::string PORT = "9101";

std::string KEY = "";
double SET;
uint64_t INC;

namespace po = boost::program_options;

int main(int argc, char *argv[]){

    po::options_description desc("Options");
    desc.add_options()
        ("help", "Produce help message")
        ("ip" , po::value<std::string>(&IP), "IP Address to connect to")
        ("port" , po::value<std::string>(&PORT), "Port to connect to")
        ("key" , po::value<std::string>(&KEY), "Key to update")
        ("set" , po::value<double>(&SET), "Set a value")
        ("inc" , po::value<uint64_t>(&INC), "Increment a value")
        ;

	po::variables_map vm;
    const auto command_line_parameters = po::parse_command_line(argc, argv, desc);
    po::store(command_line_parameters, vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }

    if (vm.count("set") && vm.count("inc")){
        std::cout << desc << std::endl;
        return 1;
    }

    assert(KEY != "");

    aggregate::Client c(IP, PORT);

    if(vm.count("inc")){
        c.inc(KEY, INC);
    }
    else if(vm.count("set")){
        c.set(KEY, SET);
    }
    else{
        assert(false);
    }

	return 0;
}
