package com.mediadownloader.extractors

import com.mediadownloader.extractors.sites.DanbooruExtractor
import com.mediadownloader.extractors.sites.InstagramExtractor
import com.mediadownloader.extractors.sites.PatternExtractor
import com.mediadownloader.extractors.sites.PixivExtractor
import com.mediadownloader.extractors.sites.TelegramExtractor
import com.mediadownloader.extractors.sites.TikTokExtractor
import com.mediadownloader.extractors.sites.TwitterExtractor
import com.mediadownloader.extractors.sites.YouTubeExtractor
import com.mediadownloader.extractors.sites.YtDlpExtractor

class ExtractorManager {
    private val extractors = listOf<BaseExtractor>(
        PixivExtractor(),
        YouTubeExtractor(),
        TelegramExtractor(),
        InstagramExtractor(),
        TwitterExtractor(),
        TikTokExtractor(),
        DanbooruExtractor(),
        PatternExtractor(),
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
