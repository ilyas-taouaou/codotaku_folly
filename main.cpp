#include <iostream>
#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/AsyncServerSocket.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/ScopedEventBaseThread.h>

using namespace folly;

class Client final : public AsyncSocket::ReadCallback {
public:
    explicit Client(AsyncSocket::UniquePtr socket): socket_{std::move(socket)} {
        socket_->setReadCB(this);
    }

    void getReadBuffer(void **bufReturn, size_t *lenReturn) override {
        *bufReturn = buffer;
        *lenReturn = sizeof(buffer);
    }

    void readDataAvailable(const size_t len) noexcept override {
        std::cout << "Received data: " << std::string(buffer, len) << std::endl;
        socket_->write(nullptr, buffer, len);
    }

    void readEOF() noexcept override {
        std::cout << "Connection closed by server." << std::endl;
        socket_->close();
    }

    void readErr(const AsyncSocketException &ex) noexcept override {
        std::cerr << "Read error: " << ex.what() << std::endl;
        socket_->close();
    }

private:
    AsyncSocket::UniquePtr socket_;
    char buffer[4096]{};
};

class EventBasePool {
public:
    explicit EventBasePool(const size_t numThreads) {
        threads_.reserve(numThreads);
        for (size_t i = 0; i < numThreads; ++i)
            threads_.emplace_back(std::make_unique<ScopedEventBaseThread>());
    }

    EventBase *getNextEventBase() {
        return threads_[nextIndex_++ % threads_.size()]->getEventBase();
    }

private:
    std::vector<std::unique_ptr<ScopedEventBaseThread> > threads_;
    size_t nextIndex_{};
};

class ServerAcceptCallback final : public AsyncServerSocket::AcceptCallback {
public:
    explicit ServerAcceptCallback(EventBasePool &eventBasePool) : eventBasePool_(eventBasePool) {
    }

    void connectionAccepted(const NetworkSocket fd, const SocketAddress &clientAddr,
                            AcceptInfo info) noexcept override {
        std::cout << "Accepted connection from " << clientAddr.describe() << std::endl;
        auto socket = AsyncSocket::newSocket(nullptr, fd);
        const auto eventBase = eventBasePool_.getNextEventBase();
        eventBase->runInEventBaseThread([socket = std::move(socket), eventBase]() mutable {
            socket->attachEventBase(eventBase);
            new Client(std::move(socket));
        });
    }

private:
    EventBasePool &eventBasePool_;
};

int main(int argc, char **argv) {
    Init init(&argc, &argv);
    EventBase eventBase;
    auto numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0)
        numThreads = 1;
    EventBasePool eventBasePool(numThreads);
    const auto socket{AsyncServerSocket::newSocket(&eventBase)};
    socket->bind({"127.0.0.1", 12345});
    socket->listen(128);
    socket->addAcceptCallback(new ServerAcceptCallback{eventBasePool}, &eventBase);
    socket->startAccepting();
    std::cout << "Server is listening on" << socket->getAddress().describe() << std::endl;
    eventBase.loopForever();
}
