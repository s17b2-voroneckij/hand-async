#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>

using std::function;
using std::string;
using std::unordered_map;
using std::unordered_set;

typedef function<void(int)> on_accept_callback_type;

typedef function<void(int, ssize_t)> on_write_callback_type;

typedef function<void(int, ssize_t, string)> on_read_callback_type;

class EventLoop {
public:
    EventLoop();

    ~EventLoop();

    void register_on_accept_callback(int sock_fd, const on_accept_callback_type& on_accept_callback);

    [[noreturn]] void run_forever();

    void register_on_write_callback(int fd, size_t length, const string& s, const on_write_callback_type& on_write_callback);

    void register_on_read_callback(int fd, size_t length, const on_read_callback_type& on_read_callback);

    void close_fd(int fd);

private:
    int epoll_fd;

    unordered_map<int, on_read_callback_type> read_waiters;
    unordered_map<int, size_t> read_sizes;
    unordered_map<int, on_write_callback_type> write_waiters;
    unordered_map<int, string> write_data;
    unordered_map<int, on_accept_callback_type> accept_waiters;
    unordered_set<int> watched_fds;
};
