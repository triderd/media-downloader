package com.mediadownloader.extractors

import com.mediadownloader.extractors.sites.DanbooruExtractor
import com.mediadownloader.extractors.sites.TelegramExtractor
import com.mediadownloader.extractors.sites.YtDlpExtractor

class ExtractorManager {
    private val extractors = listOf<BaseExtractor>(
        TelegramExtractor(),
        DanbooruExtractor(),
        YtDlpExtractor()
    )

    suspend fun extract(url: String): List<DownloadTask> {
        for (extractor in extractors) {
            if (extractor.matches(url)) {
                return extractor.extract(url)
            }
        }
        return emptyList()
    }
}
