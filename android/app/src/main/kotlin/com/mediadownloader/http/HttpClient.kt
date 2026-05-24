package com.mediadownloader.http

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.RequestBody.Companion.toRequestBody
import java.util.concurrent.TimeUnit

object HttpClient {

    private val client = OkHttpClient.Builder()
        .connectTimeout(20, TimeUnit.SECONDS)
        .readTimeout(30, TimeUnit.SECONDS)
        .followRedirects(true)
        .followSslRedirects(true)
        .build()

    suspend fun get(request: HttpRequest): HttpResponse =
        withContext(Dispatchers.IO) {
            val req = buildRequest(request).get().build()
            executeRequest(req)
        }

    suspend fun post(request: HttpRequest, body: String): HttpResponse =
        withContext(Dispatchers.IO) {
            val req = buildRequest(request)
                .post(body.toRequestBody("application/json".toMediaType()))
                .build()
            executeRequest(req)
        }

    private fun buildRequest(request: HttpRequest): Request.Builder {
        val builder = Request.Builder().url(request.url)

        if (request.referer.isNotEmpty()) {
            builder.header("Referer", request.referer)
        }
        if (request.cookies.isNotEmpty()) {
            builder.header("Cookie", request.cookies)
        }
        builder.header("User-Agent", request.userAgent)
        builder.header("Accept-Language", "en-US,en;q=0.9")

        for ((key, value) in request.headers) {
            builder.header(key, value)
        }

        return builder
    }

    private fun executeRequest(req: Request): HttpResponse {
        val response = client.newCall(req).execute()
        val body = response.body?.string() ?: ""
        val headers = mutableMapOf<String, String>()
        for ((name, value) in response.headers) {
            headers[name] = value
        }
        return HttpResponse(
            statusCode = response.code,
            body = body,
            contentType = response.header("Content-Type") ?: "",
            headers = headers
        )
    }
}
