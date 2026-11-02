package com.littleimpostorgame.ui

import android.util.Log
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.littleimpostorgame.net.GameConnection
import com.littleimpostorgame.protocol.ClientMessage
import com.littleimpostorgame.protocol.ServerMessage
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import java.io.IOException

class GameViewModel : ViewModel() {

    private val _state = MutableStateFlow(GameUiState())
    val state: StateFlow<GameUiState> = _state.asStateFlow()

    private var connection: GameConnection? = null
    private var connectionJob: Job? = null
    private var joinTimeoutJob: Job? = null

    fun onHostChange(value: String) = _state.update { it.copy(host = value) }

    fun onPortChange(value: String) = _state.update { it.copy(port = value) }

    fun onPasswordChange(value: String) =
        _state.update { it.copy(password = value.take(MAX_PASSWORD_LENGTH)) }

    fun onUsernameChange(value: String) =
        _state.update { it.copy(username = value.take(MAX_USERNAME_LENGTH)) }

    fun onAnswerChange(value: String) =
        _state.update { it.copy(answer = value.take(MAX_ANSWER_LENGTH)) }

    fun onConnect() {
        val form = _state.value
        if (form.screen !is Screen.Connect || !form.canConnect) return
        val port = form.port.toInt()

        _state.update { it.copy(screen = Screen.Connecting) }

        val newConnection = GameConnection()
        connection = newConnection
        connectionJob = viewModelScope.launch {
            try {
                newConnection.connect(form.host, port)
            } catch (e: IOException) {
                Log.w(TAG, "Connecting to ${form.host}:$port failed", e)
                closeConnection()
                showConnectError(
                    "Couldn't reach the host at ${form.host}:$port. Check the IP, that you're on " +
                        "the same Wi-Fi as the host, and the host's firewall."
                )
                return@launch
            }

            newConnection.send(ClientMessage.Join(password = form.password, username = form.username))
            startJoinTimeout()

            try {
                newConnection.lines().collect { line ->
                    val message = ServerMessage.parse(line)
                    if (message != null) onServerMessage(message)
                }
            } catch (e: IOException) {
                Log.w(TAG, "Reading from the host failed", e)
            }
            onConnectionEnded()
        }
    }

    fun onSend() {
        val current = _state.value
        val screen = current.screen as? Screen.Question ?: return
        if (screen.isSending || current.answer.isBlank()) return
        val currentConnection = connection ?: return

        _state.update { it.copy(screen = screen.copy(isSending = true, error = null)) }
        viewModelScope.launch {
            currentConnection.send(ClientMessage.Answer(roundId = screen.roundId, text = current.answer))
        }
    }

    fun onExitRoom() {
        val screen = _state.value.screen
        if (screen is Screen.Connect || screen is Screen.Connecting) return
        val leavingConnection = connection ?: return

        stopReading()
        connection = null
        _state.update { it.copy(screen = Screen.Connect()) }

        viewModelScope.launch {
            try {
                leavingConnection.send(ClientMessage.Leave)
            } finally {
                leavingConnection.close()
            }
        }
    }

    private fun startJoinTimeout() {
        joinTimeoutJob = viewModelScope.launch {
            delay(JOIN_REPLY_TIMEOUT_MS)
            joinTimeoutJob = null
            if (_state.value.screen is Screen.Connecting) {
                closeConnection()
                showConnectError("The host didn't answer.")
            }
        }
    }

    private fun onServerMessage(message: ServerMessage) {
        val screen = _state.value.screen
        when (message) {
            is ServerMessage.JoinOk -> {
                if (screen !is Screen.Connecting) return
                cancelJoinTimeout()
                _state.update {
                    it.copy(screen = Screen.Lobby(username = message.username, status = message.status))
                }
            }
            is ServerMessage.JoinError -> {
                if (screen !is Screen.Connecting) return
                closeConnection()
                showConnectError(joinErrorText(message.code))
            }
            is ServerMessage.RoundStart -> {
                if (screen is Screen.Connect || screen is Screen.Connecting) return
                _state.update {
                    it.copy(
                        answer = "",
                        screen = Screen.Question(
                            roundId = message.roundId,
                            role = message.role,
                            question = message.question,
                        ),
                    )
                }
            }
            is ServerMessage.AnswerOk -> {
                if (screen !is Screen.Question || message.roundId != screen.roundId) return
                showWaiting(answersClosed = false)
            }
            is ServerMessage.AnswerRejected -> {
                if (screen !is Screen.Question || message.roundId != screen.roundId) return
                when (message.code) {
                    "ALREADY_ANSWERED" -> showWaiting(answersClosed = false)
                    "ANSWERS_CLOSED" -> showWaiting(answersClosed = true)
                    "INVALID_ANSWER" -> _state.update {
                        it.copy(screen = screen.copy(isSending = false, error = "Answers can be 1–200 characters."))
                    }
                    else -> Log.w(TAG, "Answer rejected, nothing to change: ${message.code}")
                }
            }
            is ServerMessage.AnswersClosed -> {
                if (screen !is Screen.Question || message.roundId != screen.roundId) return
                if (screen.isSending) return
                showWaiting(answersClosed = true)
            }
            is ServerMessage.RoomClosed -> {
                closeConnection()
                showConnectError("The host ended the room.")
            }
            is ServerMessage.Error -> {
                Log.w(TAG, "The host says we broke the protocol: ${message.code} ${message.message}")
            }
        }
    }

    private fun onConnectionEnded() {
        if (_state.value.screen !is Screen.Connect) {
            closeConnection()
            showConnectError("Lost connection to the host.")
        }
    }

    private fun joinErrorText(code: String): String = when (code) {
        "WRONG_PASSWORD" -> "Wrong password."
        "INVALID_NAME" -> "Names can be 1–16 characters, without | or ,"
        "NAME_TAKEN" -> "Someone in the room already uses that name."
        "ROOM_FULL" -> "The room is full (8 players)."
        else -> {
            Log.w(TAG, "Unknown JOIN_ERROR code: $code")
            "The host refused to let you join."
        }
    }

    private fun showWaiting(answersClosed: Boolean) {
        _state.update { it.copy(screen = Screen.Waiting(answersClosed = answersClosed)) }
    }

    private fun showConnectError(text: String) {
        _state.update { it.copy(screen = Screen.Connect(error = text)) }
    }

    private fun cancelJoinTimeout() {
        joinTimeoutJob?.cancel()
        joinTimeoutJob = null
    }

    private fun stopReading() {
        cancelJoinTimeout()
        connectionJob?.cancel()
        connectionJob = null
    }

    private fun closeConnection() {
        stopReading()
        connection?.close()
        connection = null
    }

    override fun onCleared() {
        closeConnection()
    }

    private companion object {
        const val TAG = "Impostor"
        const val JOIN_REPLY_TIMEOUT_MS = 5000L
    }
}
