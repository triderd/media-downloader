package com.mediadownloader.extractors.sites

import com.mediadownloader.auth.CookieManager
import com.mediadownloader.auth.Paths
import com.mediadownloader.extractors.BaseExtractor
import com.mediadownloader.extractors.DownloadTask
import com.mediadownloader.http.HttpClient
import com.mediadownloader.http.HttpRequest
import kotlinx.serialization.json.*

class PatternExtractor : BaseExtractor {
    private val json = Json { ignoreUnknownKeys = true }
    private val rules = mutableListOf<PatternRule>()

    init {
        loadRules("patterns.json")
    }

    data class PatternRule(
        val name: String,
        val urlMatch: Regex,
        val mediaPatterns: List<Regex>,
        val headers: Map<String, String> = emptyMap(),
        val cookiesFrom: String = ""
    )

    override fun matches(url: String): Boolean {
        return rules.any { it.urlMatch.containsMatchIn(url) }
    }

    override suspend fun extract(url: String): List<DownloadTask> {
        val rule = rules.firstOrNull { it.urlMatch.containsMatchIn(url) } ?: return emptyList()

        val html = fetchPage(url, rule)
        if (html.isEmpty()) return emptyList()

        val mediaUrls = mutableSetOf<String>()
        for (pattern in rule.mediaPatterns) {
            for (match in pattern.findAll(html)) {
                mediaUrls.add(match.groupValues[1])
            }
        }

        val cookies = if (rule.cookiesFrom.isNotEmpty()) {
            CookieManager.load(rule.cookiesFrom)
        } else ""

        return mediaUrls.map { mediaUrl ->
            val resolved = resolveUrl(url, mediaUrl)
            val filename = resolved.substringAfterLast('/')
                .substringBefore('?')
                .ifEmpty { "downloaded_file" }

            DownloadTask(
                url = resolved,
                filename = filename,
                headers = rule.headers,
                cookies = cookies
            )
        }
    }

    private suspend fun fetchPage(url: String, rule: PatternRule): String {
        val resp = HttpClient.get(
            HttpRequest(
                url = url,
                userAgent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36",
                referer = rule.headers["Referer"] ?: "",
                headers = rule.headers,
                cookies = if (rule.cookiesFrom.isNotEmpty()) {
                    CookieManager.load(rule.cookiesFrom)
                } else ""
            )
        )
        return if (resp.statusCode < 400) resp.body else ""
    }

    private fun resolveUrl(pageUrl: String, mediaUrl: String): String {
        if (mediaUrl.startsWith("http://") || mediaUrl.startsWith("https://")) {
            return mediaUrl
        }
        if (mediaUrl.startsWith("//")) {
            return "https:$mediaUrl"
        }

        val protocolPos = pageUrl.indexOf("://")
        if (protocolPos == -1) return mediaUrl

        val domainStart = protocolPos + 3
        val pathPos = pageUrl.indexOf('/', domainStart)
        val base = if (pathPos == -1) pageUrl else pageUrl.substring(0, pathPos)

        return if (mediaUrl.startsWith("/")) "$base$mediaUrl" else "$base/$mediaUrl"
    }

    private fun loadRules(path: String) {
        val resolved = Paths.resolve(path)
        val file = java.io.File(resolved)
        if (!file.exists()) return

        val text = try { file.readText() } catch (_: Exception) { return }

        val config = try {
            json.parseToJsonElement(text).jsonArray
        } catch (_: Exception) { return }

        for (entry in config) {
            val obj = entry.jsonObject
            val name = obj["name"]?.jsonPrimitive?.content ?: continue
            val urlMatchStr = obj["url_match"]?.jsonPrimitive?.content ?: continue
            val urlMatch = try {
                Regex(urlMatchStr)
            } catch (_: Exception) { continue }

            val mediaPatterns = mutableListOf<Regex>()
            for (pattern in obj["media_patterns"]?.jsonArray ?: emptyList()) {
                try {
                    mediaPatterns.add(Regex(pattern.jsonPrimitive.content))
                } catch (_: Exception) {}
            }

            val headers = mutableMapOf<String, String>()
            for ((key, value) in obj["headers"]?.jsonObject ?: emptyMap()) {
                headers[key] = value.jsonPrimitive.content
            }

            val cookiesFrom = obj["cookies_from"]?.jsonPrimitive?.content ?: ""

            rules.add(PatternRule(name, urlMatch, mediaPatterns, headers, cookiesFrom))
        }
    }
}
