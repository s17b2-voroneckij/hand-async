#pragma once

#include <cstdio>
#include <string>
#include <memory>

#include "EventLoop.hpp"

using std::string;

class EventLoop;

class IClient : public std::enable_shared_from_this<IClient> {
public:
    friend class EventLoop;

    virtual ~IClient();

protected:
    virtual void on_read(ssize_t read_ret, string s) = 0;

    virtual void on_accept() = 0;

    virtual void on_write(ssize_t writen_size) = 0;

    void initiate_read(ssize_t read_size_);

    void initiate_write(const string& s);

private:
    EventLoop* loop;
    int fd = -1;
    string write_string;
    ssize_t read_size = -1;
};
