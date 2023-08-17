#include <iostream>
#include <vector>
#include <cstring>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <fstream>
#include <unistd.h>
#include "async/EventLoop.hpp"

using std::cerr;

const int PORT = 8093;
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

int main() {
    int socket_fd = init_listening();
    EventLoop eventLoop;
    on_accept_callback_type on_accept;
    on_write_callback_type on_write;
    on_read_callback_type on_read;
    on_accept = [&] (int client_fd) {
        cerr << "on_accept called for client " << client_fd << "\n";
        eventLoop.register_on_read_callback(client_fd, READ_SIZE, on_read);
    };
    on_read = [&] (int client_fd, int read, const string& s) {
        cerr << "on_read called with fd " << client_fd << " s: " << s << "\n";
        if (read < 0) {
            fprintf(stderr, "register_on_read_callback error %s\n", strerror(errno));
            eventLoop.close_fd(client_fd);
            return ;
        }
        if (read == 0) {
            fprintf(stderr, "client finished, leaving\n");
            eventLoop.close_fd(client_fd);
            return;
        }
        eventLoop.register_on_write_callback(client_fd, s.size(), s, on_write);
    };
    on_write = [&] (int client_fd, int wrote) {
        cerr << "on write called for fd: " << client_fd << "\n";
        if (wrote < 0) {
            fprintf(stderr, "register_on_write_callback error %s\n", strerror(errno));
            eventLoop.close_fd(client_fd);
            return ;
        }
        if (wrote == 0) {
            fprintf(stderr, "wrote 0, weird\n");
        }
        eventLoop.register_on_read_callback(client_fd, READ_SIZE, on_read);
    };
    eventLoop.register_on_accept_callback(socket_fd, on_accept);
    eventLoop.run_forever();
}
