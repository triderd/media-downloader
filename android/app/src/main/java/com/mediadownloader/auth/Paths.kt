package com.mediadownloader.auth

import android.content.Context
import java.io.File

object Paths {
    private var androidBasePath: String? = null

    fun init(context: Context) {
        androidBasePath = context.filesDir.absolutePath
    }

    fun getConfigDir(): String {
        androidBasePath?.let { return it }

        val xdg = System.getenv("XDG_CONFIG_HOME")
        if (!xdg.isNullOrEmpty()) return "$xdg/media-downloader"

        val home = System.getenv("HOME")
        if (!home.isNullOrEmpty()) return "$home/.config/media-downloader"

        return "."
    }

    fun resolve(filename: String): String {
        if (File(filename).exists()) return filename
        return "${getConfigDir()}/$filename"
    }
}
