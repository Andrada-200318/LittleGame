package com.littleimpostorgame.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import com.littleimpostorgame.protocol.JoinStatus

@Composable
fun LobbyScreen(
    username: String,
    status: JoinStatus,
    onExit: () -> Unit,
    modifier: Modifier = Modifier,
) {
    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(24.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp, Alignment.CenterVertically),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        Text(text = "Connected as $username", style = MaterialTheme.typography.titleLarge)

        Text(
            text = when (status) {
                JoinStatus.LOBBY -> "Waiting for the host to start the round"
                JoinStatus.WAIT_NEXT_ROUND -> "A round is in progress. You'll play from the next one."
            },
            textAlign = TextAlign.Center,
        )

        OutlinedButton(onClick = onExit) {
            Text("Exit room")
        }
    }
}
