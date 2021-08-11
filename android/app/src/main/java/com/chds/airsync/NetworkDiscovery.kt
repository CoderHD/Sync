package com.chds.airsync

import android.annotation.SuppressLint
import android.os.AsyncTask
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetAddress
import java.util.concurrent.atomic.AtomicBoolean

class DiscoveredClient(val clientIp: ByteArray, val clientId: Int, val clientName: String)
interface DiscoveryEventHandler {
    fun discovered(discoveredClient: DiscoveredClient)
}

const val NETWORK_DISCOVERY_PORT = 2001
const val DISCOVERY_MSG_SIZE = 256

class NetworkDiscovery(val localClient: DiscoveredClient, val discoveryEventHandler: DiscoveryEventHandler) {
    private val serverSocket: DatagramSocket
    private val serverTask: AsyncTask<Void, DiscoveredClient, Boolean>
    private var serverRunning = AtomicBoolean(true)

    private fun populateIntToBuffer(value: Int, buffer: ByteArray, off: Int): Int {
        buffer[off + 0] = ((value shr 0) and 0xff).toByte()
        buffer[off + 1] = ((value shr 8) and 0xff).toByte()
        buffer[off + 2] = ((value shr 16) and 0xff).toByte()
        buffer[off + 3] = ((value shr 24) and 0xff).toByte()
        return 4
    }

    private fun intFromBuffer(buffer: ByteArray, off: Int): Int {
        return ((buffer[0].toInt() shl 32) or (buffer[1].toInt() shl 24)
                or (buffer[2].toInt() shl 16) or (buffer[1].toInt() shl 8)
                or (buffer[0].toInt() shl 0))
    }

    private fun populateBuffer(clientId: Int, clientName: String, buffer: ByteArray, offset: Int): Int {
        var off = offset
        off += populateIntToBuffer(clientId, buffer, off)
        off += populateIntToBuffer(clientName.length, buffer, off)
        for (i in 0..clientName.length)
            buffer[off + i] = clientName[i].toByte()
        return off + clientName.length
    }

    private fun msgFromBuffer(clientIp: ByteArray, buffer: ByteArray, off: Int): DiscoveredClient {
        val clientId = intFromBuffer(buffer, off + 0)
        val clientNameLength = intFromBuffer(buffer, off + 4)
        val clientName = String(buffer, off + 8, clientNameLength)
        return DiscoveredClient(clientIp, clientId, clientName)
    }

    init {
        serverSocket = DatagramSocket(NETWORK_DISCOVERY_PORT, InetAddress.getByName("0.0.0.0"))
        serverSocket.broadcast = true

        serverTask = @SuppressLint("StaticFieldLeak") object: AsyncTask<Void, DiscoveredClient, Boolean>() {
            override fun doInBackground(vararg params: Void?): Boolean {
                val recieveBuffer = ByteArray(DISCOVERY_MSG_SIZE)
                val recievePacket = DatagramPacket(recieveBuffer, recieveBuffer.size)
                val sendBuff = ByteArray(DISCOVERY_MSG_SIZE)
                val sendSocket = DatagramSocket()

                while (serverRunning.get()) {
                    serverSocket.receive(recievePacket)
                    val recieveAddress = InetAddress.getByAddress(recievePacket.data)

                    while (true) {
                        // Anfrage abwarten
                        serverSocket.receive(recievePacket)
                        val remoteClient = msgFromBuffer(recieveAddress.address, recieveBuffer, 0)

                        // Antwort erstellen
                        val sendBuffSize = populateBuffer(localClient.clientId, localClient.clientName, sendBuff, 0)

                        // Puffer senden
                        val sendPacket = DatagramPacket(sendBuff, sendBuffSize, recieveAddress, NETWORK_DISCOVERY_PORT)
                        sendSocket.send(sendPacket)

                        // Notification senden
                        publishProgress(remoteClient)
                    }
                }
                serverSocket.close()
                return true
            }

            override fun onProgressUpdate(vararg clients: DiscoveredClient?) {
                for (client in clients)
                    discoveryEventHandler.discovered(client!!)
            }
        }
        serverTask.execute()
    }

    public fun sendDiscoveryRequest() {
        val buffer = ByteArray(DISCOVERY_MSG_SIZE)
        val length = populateBuffer(localClient.clientId, localClient.clientName, buffer, 0)
        val packet = DatagramPacket(buffer, length)
        serverSocket.send(packet)
    }

    public fun stopServer() {
        serverRunning.set(false)
    }
}