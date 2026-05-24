package com.mediadownloader.config

import android.content.Context
import android.os.Environment
import com.mediadownloader.auth.Paths
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json
import java.io.File

@Serializable
data class ConfigData(
    val download_dir: String = ".",
    val default_format: String = "bestvideo+bestaudio/best",
    val concurrent_downloads: Int = 3
)

object Config {
    private val json = Json { ignoreUnknownKeys = true }
    private var data = ConfigData()

    fun load(path: String = "config.json") {
        val resolved = Paths.resolve(path)
        val file = File(resolved)
        if (!file.exists()) return

        try {
            val text = file.readText()
            data = json.decodeFromString(ConfigData.serializer(), text)
        } catch (_: Exception) {
        }
    }

    fun getDownloadDir() = data.download_dir

    fun getDefaultFormat() = data.default_format

    fun getConcurrentDownloads() = data.concurrent_downloads

    fun resolveDownloadDir(context: Context): String {
        val configured = data.download_dir
        if (configured != ".") return configured
        return context.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS)?.absolutePath
            ?: context.filesDir.absolutePath
    }
}
