package com.mediadownloader.extractors.sites

import com.mediadownloader.config.Config
import com.mediadownloader.extractors.BaseExtractor
import com.mediadownloader.extractors.DownloadTask

class InstagramExtractor : BaseExtractor {
    override fun matches(url: String): Boolean =
        url.contains("instagram.com")

    override suspend fun extract(url: String): List<DownloadTask> =
        listOf(
            DownloadTask(
                url = url,
                useYtdlp = true,
                formatId = Config.getDefaultFormat()
            )
        )
}
