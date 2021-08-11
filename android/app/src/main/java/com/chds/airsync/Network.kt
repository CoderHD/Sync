package com.chds.airsync

import android.annotation.SuppressLint
import android.os.AsyncTask
import java.io.InputStream
import java.net.DatagramPacket
import java.net.ServerSocket
import java.security.MessageDigest
import java.util.concurrent.BlockingQueue
import java.util.concurrent.LinkedBlockingQueue
import java.util.concurrent.atomic.AtomicBoolean

// Eingebaute Nachrichten-Ids
const val CONNECT_REQ_MSG = 0
const val CONNECT_REQ_DENY_MSG = 1
const val CONNECT_REQ_ACCEPT_MSG = 2
const val MULTIPACKET_INIT_MSG = 3
const val MULTIPACKET_DATA_MSG = 3
const val MULTIPACKET_VERIFY_MSG = 3

interface MessageResponseHandler {
    fun response(msg: Message)
}

abstract class Message {
    companion object {
        var NEXT_ID = 0
    }

    var id = NEXT_ID++
    abstract fun headerType(): Int
    abstract fun bufferLength(): Int
    abstract fun bufferFill(buffer: ByteArray, off: Int)
    abstract fun bufferRetrieveData(buffer: ByteArray, bufferSize: Int, dataOff: Int): Message
    abstract fun requestHandling(): Boolean
    abstract fun requestHandler(): MessageResponseHandler
    abstract fun awaitSynchronousAnswer(): Boolean
    abstract fun multipacketInfo(): MessageMultipacketInfo
}

class MessageHeaderInfo(val type: Short, val id: Int, val singlepacketDataSize: Int);
class MessageMultipacketInfo(val enabled: Boolean, val stream: InputStream?) {
    constructor(enabled: Boolean): this(enabled, null)
}

interface ServerConnectHandler {
    fun connectionUpdated(connected: Boolean)
}

class ServerConnectMessage: Message() {
    var connectHandler: ServerConnectHandler? = null
    val responseHandler = object: MessageResponseHandler {
        override fun response(msg: Message) {
            when (msg.headerType()) {
                CONNECT_REQ_ACCEPT_MSG -> {
                    connectHandler?.connectionUpdated(true)
                }
                CONNECT_REQ_DENY_MSG -> {
                    connectHandler?.connectionUpdated(false)
                }
            }
        }
    }
    val multipacketInfo = MessageMultipacketInfo(false)

    override fun headerType() = CONNECT_REQ_MSG
    override fun bufferLength() = 0
    override fun bufferFill(buffer: ByteArray, off: Int) {}
    override fun bufferRetrieveData(buffer: ByteArray, bufferSize: Int, dataOff: Int): Message {
        return ServerConnectMessage()
    }

    override fun requestHandling() = connectHandler != null
    override fun requestHandler() = responseHandler
    override fun awaitSynchronousAnswer() = false
    override fun multipacketInfo() = multipacketInfo
}

val NETWORK_PORT = 3232
val NETWORK_MSG_SIZE = 1024

class Network {

    private val serverSocket: ServerSocket
    private val serverTask: AsyncTask<Void, ServerConnectMessage, Boolean>
    private var serverRunning = AtomicBoolean(true)
    private var serverDeviceConnected = false

    private val waitingQueue = Object()
    private val msgQueue: BlockingQueue<Message>
    private val msgMap: HashMap<Int, Message>

    var MSG_HEADER_SIZE: Int

    private fun shortFromBuff(buffer: ByteArray, off: Int): Short {
        return ((buffer[0].toInt() shl 8) or (buffer[1].toInt() shl 0)).toShort()
    }

    private fun intFromBuff(buffer: ByteArray, off: Int): Int {
        return (buffer[3].toInt() shl 24) or (buffer[2].toInt() shl 16) or
                (buffer[1].toInt() shl 8) or (buffer[0].toInt() shl 0)
    }

    private fun shortToBuff(value: Short, buffer: ByteArray, off: Int): Short {
        val valueInt = value.toInt()
        buffer[off + 0] = ((valueInt shr 0) and 0xff).toByte()
        buffer[off + 1] = ((valueInt shr 8) and 0xff).toByte()
        return 2
    }

    private fun intToBuff(value: Int, buffer: ByteArray, off: Int): Short {
        buffer[off + 0] = ((value shr 0) and 0xff).toByte()
        buffer[off + 1] = ((value shr 8) and 0xff).toByte()
        buffer[off + 2] = ((value shr 16) and 0xff).toByte()
        buffer[off + 3] = ((value shr 24) and 0xff).toByte()
        return 4
    }

    private fun populateBufferHeader(type: Short, msgId: Int, dataSize: Int, buffer: ByteArray, off: Int): Int {
        var length = 0
        length += shortToBuff(type, buffer, off + length)
        length += intToBuff(msgId, buffer, off + length)
        length += intToBuff(dataSize, buffer, off + length)
        return length
    }

    private fun msgHeaderInfoFromBuffer(buffer: ByteArray, off: Int): MessageHeaderInfo {
        return MessageHeaderInfo(shortFromBuff(buffer, off + 0), intFromBuff(buffer, off + 2), intFromBuff(buffer, off + 6))
    }

