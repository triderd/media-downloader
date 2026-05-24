package com.mediadownloader.http

data class HttpRequest(
    val url: String,
    val headers: Map<String, String> = emptyMap(),
    val cookies: String = "",
    val referer: String = "",
    val userAgent: String = "Mozilla/5.0 (compatible; media-downloader/1.0)",
    val timeout: Long = 30,
    val followRedirects: Boolean = true
)

data class HttpResponse(
    val statusCode: Int,
    val body: String,
    val contentType: String = "",
    val headers: Map<String, String> = emptyMap()
)
