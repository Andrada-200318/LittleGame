package com.littleimpostorgame.protocol

import android.util.Log

sealed interface ServerMessage {

    data class JoinOk(val username: String, val status: JoinStatus) : ServerMessage

    data class JoinError(val code: String, val message: String) : ServerMessage

    data class RoundStart(val roundId: Int, val role: Role, val question: String) : ServerMessage

    data class AnswerOk(val roundId: Int) : ServerMessage

    data class AnswerRejected(val roundId: Int, val code: String) : ServerMessage

    data class AnswersClosed(val roundId: Int) : ServerMessage

    data class RoomClosed(val message: String) : ServerMessage

    data class Error(val code: String, val message: String) : ServerMessage

    companion object {
        private const val TAG = "Impostor"

        private val FIELD_COUNTS = mapOf(
            "JOIN_OK" to 2,
            "JOIN_ERROR" to 2,
            "ROUND_START" to 3,
            "ANSWER_OK" to 1,
            "ANSWER_REJECTED" to 2,
            "ANSWERS_CLOSED" to 1,
            "ROOM_CLOSED" to 1,
            "ERROR" to 2,
        )

        fun parse(rawLine: String): ServerMessage? {
            val line = rawLine.removeSuffix("\r")
            val command = line.substringBefore("|")

            val fieldCount = FIELD_COUNTS[command]
            if (fieldCount == null) {
                Log.w(TAG, "Unknown command, ignored: $line")
                return null
            }

            val parts = line.split("|", limit = fieldCount + 1)
            if (parts.size != fieldCount + 1) {
                Log.w(TAG, "Wrong number of fields, ignored: $line")
                return null
            }

            val message: ServerMessage? = when (command) {
                "JOIN_OK" -> {
                    val status = JoinStatus.entries.find { it.name == parts[2] }
                    if (status == null) null else JoinOk(username = parts[1], status = status)
                }
                "JOIN_ERROR" -> JoinError(code = parts[1], message = parts[2])
                "ROUND_START" -> {
                    val roundId = parts[1].toIntOrNull()
                    val role = Role.entries.find { it.name == parts[2] }
                    if (roundId == null || role == null) null
                    else RoundStart(roundId = roundId, role = role, question = parts[3])
                }
                "ANSWER_OK" -> {
                    val roundId = parts[1].toIntOrNull()
                    if (roundId == null) null else AnswerOk(roundId)
                }
                "ANSWER_REJECTED" -> {
                    val roundId = parts[1].toIntOrNull()
                    if (roundId == null) null else AnswerRejected(roundId = roundId, code = parts[2])
                }
                "ANSWERS_CLOSED" -> {
                    val roundId = parts[1].toIntOrNull()
                    if (roundId == null) null else AnswersClosed(roundId)
                }
                "ROOM_CLOSED" -> RoomClosed(message = parts[1])
                "ERROR" -> Error(code = parts[1], message = parts[2])
                else -> null
            }

            if (message == null) {
                Log.w(TAG, "Bad field value, ignored: $line")
            }
            return message
        }
    }
}

enum class JoinStatus {
    LOBBY,
    WAIT_NEXT_ROUND,
}

enum class Role {
    NORMAL,
    IMPOSTOR,
}
