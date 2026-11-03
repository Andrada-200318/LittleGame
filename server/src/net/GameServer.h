#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class Room;
class TCPSocket;

class GameServer {
public:
    explicit GameServer(Room& room);
    ~GameServer();

    bool start(uint16_t port, std::string& error);

    void stop();

private:
    void acceptLoop();
    void clientThread(int clientId, std::shared_ptr<TCPSocket> socket);
    void serveClient(int clientId, const std::shared_ptr<TCPSocket>& socket);
    void forgetSocket(const std::shared_ptr<TCPSocket>& socket);

    Room& room_;

    std::atomic<bool> running_{false};

    std::shared_ptr<TCPSocket> listenSocket_;
    std::thread acceptThread_;

    int nextClientId_ = 1;

    std::mutex mutex_;
    std::vector<std::thread> clientThreads_;
    std::vector<std::shared_ptr<TCPSocket>> openSockets_;
};
