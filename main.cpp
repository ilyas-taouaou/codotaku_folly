#include <iostream>
#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/AsyncServerSocket.h>
#include <folly/io/async/AsyncSocket.h>

using namespace folly;

class ServerAcceptCallback final : public AsyncServerSocket::AcceptCallback {
public:
    explicit ServerAcceptCallback(EventBase &eventBase) : eventBase_(eventBase) {
    }

    void connectionAccepted(const NetworkSocket fd, const SocketAddress &clientAddr,
                            AcceptInfo info) noexcept override {
        std::cout << "Accepted connection from " << clientAddr.describe() << std::endl;
        auto socket = AsyncSocket::newSocket(&eventBase_, fd);
    }

private:
    EventBase &eventBase_;
};

int main(int argc, char **argv) {
    Init init(&argc, &argv);
    EventBase eventBase;
    const auto socket{AsyncServerSocket::newSocket(&eventBase)};
    socket->bind({"127.0.0.1", 12345});
    socket->listen(128);
    socket->addAcceptCallback(new ServerAcceptCallback{eventBase}, &eventBase);
    socket->startAccepting();
    std::cout << "Server is listening on" << socket->getAddress().describe() << std::endl;
    eventBase.loopForever();
}