    init {
        serverSocket = ServerSocket(NETWORK_PORT)

        msgQueue = LinkedBlockingQueue()
        msgMap = HashMap()
        MSG_HEADER_SIZE = 0

        serverTask = @SuppressLint("StaticFieldLeak") object: AsyncTask<Void, ServerConnectMessage, Boolean>() {
            override fun doInBackground(vararg params: Void?): Boolean {
                val recieveBuffer = ByteArray(NETWORK_MSG_SIZE)
                MSG_HEADER_SIZE = populateBufferHeader(0, 0, 0, recieveBuffer, 0)
                val recievePacket = DatagramPacket(recieveBuffer, recieveBuffer.size)
                var recieveNow = false
                val sendBuff = ByteArray(NETWORK_MSG_SIZE)
                val hashDigest = MessageDigest.getInstance("MD5")

                while (serverRunning.get()) {
                    val socket = serverSocket.accept()
                    val socketInput = socket.getInputStream()
                    val socketOutput = socket.getOutputStream()
                    val addressBuffer = ByteArray(4)
                    socketInput.read(recieveBuffer, 0, 4)
                    //val recieveAddress = InetAddress.getByAddress(addressBuffer)

                    var setupSent = false
                    while (true) {
                        if (setupSent) {
                            synchronized(waitingQueue) {
                                waitingQueue.wait()
                            }
                        }
                        setupSent = true

                        fun sendPacket(sendBuffLength: Int) {
                            socketOutput.write(sendBuff, 0, sendBuffLength)
                            //val sendPacket = DatagramPacket(sendBuff, sendBuffLength, recieveAddress, NETWORK_PORT)
                            //sendSocket.send(sendPacket)
                        }

                        while (msgQueue.isNotEmpty()) {
                            val msg = msgQueue.poll()!!

                            // Puffer füllen und senden
                            msgMap[msg.id] = msg
                            var sendBuffInd = populateBufferHeader(msg.headerType().toShort(), msg.bufferLength(), sendBuff, 0)
                            msg.bufferFill(sendBuff, sendBuffInd); sendBuffInd += msg.bufferLength()
                            sendPacket(sendBuffInd)
                            // Multipacket Nachrichten senden
                            val multipacketInfo = msg.multipacketInfo()
                            if (multipacketInfo.enabled) {
                                // 1. Multipacket-Init Nachricht senden
                                sendBuffInd = populateBufferHeader(MULTIPACKET_INIT_MSG.toShort(), Int.SIZE_BYTES, msg.id, sendBuff, 0)
                                sendBuffInd += intToBuff(multipacketInfo.stream!!.available(), sendBuff, sendBuffInd)
                                sendPacket(sendBuffInd)

                                // 2. Multipacket-Data Nachrichten senden
                                sendBuffInd = MSG_HEADER_SIZE
                                val freeDataSize = NETWORK_MSG_SIZE - sendBuffInd
                                populateBufferHeader(MULTIPACKET_DATA_MSG.toShort(), freeDataSize, msg.id, sendBuff, 0)
                                while (multipacketInfo.stream.available() > freeDataSize) {
                                    multipacketInfo.stream.read(sendBuff, sendBuffInd, freeDataSize)
                                    hashDigest.update(sendBuff, sendBuffInd, freeDataSize)
                                    sendPacket(NETWORK_MSG_SIZE)
                                }
                                val remainingDataSize = multipacketInfo.stream.available()
                                sendBuffInd = populateBufferHeader(MULTIPACKET_DATA_MSG.toShort(), remainingDataSize, msg.id, sendBuff, 0)
                                multipacketInfo.stream.read(sendBuff, sendBuffInd, remainingDataSize)
                                hashDigest.update(sendBuff, sendBuffInd, remainingDataSize)
                                sendPacket(NETWORK_MSG_SIZE)

                                // 3. Multipacket-Verify Nachrichten senden
                                val dataHash = hashDigest.digest()
                                sendBuffInd = populateBufferHeader(MULTIPACKET_VERIFY_MSG.toShort(), dataHash.size, msg.id, sendBuff, 0)
                                for (i in dataHash.indices) {
                                    sendBuff[sendBuffInd + i] = dataHash[i]
                                }; sendBuffInd += dataHash.size
                                sendPacket(sendBuffInd)
                            }
                            // RecieveNow Status verändern
                            recieveNow = recieveNow or msg.awaitSynchronousAnswer()
                        }

                        // Puffer annehmen
                        if (recieveNow) {
                            while (recieveNow) {
                                socketInput.read(recieveBuffer, 0, MSG_HEADER_SIZE)
                                val headerInfo = msgHeaderInfoFromBuffer(recievePacket.data, 0)
                                socketInput.read(recieveBuffer, MSG_HEADER_SIZE, headerInfo.singlepacketDataSize)

                                if (msgMap.containsKey(headerInfo.id)) {
                                    val msg = msgMap[headerInfo.id]!!
                                    val response = msg.bufferRetrieveData(recievePacket.data, headerInfo.singlepacketDataSize, 6)
                                    if (msg.requestHandling()) {
                                        msg.requestHandler().response(response)
                                    }
                                }
                            }
                        }
                    }
                }
                serverSocket.close()
                return true
            }

            override fun onProgressUpdate(vararg serverStates: ServerConnectMessage?) {
            }
        }
        serverTask.execute()
    }

    private fun notifyServer() {
        synchronized(waitingQueue) {
            waitingQueue.notify()
        }
    }

    public fun sendMessage(msg: Message): Boolean {
        if (serverDeviceConnected) {
            msgQueue.add(msg)
            notifyServer()
        }
        return serverDeviceConnected
    }

    public fun stopServer() {
        serverRunning.set(false)
    }
}