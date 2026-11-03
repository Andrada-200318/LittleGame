#include "TCPSocket.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include <chrono>

namespace {

constexpr std::size_t kMaxLineBytes = 2048;

constexpr int kDrainTimeoutMs = 1000;

const std::uintptr_t kNoHandle = static_cast<std::uintptr_t>(INVALID_SOCKET);

SOCKET toSocket(std::uintptr_t handle) {
    return static_cast<SOCKET>(handle);
}

std::string lastErrorText(const std::string& what) {
    return what + " failed (error " + std::to_string(WSAGetLastError()) + ")";  // WSAGetLastError: code of the last failed call
}

std::string toUtf8(const wchar_t* wide) {
    const int size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return "";
    }
    std::string text(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, text.data(), size, nullptr, nullptr);
    text.pop_back();
    return text;
}

}

bool TCPSocket::startup() {
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
}

void TCPSocket::cleanup() {
    WSACleanup();
}

std::vector<std::pair<std::string, std::string>> TCPSocket::localIPv4Addresses() {
    std::vector<std::pair<std::string, std::string>> addresses;

    const ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG size = 16 * 1024;
    std::vector<unsigned char> buffer;
    ULONG result;
    do {
        buffer.resize(size);
        result = GetAdaptersAddresses(AF_INET, flags, nullptr,
                                      reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()), &size);
    } while (result == ERROR_BUFFER_OVERFLOW);

    if (result != NO_ERROR) {
        return addresses;
    }

    auto* adapter = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
    for (; adapter != nullptr; adapter = adapter->Next) {
        if (adapter->OperStatus != IfOperStatusUp || adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) {
            continue;
        }
        const std::string name = toUtf8(adapter->FriendlyName);
        for (auto* unicast = adapter->FirstUnicastAddress; unicast != nullptr; unicast = unicast->Next) {
            auto* ipv4 = reinterpret_cast<sockaddr_in*>(unicast->Address.lpSockaddr);
            char text[INET_ADDRSTRLEN] = {};
            // inet_ntop: turns the binary address into text like "192.168.43.12".
            if (inet_ntop(AF_INET, &ipv4->sin_addr, text, sizeof(text)) != nullptr) {
                addresses.emplace_back(name, text);
            }
        }
    }
    return addresses;
}

std::shared_ptr<TCPSocket> TCPSocket::listenOn(uint16_t port, std::string& error) {
    // socket: creates a TCP socket over IPv4.
    SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        error = lastErrorText("socket");
        return nullptr;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);  // htonl: converts to network byte order. INADDR_ANY = all interfaces.
    address.sin_port = htons(port);               // htons: same, for the 16-bit port.

    if (::bind(listenSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAEADDRINUSE) {
            error = "Port " + std::to_string(port) + " is already in use (error 10048)";
        } else {
            error = lastErrorText("bind");
        }
        ::closesocket(listenSocket);  // closesocket: releases the socket.
        return nullptr;
    }

    if (::listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        error = lastErrorText("listen");
        ::closesocket(listenSocket);  // closesocket: releases the socket.
        return nullptr;
    }

    return std::make_shared<TCPSocket>(static_cast<std::uintptr_t>(listenSocket));
}

TCPSocket::TCPSocket(std::uintptr_t handle) : handle_(handle) {
}

TCPSocket::~TCPSocket() {
    close();
}

std::shared_ptr<TCPSocket> TCPSocket::accept() {
    while (true) {
        // accept: blocks until a client connects, then returns a new socket for that client.
        SOCKET client = ::accept(toSocket(handle_.load()), nullptr, nullptr);
        if (client != INVALID_SOCKET) {
            return std::make_shared<TCPSocket>(static_cast<std::uintptr_t>(client));
        }
        if (WSAGetLastError() != WSAECONNRESET) {
            return nullptr;
        }
    }
}

bool TCPSocket::sendLine(const std::string& line) {
    const std::string data = line + '\n';


    std::lock_guard<std::mutex> lock(sendMutex_);

    std::size_t sentBytes = 0;
    while (sentBytes < data.size()) {
        const int sent = ::send(toSocket(handle_.load()), data.data() + sentBytes,
                                static_cast<int>(data.size() - sentBytes), 0);
        if (sent == SOCKET_ERROR) {
            shutdown();
            return false;
        }
        sentBytes += static_cast<std::size_t>(sent);
    }
    return true;
}

TCPSocket::ReadStatus TCPSocket::readLine(std::string& line) {
    while (true) {
        const std::size_t lineEnd = readBuffer_.find('\n');
        if (lineEnd != std::string::npos) {
            line = readBuffer_.substr(0, lineEnd);
            readBuffer_.erase(0, lineEnd + 1);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            return line.size() > kMaxLineBytes ? ReadStatus::TooLong : ReadStatus::Ok;
        }

        if (readBuffer_.size() > kMaxLineBytes + 1) {
            return ReadStatus::TooLong;
        }

        char chunk[1024];
        const int received = ::recv(toSocket(handle_.load()), chunk, sizeof(chunk), 0);
        if (received <= 0) {
            return ReadStatus::Closed;
        }
        readBuffer_.append(chunk, static_cast<std::size_t>(received));
    }
}

void TCPSocket::setRecvTimeout(int ms) {
    const DWORD timeout = static_cast<DWORD>(ms);
    ::setsockopt(toSocket(handle_.load()), SOL_SOCKET, SO_RCVTIMEO,
                 reinterpret_cast<const char*>(&timeout), sizeof(timeout));
}

void TCPSocket::setSendTimeout(int ms) {
    const DWORD timeout = static_cast<DWORD>(ms);
    ::setsockopt(toSocket(handle_.load()), SOL_SOCKET, SO_SNDTIMEO,
                 reinterpret_cast<const char*>(&timeout), sizeof(timeout));
}

void TCPSocket::shutdown() {
    const std::uintptr_t handle = handle_.load();
    if (handle != kNoHandle) {
        ::shutdown(toSocket(handle), SD_BOTH);

        ::CancelIoEx(reinterpret_cast<HANDLE>(handle), nullptr);
    }
}

void TCPSocket::drainBeforeClose() {
    ::shutdown(toSocket(handle_.load()), SD_SEND);

    setRecvTimeout(kDrainTimeoutMs);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(kDrainTimeoutMs);

    char chunk[1024];
    while (std::chrono::steady_clock::now() < deadline) {
        if (::recv(toSocket(handle_.load()), chunk, sizeof(chunk), 0) <= 0) {
            return;
        }
    }
}

void TCPSocket::close() {
    const std::uintptr_t handle = handle_.exchange(kNoHandle);
    if (handle != kNoHandle) {
        ::closesocket(toSocket(handle));
    }
}

std::string TCPSocket::peerAddress() const {
    sockaddr_in address{};
    int size = sizeof(address);
    if (::getpeername(toSocket(handle_.load()), reinterpret_cast<sockaddr*>(&address), &size) == SOCKET_ERROR) {
        return "unknown";
    }
    char text[INET_ADDRSTRLEN] = {};
    if (inet_ntop(AF_INET, &address.sin_addr, text, sizeof(text)) == nullptr) {
        return "unknown";
    }
    return std::string(text) + ":" + std::to_string(ntohs(address.sin_port));
}
