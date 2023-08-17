#include "IClient.hpp"
#include <iostream>

using std::cerr;

IClient::~IClient() {
    cerr << "~IClient\n";
    loop->close_fd(fd);
}

void IClient::initiate_write(const string &s) {
    write_string = s;
    loop->register_on_write_callback(shared_from_this());
}

void IClient::initiate_read(ssize_t read_size_) {
    read_size = read_size_;
    loop->register_on_read_callback(shared_from_this());
}