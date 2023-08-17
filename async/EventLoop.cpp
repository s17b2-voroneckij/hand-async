#include "EventLoop.hpp"
#include <sys/epoll.h>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>


EventLoop::EventLoop() {
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        printf("epoll_create1 error: %s\n", strerror(errno));
    }
}

EventLoop::~EventLoop() {
    close(epoll_fd);
}

void EventLoop::register_on_accept(int sock_fd, const on_accept_callback_type &on_accept_callback) {
    epoll_event event{};
    event.data.fd = sock_fd;
    event.events = EPOLLIN;
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &event);
    if (ret < 0) {
        printf("epoll_ctl error: %s\n", strerror(errno));
    }
    accept_waiters[sock_fd] = on_accept_callback;
}

const int EVENT_NUMBER = 100;

[[noreturn]] void EventLoop::run_forever() {
    while (true) {
        epoll_event events[EVENT_NUMBER];
        int ret = epoll_wait(epoll_fd, events, EVENT_NUMBER, -1);
        if (ret < 0) {
            printf("epoll_wait error: %s\n", strerror(errno));
            exit(1);
        }
        for (int i = 0; i < ret; i++) {
            auto event = events[i];
            int fd = event.data.fd;
            if (event.events & EPOLLOUT && write_waiters.contains(fd)) {
                // let`s write
                // for now, assuming that we will always write everything
                ssize_t write_ret = ::write(fd, write_data.at(fd).c_str(), write_data.at(fd).size());
                if (write_ret < 0) {
                    printf("write error: %s\n", strerror(errno));
                }
                auto func = write_waiters.at(fd);
                // erase fd from events
                write_waiters.erase(fd);
                write_data.erase(fd);
                // epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                func(fd, write_ret);
            } else if (event.events & EPOLLIN && accept_waiters.contains(fd)) {
                int client_fd = accept4(fd, nullptr, nullptr, SOCK_NONBLOCK);
                if (client_fd < 0) {
                    printf("accept4 error: %s\n", strerror(errno));
                }
                // do not erase fd from those watched
                auto func = accept_waiters.at(fd);
                // accept_waiters.erase(fd);
                // epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                func(client_fd);
            } else if (event.events & EPOLLIN && read_waiters.contains(fd)) {
                // let`s read
                size_t size = read_sizes.at(fd);
                char buf[size];
                ssize_t read_ret = ::read(fd, buf, size);
                auto func = read_waiters.at(fd);
                read_waiters.erase(fd);
                // epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                func(fd, read_ret, string(buf, buf + read_ret));
            }
        }
    }
}

void EventLoop::write(int fd, size_t length, const string &s, const on_write_callback_type &on_write_callback) {
    epoll_event event{};
    event.data.fd = fd;
    event.events = EPOLLOUT;
    if (!watched_fds.contains(fd)) {
        int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
        if (ret < 0) {
            printf("epoll_ctl error: %s\n", strerror(errno));
        }
        watched_fds.insert(fd);
    } else {
        int ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
        if (ret < 0) {
            printf("epoll_ctl error: %s\n", strerror(errno));
        }
    }
    write_data[fd] = s;
    write_waiters[fd] = on_write_callback;
}

void EventLoop::read(int fd, size_t length, const on_read_callback_type &on_read_callback) {
    epoll_event event{};
    event.data.fd = fd;
    event.events = EPOLLIN;
    if (!watched_fds.contains(fd)) {
        int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
        if (ret < 0) {
            printf("epoll_ctl error: %s\n", strerror(errno));
        }
        watched_fds.insert(fd);
    } else {
        int ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
        if (ret < 0) {
            printf("epoll_ctl error: %s\n", strerror(errno));
        }
    }
    read_sizes[fd] = length;
    read_waiters[fd] = on_read_callback;
}

void EventLoop::close(int fd) {
    if (watched_fds.contains(fd)) {
        int ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
        if (ret < 0) {
            printf("epoll_ctl error: %s\n", strerror(errno));
        }
        watched_fds.erase(fd);
    }
    ::close(fd);
}
