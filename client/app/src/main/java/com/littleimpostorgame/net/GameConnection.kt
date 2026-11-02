package com.littleimpostorgame.net

import android.util.Log
import com.littleimpostorgame.protocol.ClientMessage
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.flow.flowOn
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlinx.coroutines.withContext
import java.io.BufferedReader
import java.io.BufferedWriter
import java.io.IOException
import java.net.InetSocketAddress
import java.net.Socket

class GameConnection {

    @Volatile
    private var socket: Socket? = null

    @Volatile
    private var reader: BufferedReader? = null

    @Volatile
    private var writer: BufferedWriter? = null

    private val writeMutex = Mutex()

    suspend fun connect(host: String, port: Int) {
        withContext(Dispatchers.IO) {
            val newSocket = Socket()
            socket = newSocket
            newSocket.connect(InetSocketAddress(host, port), CONNECT_TIMEOUT_MS)
            reader = newSocket.getInputStream().bufferedReader(Charsets.UTF_8)
            writer = newSocket.getOutputStream().bufferedWriter(Charsets.UTF_8)
        }
    }

    fun lines(): Flow<String> = flow {
        val currentReader = reader ?: return@flow
        while (true) emit(currentReader.readLine() ?: break)
    }.flowOn(Dispatchers.IO)

    suspend fun send(message: ClientMessage) {
        withContext(Dispatchers.IO) {
            writeMutex.withLock {
                val currentWriter = writer ?: return@withLock
                try {
                    currentWriter.write(message.encode() + "\n")
                    currentWriter.flush()
                } catch (e: IOException) {
                    Log.w(TAG, "Sending failed", e)
                }
            }
        }
    }

    fun close() {
        try {
            socket?.close()
        } catch (e: IOException) {
            Log.w(TAG, "Closing the socket failed", e)
        }
    }

    private companion object {
        const val TAG = "Impostor"
        const val CONNECT_TIMEOUT_MS = 5000
    }
}
