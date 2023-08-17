#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>

#include "IClient.hpp"

using std::function;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::shared_ptr;

class IClient;

class EventLoop {
public:
    EventLoop();

    ~EventLoop();

    void register_client(ssize_t sock_fd, const function<shared_ptr<IClient>()>& client_factory);

    [[noreturn]] void run_forever();

    void register_on_write_callback(const shared_ptr<IClient>& client);

    void register_on_read_callback(const shared_ptr<IClient>& client);

    void close_fd(int fd);

private:
    int epoll_fd;

    unordered_map<ssize_t, shared_ptr<IClient>> read_waiters;
    unordered_map<ssize_t, shared_ptr<IClient>> write_waiters;
    unordered_set<ssize_t> watched_fds;
    unordered_map<ssize_t, function<shared_ptr<IClient>()>> client_factories;
};
