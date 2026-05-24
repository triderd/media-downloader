package com.mediadownloader.extractors.sites

import com.mediadownloader.config.Config
import com.mediadownloader.extractors.BaseExtractor
import com.mediadownloader.extractors.DownloadTask

class TwitterExtractor : BaseExtractor {
    override fun matches(url: String): Boolean =
        url.contains("twitter.com") || url.contains("x.com")

    override suspend fun extract(url: String): List<DownloadTask> =
        listOf(
            DownloadTask(
                url = url,
                useYtdlp = true,
                formatId = Config.getDefaultFormat()
            )
        )
}
