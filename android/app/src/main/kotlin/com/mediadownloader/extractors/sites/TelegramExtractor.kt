package com.mediadownloader.extractors.sites

import com.mediadownloader.extractors.BaseExtractor
import com.mediadownloader.extractors.DownloadTask
import com.mediadownloader.http.HttpClient
import com.mediadownloader.http.HttpRequest

class TelegramExtractor : BaseExtractor {
    override fun matches(url: String): Boolean =
        url.contains("t.me/") || url.contains("telegram.me/")

    override suspend fun extract(url: String): List<DownloadTask> {
        val pattern = Regex("""t(?:elegram)?\.me/([^/?]+)/(\d+)""")
        val match = pattern.find(url) ?: return emptyList()
        val channel = match.groupValues[1]
        val postId = match.groupValues[2]

        val html = fetchEmbed(channel, postId)
        if (html.isEmpty()) return emptyList()

        val urls = extractMediaUrls(html, postId)
        return urls.map { mediaUrl ->
            var filename = mediaUrl.substringAfterLast('/')
            val qpos = filename.indexOf('?')
            if (qpos != -1) filename = filename.substring(0, qpos)

            if (filename.isEmpty() || filename.length > 200) {
                filename = "tg_${channel}_$postId"
                val dot = mediaUrl.lastIndexOf('.')
                val slashOrQ = mediaUrl.indexOf('/', if (dot != -1) dot else 0)
                val q = mediaUrl.indexOf('?', if (dot != -1) dot else 0)
                val end = when {
                    slashOrQ == -1 && q == -1 -> mediaUrl.length
                    slashOrQ == -1 -> q
                    q == -1 -> slashOrQ
                    else -> minOf(slashOrQ, q)
                }
                if (dot != -1 && end > dot) {
                    val ext = mediaUrl.substring(dot, end)
                    if (ext.length <= 10) filename += ext
                }
            }

            DownloadTask(
                url = mediaUrl,
                filename = filename,
                headers = mapOf("Referer" to "https://t.me/")
            )
        }
    }

    private suspend fun fetchEmbed(channel: String, postId: String): String {
        val resp = HttpClient.get(
            HttpRequest(
                url = "https://t.me/s/$channel/$postId",
                userAgent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36"
            )
        )
        return if (resp.statusCode < 400) resp.body else ""
    }

    private fun extractMediaUrls(html: String, postId: String): List<String> {
        val urls = mutableListOf<String>()

        val photoPattern = Regex(
            """<a[^>]*class="tgme_widget_message_photo_wrap[^"]*"[^>]*href="[^"]*/(\d+)"[^>]*style="[^"]*background-image:url\('([^']+)'\)"""
        )
        for (m in photoPattern.findAll(html)) {
            if (m.groupValues[1] == postId) {
                urls.add(m.groupValues[2])
            }
        }
        if (urls.isNotEmpty()) return urls

        for (pattern in listOf(
            Regex("""<video[^>]*src="([^"]+)""""),
            Regex("""<meta[^>]*property="og:image"[^>]*content="([^"]+)""""),
            Regex("""<meta[^>]*property="og:video"[^>]*content="([^"]+)"""")
        )) {
            val m = pattern.find(html) ?: continue
            urls.add(m.groupValues[1])
            break
        }

        return urls
    }
}
