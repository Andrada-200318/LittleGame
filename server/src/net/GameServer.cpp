#include "GameServer.h"

#include <QtDebug>

#include <algorithm>
#include <exception>

#include "../game/Protocol.h"
#include "../game/Room.h"
#include "TCPSocket.h"

GameServer::GameServer(Room& room) : room_(room) {
}

GameServer::~GameServer() {
    stop();
}

bool GameServer::start(uint16_t port, std::string& error) {
    listenSocket_ = TCPSocket::listenOn(port, error);
    if (!listenSocket_) {
        return false;
    }

    running_ = true;
    acceptThread_ = std::thread(&GameServer::acceptLoop, this);
    qDebug("Listening on port %d", port);
    return true;
}

void GameServer::stop() {
    if (!running_.exchange(false)) {
        return;
    }


    listenSocket_->close();
    acceptThread_.join();

    std::vector<std::thread> threads;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (const std::shared_ptr<TCPSocket>& socket : openSockets_) {
            socket->shutdown();
        }
        threads.swap(clientThreads_);
    }


    for (std::thread& thread : threads) {
        thread.join();
    }
}

void GameServer::acceptLoop() {
    try {
        while (running_) {
            std::shared_ptr<TCPSocket> client = listenSocket_->accept();
            if (!client) {
                break;
            }

            const int clientId = nextClientId_++;
            qDebug("[client %d] connected from %s", clientId, client->peerAddress().c_str());

            std::lock_guard<std::mutex> lock(mutex_);
            openSockets_.push_back(client);
            clientThreads_.emplace_back(&GameServer::clientThread, this, clientId, client);
        }
    } catch (const std::exception& e) {
        qDebug("Accept thread stopped: %s", e.what());
    } catch (...) {
        qDebug("Accept thread stopped: unknown error");
    }
}

void GameServer::clientThread(int clientId, std::shared_ptr<TCPSocket> socket) {
    try {
        serveClient(clientId, socket);
    } catch (const std::exception& e) {
        qDebug("[client %d] error: %s", clientId, e.what());
    } catch (...) {
        qDebug("[client %d] unknown error", clientId);
    }

    try {
        room_.removePlayer(clientId);
        socket->drainBeforeClose();
        forgetSocket(socket);
        socket->close();
    } catch (const std::exception& e) {
        qDebug("[client %d] error during cleanup: %s", clientId, e.what());
    } catch (...) {
        qDebug("[client %d] unknown error during cleanup", clientId);
    }
}


static void sendError(int clientId, TCPSocket& socket, const char* code, const char* message) {
    socket.sendLine(Protocol::format(Protocol::kError, {code, message}));
    qDebug("[client %d] ERROR %s", clientId, code);
}

void GameServer::serveClient(int clientId, const std::shared_ptr<TCPSocket>& socket) {
    socket->setSendTimeout(Protocol::kSendTimeoutMs);
    socket->setRecvTimeout(Protocol::kJoinTimeoutMs);

    bool joined = false;
    std::string line;

    while (true) {
        const TCPSocket::ReadStatus status = socket->readLine(line);
        if (status == TCPSocket::ReadStatus::Closed) {
            qDebug("[client %d] disconnected", clientId);
            return;
        }
        if (status == TCPSocket::ReadStatus::TooLong) {
            sendError(clientId, *socket, Protocol::kLineTooLong, "Line over 2048 bytes");
            return;
        }

        const std::vector<std::string> parts = Protocol::split(line, Protocol::kJoinFieldCount);
        const std::string command = Protocol::split(line, 1).front();

        if (!joined) {
            if (command != Protocol::kJoin) {
                sendError(clientId, *socket, Protocol::kNotJoined, "First message must be JOIN");
                return;
            }
            if (parts.size() != 3) {
                sendError(clientId, *socket, Protocol::kBadFormat, "JOIN needs 2 fields");
                continue;
            }

            qDebug("[client %d] JOIN %s", clientId, parts[2].c_str());
            joined = room_.join(clientId, parts[1], parts[2], socket);
            if (!joined) {
                return;
            }
            socket->setRecvTimeout(0);
            continue;
        }

        int roundId = 0;
        if (command == Protocol::kJoin) {
            sendError(clientId, *socket, Protocol::kAlreadyJoined, "Already joined");
        } else if (command == Protocol::kAnswer) {
            if (parts.size() == 3 && Protocol::parseRoundId(parts[1], roundId)) {
                room_.submitAnswer(clientId, roundId, parts[2]);
            } else {
                sendError(clientId, *socket, Protocol::kBadFormat, "ANSWER needs a roundId number and a text");
            }
        } else if (command == Protocol::kLeave) {
            if (line == Protocol::kLeave) {
                qDebug("[client %d] LEAVE", clientId);
                return;
            }
            sendError(clientId, *socket, Protocol::kBadFormat, "LEAVE has no fields");
        } else {
            sendError(clientId, *socket, Protocol::kUnknownCommand, "Unknown command");
        }
    }
}

void GameServer::forgetSocket(const std::shared_ptr<TCPSocket>& socket) {
    std::lock_guard<std::mutex> lock(mutex_);
    openSockets_.erase(std::remove(openSockets_.begin(), openSockets_.end(), socket), openSockets_.end());
}
