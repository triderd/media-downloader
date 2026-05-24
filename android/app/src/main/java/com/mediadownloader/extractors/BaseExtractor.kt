package com.mediadownloader.extractors

interface BaseExtractor {
    fun matches(url: String): Boolean
    suspend fun extract(url: String): List<DownloadTask>
}
