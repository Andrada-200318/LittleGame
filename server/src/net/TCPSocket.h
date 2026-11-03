#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>


class TCPSocket {
public:
    enum class ReadStatus { Ok, Closed, TooLong };

    static bool startup();
    static void cleanup();
    static std::vector<std::pair<std::string, std::string>> localIPv4Addresses();

    static std::shared_ptr<TCPSocket> listenOn(uint16_t port, std::string& error);

    explicit TCPSocket(std::uintptr_t handle);
    ~TCPSocket();

    TCPSocket(const TCPSocket&) = delete;
    TCPSocket& operator=(const TCPSocket&) = delete;

    std::shared_ptr<TCPSocket> accept();

    bool sendLine(const std::string& line);
    ReadStatus readLine(std::string& line);
    void setRecvTimeout(int ms);
    void setSendTimeout(int ms);
    void shutdown();
    void drainBeforeClose();
    void close();
    std::string peerAddress() const;

private:
    std::atomic<std::uintptr_t> handle_;

    std::mutex sendMutex_;

    std::string readBuffer_;
};
