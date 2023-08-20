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

using std::cerr;

const int PORT = 8002;
const int READ_SIZE = 100;

int init_listening() {
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

class EchoClient: public IClient {
public:
    EchoClient() = default;

protected:
    void on_accept() override {
        cerr << "on_accept called for client " << "\n";
        initiate_read(READ_SIZE);
    }

    void on_write(ssize_t writen_size) override {
        // cerr << "on initiate_write called for fd: " << "\n";
        initiate_read(READ_SIZE);
    }

    void on_read(ssize_t read_ret, string s) override {
        // cerr << "on_read called with " << " s: " << s << "\n";
        if (read_ret == 0) {
            fprintf(stderr, "client finished, leaving\n");
            return;
        }
        initiate_write(s);
    }
};

int main() {
    signal(SIGPIPE, SIG_IGN);
    int socket_fd = init_listening();
    EventLoop eventLoop;
    // register client takes socket fd for listening and client factory
    eventLoop.register_client(socket_fd, [] () {
        return std::shared_ptr<IClient>(new EchoClient());
    });
    eventLoop.run_forever();
}
