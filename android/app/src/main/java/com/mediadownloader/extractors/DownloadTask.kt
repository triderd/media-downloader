package com.mediadownloader.extractors

data class DownloadTask(
    val url: String,
    val filename: String = "",
    val headers: Map<String, String> = emptyMap(),
    val cookies: String = "",
    val useYtdlp: Boolean = false,
    val formatId: String = "bestvideo+bestaudio/best",
    val playlistTitle: String = "",
    val playlistIndex: Int = 0,
    val playlistTotal: Int = 0,
    val frameDelays: List<Int> = emptyList()
)
