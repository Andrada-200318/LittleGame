package com.littleimpostorgame.protocol

sealed interface ClientMessage {
    fun encode(): String

    data class Join(val password: String, val username: String) : ClientMessage {
        override fun encode(): String =
            "JOIN|${toSingleLine(password)}|${toSingleLine(username).trim()}"
    }

    data class Answer(val roundId: Int, val text: String) : ClientMessage {
        override fun encode(): String =
            "ANSWER|$roundId|${toSingleLine(text).trim()}"
    }

    data object Leave : ClientMessage {
        override fun encode(): String = "LEAVE"
    }
}

private fun toSingleLine(text: String): String =
    text.replace('\n', ' ').replace('\r', ' ')
