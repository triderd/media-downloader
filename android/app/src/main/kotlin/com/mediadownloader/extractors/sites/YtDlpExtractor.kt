package com.mediadownloader.extractors.sites

import com.mediadownloader.config.Config
import com.mediadownloader.extractors.BaseExtractor
import com.mediadownloader.extractors.DownloadTask

class YtDlpExtractor : BaseExtractor {
    override fun matches(url: String): Boolean = true

    override suspend fun extract(url: String): List<DownloadTask> {
        return listOf(
            DownloadTask(
                url = url,
                useYtdlp = true,
                formatId = Config.getDefaultFormat()
            )
        )
    }
}
