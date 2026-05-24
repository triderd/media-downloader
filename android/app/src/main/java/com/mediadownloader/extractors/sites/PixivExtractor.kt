package com.mediadownloader.extractors.sites

import com.mediadownloader.auth.CookieManager
import com.mediadownloader.extractors.BaseExtractor
import com.mediadownloader.extractors.DownloadTask
import com.mediadownloader.http.HttpClient
import com.mediadownloader.http.HttpRequest
import kotlinx.serialization.json.*

class PixivExtractor : BaseExtractor {
    private val json = Json { ignoreUnknownKeys = true }

    override fun matches(url: String): Boolean =
        url.contains("pixiv.net")

    override suspend fun extract(url: String): List<DownloadTask> {
        val artworkId = extractArtworkId(url)
        if (artworkId.isEmpty()) return emptyList()

        val cookies = CookieManager.load("pixiv")

        val infoBody = fetchJson(
            "https://www.pixiv.net/ajax/illust/$artworkId",
            cookies
        ) ?: return emptyList()

        val body = infoBody["body"]?.jsonObject ?: return emptyList()
        val illustType = body["illustType"]?.jsonPrimitive?.int ?: 0

        if (illustType == 2) {
            return extractUgoira(artworkId, cookies)
        }

        return extractPages(artworkId, cookies)
    }

    private suspend fun extractUgoira(
        artworkId: String,
        cookies: String
    ): List<DownloadTask> {
        val metaBody = fetchJson(
            "https://www.pixiv.net/ajax/illust/$artworkId/ugoira_meta",
            cookies
        ) ?: return emptyList()

        val body = metaBody["body"]?.jsonObject ?: return emptyList()
        val zipUrl = body["originalSrc"]?.jsonPrimitive?.content ?: return emptyList()

        val frameDelays = mutableListOf<Int>()
        for (frame in body["frames"]?.jsonArray ?: emptyList()) {
            frame.jsonObject["delay"]?.jsonPrimitive?.int?.let { frameDelays.add(it) }
        }

        return listOf(
            DownloadTask(
                url = zipUrl,
                filename = "${artworkId}_ugoira.zip",
                headers = mapOf("Referer" to "https://www.pixiv.net/"),
                cookies = cookies,
                frameDelays = frameDelays
            )
        )
    }

    private suspend fun extractPages(
        artworkId: String,
        cookies: String
    ): List<DownloadTask> {
        val pagesBody = fetchJson(
            "https://www.pixiv.net/ajax/illust/$artworkId/pages",
            cookies
        ) ?: return emptyList()

        val pages = pagesBody["body"]?.jsonArray ?: return emptyList()

        return pages.mapNotNull { page ->
            val originalUrl = page.jsonObject["urls"]
                ?.jsonObject
                ?.get("original")
                ?.jsonPrimitive
                ?.content ?: return@mapNotNull null

            DownloadTask(
                url = originalUrl,
                filename = originalUrl.substringAfterLast('/'),
                headers = mapOf("Referer" to "https://www.pixiv.net/"),
                cookies = cookies
            )
        }
    }

    private suspend fun fetchJson(
        url: String,
        cookies: String
    ): JsonObject? {
        val resp = HttpClient.get(
            HttpRequest(
                url = url,
                userAgent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36",
                referer = "https://www.pixiv.net/",
                headers = mapOf("Referer" to "https://www.pixiv.net/"),
                cookies = cookies
            )
        )
        if (resp.statusCode >= 400) return null

        return try {
            json.parseToJsonElement(resp.body).jsonObject
        } catch (_: Exception) {
            null
        }
    }

    private fun extractArtworkId(url: String): String {
        val pattern = Regex("""/artworks/(\d+)""")
        return pattern.find(url)?.groupValues?.get(1) ?: ""
    }
}
