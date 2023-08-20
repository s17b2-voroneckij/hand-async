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
    close_fd(epoll_fd);
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
                // let`s register_on_write_callback
                auto client = write_waiters.at(fd);
                ssize_t write_ret = ::write(fd, client->write_string.c_str(), client->write_string.size());
                if (write_ret < 0) {
                    //printf("register_on_write_callback error: %s\n", strerror(errno));
                }
                if (write_ret == client->write_string.size() || write_ret < 0) {
                    // erase fd from events
                    write_waiters.erase(fd);
                    // epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                    client->on_write(write_ret);
                } else {
                    // this isn`t really effective
                    client->write_string = client->write_string.substr(write_ret);
                }
            } else if (event.events & EPOLLIN && client_factories.contains(fd)) {
                int client_fd = accept4(fd, nullptr, nullptr, SOCK_NONBLOCK);
                if (client_fd < 0) {
                    printf("accept4 error: %s\n", strerror(errno));
                    exit(0);
                }
                // do not erase fd from those watched
                auto client = client_factories.at(fd)();
                // accept_waiters.erase(fd);
                // epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                client->fd = client_fd;
                client->loop = this;
                client->on_accept();
            } else if (event.events & EPOLLIN && read_waiters.contains(fd)) {
                // let`s register_on_read_callback
                auto client = read_waiters.at(fd);
                size_t size = client->read_size;
                char buf[size];
                ssize_t read_ret = ::read(fd, buf, size);
                if (read_ret < 0) {
                    //printf("initiate_read error: %s\n", strerror(errno));
                }
                read_waiters.erase(fd);
                // epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                if (read_ret >= 0) {
                    client->on_read(read_ret, string(buf, buf + read_ret));
                } else {
                    client->on_read(read_ret, string());
                }
            }
        }
    }
}

void EventLoop::register_on_write_callback(const shared_ptr<IClient>& client) {
    auto fd = client->fd;
    if (!watched_fds.contains(fd)) {
        epoll_event event{};
        event.data.fd = fd;
        event.events = EPOLLOUT | EPOLLIN;
        int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
        if (ret < 0) {
            printf("epoll_ctl error: %s\n", strerror(errno));
        }
        watched_fds.insert(fd);
    }
    write_waiters[fd] = client;
}

void EventLoop::register_on_read_callback(const shared_ptr<IClient>& client) {
    int fd = client->fd;
    if (!watched_fds.contains(fd)) {
        epoll_event event{};
        event.data.fd = fd;
        event.events = EPOLLIN | EPOLLOUT;
        int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
        if (ret < 0) {
            printf("epoll_ctl error: %s\n", strerror(errno));
        }
        watched_fds.insert(fd);
    }
    read_waiters[fd] = client;
}

void EventLoop::close_fd(int fd) {
    //fprintf(stderr, "close_fd\n");
    if (watched_fds.contains(fd)) {
        int ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
        if (ret < 0) {
            printf("epoll_ctl error: %s\n", strerror(errno));
        }
        watched_fds.erase(fd);
    }
    ::close(fd);
}

void EventLoop::register_client(ssize_t sock_fd, const function<shared_ptr<IClient>()> &client_factory) {
    epoll_event event{};
    event.data.fd = sock_fd;
    event.events = EPOLLIN;
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &event);
    if (ret < 0) {
        printf("epoll_ctl error: %s\n", strerror(errno));
    }
    client_factories[sock_fd] = client_factory;
}
