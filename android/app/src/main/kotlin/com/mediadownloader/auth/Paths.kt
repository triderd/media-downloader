package com.mediadownloader.auth

import java.io.File

object Paths {
    fun getConfigDir(): String {
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
