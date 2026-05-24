package com.mediadownloader.auth

import java.io.File

object CookieManager {
    fun load(site: String): String {
        val path = Paths.resolve("cookies/$site.txt")
        val file = File(path)
        if (!file.exists()) return ""
        return file.readText().trim()
    }
}
