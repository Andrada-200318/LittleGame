#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Protocol {

constexpr uint16_t kPort = 5000;
constexpr int kMinPlayers = 3;
constexpr int kMaxPlayers = 8;
constexpr int kMaxPasswordChars = 20;
constexpr std::size_t kMaxUsernameBytes = 64;
constexpr std::size_t kMaxAnswerBytes = 800;
constexpr int kEarlyStopSeconds = 5;
constexpr int kJoinTimeoutMs = 10000;
constexpr int kSendTimeoutMs = 2000;

// Client -> server
constexpr const char* kJoin = "JOIN";
constexpr const char* kAnswer = "ANSWER";
constexpr int kJoinFieldCount = 2;
constexpr const char* kLeave = "LEAVE";

// Server -> client
constexpr const char* kJoinOk = "JOIN_OK";
constexpr const char* kJoinError = "JOIN_ERROR";
constexpr const char* kRoundStart = "ROUND_START";
constexpr const char* kAnswerOk = "ANSWER_OK";
constexpr const char* kAnswerRejected = "ANSWER_REJECTED";
constexpr const char* kAnswersClosed = "ANSWERS_CLOSED";
constexpr const char* kRoomClosed = "ROOM_CLOSED";
constexpr const char* kError = "ERROR";

constexpr const char* kStatusLobby = "LOBBY";
constexpr const char* kStatusWaitNextRound = "WAIT_NEXT_ROUND";

constexpr const char* kRoleNormal = "NORMAL";
constexpr const char* kRoleImpostor = "IMPOSTOR";

constexpr const char* kWrongPassword = "WRONG_PASSWORD";
constexpr const char* kInvalidName = "INVALID_NAME";
constexpr const char* kNameTaken = "NAME_TAKEN";
constexpr const char* kRoomFull = "ROOM_FULL";

constexpr const char* kWrongRound = "WRONG_ROUND";
constexpr const char* kNotInRound = "NOT_IN_ROUND";
constexpr const char* kAlreadyAnswered = "ALREADY_ANSWERED";
constexpr const char* kInvalidAnswer = "INVALID_ANSWER";

constexpr const char* kNotJoined = "NOT_JOINED";
constexpr const char* kLineTooLong = "LINE_TOO_LONG";
constexpr const char* kAlreadyJoined = "ALREADY_JOINED";
constexpr const char* kBadFormat = "BAD_FORMAT";
constexpr const char* kUnknownCommand = "UNKNOWN_COMMAND";

std::vector<std::string> split(const std::string& line, int fieldCount);

std::string format(const std::string& command, const std::vector<std::string>& fields);

bool parseRoundId(const std::string& text, int& roundId);

}
