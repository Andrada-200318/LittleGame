package com.littleimpostorgame.ui

import com.littleimpostorgame.protocol.JoinStatus
import com.littleimpostorgame.protocol.Role

const val MAX_PASSWORD_LENGTH = 20
const val MAX_USERNAME_LENGTH = 16
const val MAX_ANSWER_LENGTH = 200

data class GameUiState(
    val host: String = "",
    val port: String = "5000",
    val password: String = "",
    val username: String = "",
    val answer: String = "",
    val screen: Screen = Screen.Connect(),
) {
    val canConnect: Boolean
        get() = isValidIpv4(host) && isValidPort(port) && isValidPassword(password) && isValidUsername(username)
}

sealed interface Screen {
    data class Connect(val error: String? = null) : Screen
    data object Connecting : Screen
    data class Lobby(val username: String, val status: JoinStatus) : Screen
    data class Question(
        val roundId: Int,
        val role: Role,
        val question: String,
        val isSending: Boolean = false,
        val error: String? = null,
    ) : Screen
    data class Waiting(val answersClosed: Boolean) : Screen
}

private fun isValidIpv4(text: String): Boolean {
    val parts = text.split(".")
    return parts.size == 4 && parts.all { part ->
        part.length in 1..3 &&
            part.all { it in '0'..'9' } &&
            (part == "0" || !part.startsWith("0")) &&
            part.toInt() <= 255
    }
}

private fun isValidPort(text: String): Boolean {
    val port = text.toIntOrNull() ?: return false
    return port in 1..65535
}

private fun isValidPassword(text: String): Boolean =
    text.length in 1..MAX_PASSWORD_LENGTH && '|' !in text

private fun isValidUsername(text: String): Boolean {
    val name = text.trim()
    return name.length in 1..MAX_USERNAME_LENGTH && '|' !in name && ',' !in name
}
