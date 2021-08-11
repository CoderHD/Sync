package com.chds.airsync

import android.content.Context
import android.content.SharedPreferences
import androidx.appcompat.app.AppCompatActivity

class Storage(context: Context) {
    val prefs: SharedPreferences
    var prefsEdit: SharedPreferences.Editor? = null

    init {
        prefs = context.getSharedPreferences("Storage", AppCompatActivity.MODE_PRIVATE)
    }

    fun quickExists(key: String) = prefs.contains(key)
    fun quickGetInt(key: String, def: Int = 0) = prefs.getInt(key, def)
    fun quickGetLong(key: String, def: Long = 0) = prefs.getLong(key, def)
    fun quickEditBegin() {
        prefsEdit = prefs.edit()
    }

    fun quickStore(key: String, value: Int) {
        prefsEdit!!.putInt(key, value).apply()
    }

    fun quickStore(key: String, value: Long) {
        prefsEdit!!.putLong(key, value).apply()
    }

    fun quickEditEnd() {
        prefsEdit = null
    }
}