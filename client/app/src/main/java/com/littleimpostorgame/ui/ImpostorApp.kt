package com.littleimpostorgame.ui

import androidx.compose.foundation.layout.consumeWindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.imePadding
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Scaffold
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import com.littleimpostorgame.ui.screens.ConnectScreen
import com.littleimpostorgame.ui.screens.LobbyScreen
import com.littleimpostorgame.ui.screens.QuestionScreen
import com.littleimpostorgame.ui.screens.WaitingScreen
import com.littleimpostorgame.ui.theme.LittleImpostorGameTheme

@Composable
fun ImpostorApp(viewModel: GameViewModel) {
    val state by viewModel.state.collectAsState()

    LittleImpostorGameTheme {
        Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
            val screenModifier = Modifier
                .padding(innerPadding)
                .consumeWindowInsets(innerPadding)
                .imePadding()

            when (val screen = state.screen) {
                is Screen.Connect, Screen.Connecting -> ConnectScreen(
                    host = state.host,
                    port = state.port,
                    password = state.password,
                    username = state.username,
                    error = (screen as? Screen.Connect)?.error,
                    isConnecting = screen is Screen.Connecting,
                    canConnect = state.canConnect,
                    onHostChange = viewModel::onHostChange,
                    onPortChange = viewModel::onPortChange,
                    onPasswordChange = viewModel::onPasswordChange,
                    onUsernameChange = viewModel::onUsernameChange,
                    onConnect = viewModel::onConnect,
                    modifier = screenModifier,
                )
                is Screen.Lobby -> LobbyScreen(
                    username = screen.username,
                    status = screen.status,
                    onExit = viewModel::onExitRoom,
                    modifier = screenModifier,
                )
                is Screen.Question -> QuestionScreen(
                    role = screen.role,
                    question = screen.question,
                    answer = state.answer,
                    isSending = screen.isSending,
                    error = screen.error,
                    onAnswerChange = viewModel::onAnswerChange,
                    onSend = viewModel::onSend,
                    onExit = viewModel::onExitRoom,
                    modifier = screenModifier,
                )
                is Screen.Waiting -> WaitingScreen(
                    answersClosed = screen.answersClosed,
                    onExit = viewModel::onExitRoom,
                    modifier = screenModifier,
                )
            }
        }
    }
}
