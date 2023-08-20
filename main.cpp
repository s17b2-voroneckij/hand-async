#include <iostream>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <fstream>
#include "async/EventLoop.hpp"
#include <memory>
#include <csignal>
#include <thread>

using std::cerr;

const int READ_SIZE = 100;

int init_listening(int PORT) {
    int socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd < 0) {
        fprintf(stderr, "socket error: %s\n", strerror(errno));
        exit(0);
    }
    int ret = fcntl(socket_fd, F_SETFL, O_NONBLOCK);
    if (ret == -1) {
        fprintf(stderr, "fcntl error: %s\n", strerror(errno));
        exit(0);
    }
    sockaddr_in sin{};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);
    sin.sin_addr = in_addr{0};
    if (bind(socket_fd, reinterpret_cast<const sockaddr *>(&sin), sizeof(sin)) < 0) {
        fprintf(stderr, "bind error: %s\n", strerror(errno));
        exit(0);
    }
    if (listen(socket_fd, 10) < 0) {
        fprintf(stderr, "listen error: %s\n", strerror(errno));
        exit(0);
    }
    fprintf(stderr, "listening on port %d\n", PORT);
    return socket_fd;
}

unsigned long fibonacci(long n) {
    if (n <= 2) {
        return 1;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

class EchoClient: public IClient {
public:
    EchoClient() = default;

protected:
    void on_accept() override {
        //cerr << "on_accept called for client " << "\n";
        initiate_read(READ_SIZE);
    }

    void on_write(ssize_t writen_size) override {
        // cerr << "on initiate_write called for fd: " << "\n";
        initiate_read(READ_SIZE);
    }

    void on_read(ssize_t read_ret, string s) override {
        // cerr << "on_read called with " << " s: " << s << "\n";
        if (read_ret == 0) {
            //fprintf(stderr, "client finished, leaving\n");
            return;
        }
        long n = strtol(s.c_str(), nullptr, 10);
        initiate_write("response: " + std::to_string(fibonacci(n)) + "\n");
    }
};

int main(int argc, char** argv) {
    signal(SIGPIPE, SIG_IGN);
//    std::thread other_worker([] () {
//        int socket_fd = init_listening(8002);
//        EventLoop eventLoop;
//        // register client takes socket fd for listening and client factory
//        eventLoop.register_client(socket_fd, [] () {
//            return std::shared_ptr<IClient>(new EchoClient());
//        });
//        eventLoop.run_forever();
//    });
//    other_worker.detach();
    int socket_fd = init_listening(strtol(argv[1], nullptr, 10));
    EventLoop eventLoop;
    // register client takes socket fd for listening and client factory
    eventLoop.register_client(socket_fd, [] () {
        return std::shared_ptr<IClient>(new EchoClient());
    });
    eventLoop.run_forever();
}
