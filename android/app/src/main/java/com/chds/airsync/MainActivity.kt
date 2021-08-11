package com.chds.airsync

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import java.security.SecureRandom

class MainActivity: AppCompatActivity() {

    val ID_KEY = "id"

    val clients = ArrayList<DiscoveredClient>()
    val id: Int
    val storage: Storage
    val network: Network
    val networkDiscovery: NetworkDiscovery

    init {
        storage = Storage(this)
        if (storage.quickExists(ID_KEY)) {
            id = storage.quickGetInt(ID_KEY)
        } else {
            val secureRandom = SecureRandom()
            id = secureRandom.nextInt()
            storage.quickEditBegin(); storage.quickStore(ID_KEY, id); storage.quickEditEnd()
        }

        network = Network()

        val localClient = DiscoveredClient(byteArrayOf(), id, android.os.Build.MODEL)
        val discoveryEventHandler = object: DiscoveryEventHandler {
            override fun discovered(discoveredClient: DiscoveredClient) {
                for (client in clients) if (client.clientId == discoveredClient.clientId) return
                clients.add(discoveredClient)
            }
        }
        networkDiscovery = NetworkDiscovery(localClient, discoveryEventHandler)
        networkDiscovery.sendDiscoveryRequest()
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
    }
}