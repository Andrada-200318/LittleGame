package com.littleimpostorgame.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.littleimpostorgame.protocol.Role
import com.littleimpostorgame.ui.MAX_ANSWER_LENGTH

@Composable
fun QuestionScreen(
    role: Role,
    question: String,
    answer: String,
    isSending: Boolean,
    error: String?,
    onAnswerChange: (String) -> Unit,
    onSend: () -> Unit,
    onExit: () -> Unit,
    modifier: Modifier = Modifier,
) {
    Column(
        modifier = modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState())
            .padding(24.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp),
    ) {
        if (role == Role.IMPOSTOR) {
            Text(
                text = "IMPOSTOR",
                color = MaterialTheme.colorScheme.error,
                fontWeight = FontWeight.Bold,
                style = MaterialTheme.typography.titleMedium,
            )
        }

        Text(text = question, style = MaterialTheme.typography.headlineSmall)

        OutlinedTextField(
            value = answer,
            onValueChange = onAnswerChange,
            label = { Text("Your answer") },
            supportingText = { Text("${answer.length}/$MAX_ANSWER_LENGTH") },
            isError = error != null,
            modifier = Modifier.fillMaxWidth(),
        )

        if (error != null) {
            Text(text = error, color = MaterialTheme.colorScheme.error)
        }

        Button(
            onClick = onSend,
            enabled = answer.isNotBlank() && !isSending,
            modifier = Modifier.fillMaxWidth(),
        ) {
            Text("Send")
        }

        OutlinedButton(onClick = onExit, modifier = Modifier.fillMaxWidth()) {
            Text("Exit room")
        }
    }
}
