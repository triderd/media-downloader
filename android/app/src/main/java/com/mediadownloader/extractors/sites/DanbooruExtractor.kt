package com.mediadownloader.extractors.sites

import com.mediadownloader.auth.CookieManager
import com.mediadownloader.extractors.BaseExtractor
import com.mediadownloader.extractors.DownloadTask
import com.mediadownloader.http.HttpClient
import com.mediadownloader.http.HttpRequest
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.*

class DanbooruExtractor : BaseExtractor {
    private val json = Json { ignoreUnknownKeys = true }

    override fun matches(url: String): Boolean =
        url.contains("danbooru.donmai.us") ||
        url.contains("gelbooru.com") ||
        url.contains("safebooru.org")

    override suspend fun extract(url: String): List<DownloadTask> {
        val isDanbooru = url.contains("danbooru.donmai.us")
        val isGelbooru = url.contains("gelbooru.com")

        val postId = extractPostId(url)

        if (isGelbooru) {
            return extractGelbooru(postId)
        }

        if (isDanbooru && postId.isEmpty() && url.contains("tags=")) {
            return extractDanbooruSearch(url)
        }

        if (postId.isEmpty()) return emptyList()

        return if (isDanbooru) extractDanbooruPost(postId)
        else emptyList()
    }

    private fun extractPostId(url: String): String {
        for (pattern in listOf(
            Regex("""/posts/(\d+)"""),
            Regex("""[?&]id=(\d+)""")
        )) {
            pattern.find(url)?.let { return it.groupValues[1] }
        }
        return ""
    }

    private suspend fun extractDanbooruPost(postId: String): List<DownloadTask> {
        val apiUrl = "https://danbooru.donmai.us/posts/$postId.json"
        val body = fetchJson(apiUrl, "https://danbooru.donmai.us/", "danbooru")
        if (body.isEmpty()) return emptyList()

        val post = try { json.parseToJsonElement(body).jsonObject } catch (_: Exception) { return emptyList() }
        val fileUrl = getFileUrl(post) ?: return emptyList()

        return listOf(
            DownloadTask(
                url = fileUrl,
                filename = extractFilename(fileUrl, postId),
                headers = mapOf("Referer" to "https://danbooru.donmai.us/"),
                cookies = CookieManager.load("danbooru")
            )
        )
    }

    private suspend fun extractGelbooru(postId: String): List<DownloadTask> {
        val pageUrl = "https://gelbooru.com/index.php?page=post&s=view&id=$postId"
        val resp = HttpClient.get(
            HttpRequest(
                url = pageUrl,
                userAgent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36",
                referer = "https://gelbooru.com/",
                headers = mapOf(
                    "Accept" to "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
                    "Accept-Language" to "en-US,en;q=0.9"
                )
            )
        )
        if (resp.statusCode >= 400) return emptyList()

        val imgPattern = Regex("""<img[^>]*id="image"[^>]*src="([^"]+)"""")
        val match = imgPattern.find(resp.body) ?: return emptyList()
        val fileUrl = match.groupValues[1]

        return listOf(
            DownloadTask(
                url = fileUrl,
                filename = extractFilename(fileUrl, postId),
                headers = mapOf("Referer" to "https://gelbooru.com/")
            )
        )
    }

    private suspend fun extractDanbooruSearch(url: String): List<DownloadTask> {
        val tagsStart = url.indexOf("tags=")
        if (tagsStart == -1) return emptyList()
        var tags = url.substring(tagsStart + 5)
        val amp = tags.indexOf('&')
        if (amp != -1) tags = tags.substring(0, amp)

        val apiUrl = "https://danbooru.donmai.us/posts.json?tags=$tags&limit=200"
        val body = fetchJson(apiUrl, "https://danbooru.donmai.us/", "danbooru")
        if (body.isEmpty()) return emptyList()

        val posts = try {
            json.parseToJsonElement(body).jsonArray
        } catch (_: Exception) { return emptyList() }

        val cookies = CookieManager.load("danbooru")
        return posts.mapNotNull { element ->
            val post = element.jsonObject
            val fileUrl = getFileUrl(post) ?: return@mapNotNull null
            DownloadTask(
                url = fileUrl,
                filename = extractFilename(fileUrl, post["id"]?.jsonPrimitive?.content ?: "0"),
                headers = mapOf("Referer" to "https://danbooru.donmai.us/"),
                cookies = cookies
            )
        }
    }

    private suspend fun fetchJson(url: String, referer: String, site: String): String {
        val resp = HttpClient.get(
            HttpRequest(
                url = url,
                userAgent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36",
                referer = referer,
                headers = mapOf(
                    "Accept" to "application/json",
                    "Accept-Language" to "en-US,en;q=0.9"
                ),
                cookies = CookieManager.load(site)
            )
        )
        return if (resp.statusCode < 400) resp.body else ""
    }

    private fun getFileUrl(post: JsonObject): String? {
        val hasLarge = post["has_large"]?.jsonPrimitive?.boolean ?: false

        if (hasLarge) {
            post["large_file_url"]?.jsonPrimitive?.content?.let { return it }
        }

        val md5 = post["md5"]?.jsonPrimitive?.content ?: return null
        val ext = post["file_ext"]?.jsonPrimitive?.content ?: "jpg"

        if (hasLarge) {
            return "https://cdn.donmai.us/sample/${md5.substring(0, 2)}/${md5.substring(2, 4)}/sample-${md5}.jpg"
        }

        val fileUrl = post["file_url"]?.jsonPrimitive?.content
        if (fileUrl != null && fileUrl.contains("/original/")) {
            val tagParts = mutableListOf<String>()
            for (field in listOf("tag_string_character", "tag_string_copyright", "tag_string_artist")) {
                val tagStr = post[field]?.jsonPrimitive?.content ?: ""
                if (tagStr.isNotEmpty()) {
                    tagParts.add(tagStr.replace(' ', '_'))
                }
            }
            if (tagParts.isNotEmpty()) {
                val joined = tagParts.joinToString("_")
                return "https://cdn.donmai.us/original/${md5.substring(0, 2)}/${md5.substring(2, 4)}/__${joined}__${md5}.$ext"
            }
        }

        return fileUrl
    }

    private fun extractFilename(url: String, postId: String): String {
        val lastSlash = url.lastIndexOf('/')
        var filename = if (lastSlash != -1 && lastSlash + 1 < url.length)
            url.substring(lastSlash + 1)
        else
            "danbooru_$postId"

        val qpos = filename.indexOf('?')
        if (qpos != -1) filename = filename.substring(0, qpos)
        return filename
    }
}
