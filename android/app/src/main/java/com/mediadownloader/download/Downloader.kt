package com.mediadownloader.download

import android.content.Context
import com.mediadownloader.extractors.DownloadTask
import kotlinx.coroutines.*
import okhttp3.OkHttpClient
import okhttp3.Request
import java.io.File
import java.io.FileOutputStream
import java.util.concurrent.TimeUnit

class Downloader(
    private val context: Context? = null
) {
    private val client = OkHttpClient.Builder()
        .connectTimeout(20, TimeUnit.SECONDS)
        .readTimeout(0, TimeUnit.SECONDS)
        .followRedirects(true)
        .followSslRedirects(true)
        .build()

    suspend fun download(
        task: DownloadTask,
        outputFile: String,
        maxRetries: Int = 3,
        showProgress: Boolean = true,
        onProgress: ((ProgressInfo) -> Unit)? = null
    ): Boolean = withContext(Dispatchers.IO) {
        val finalName = outputFile.ifEmpty {
            extractFilenameFromUrl(task.url)
        }

        for (attempt in 1..maxRetries) {
            if (attempt > 1) {
                System.err.println("Retry $attempt/$maxRetries for ${task.url}")
                delay(attempt * 1000L)
                val f = File(finalName)
                if (!f.exists() || f.length() == 0L) f.delete()
            }

            val existingSize = if (attempt == 1 && File(finalName).exists())
                File(finalName).length() else 0L

            val reqBuilder = Request.Builder().url(task.url)
                .header("User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36")
                .header("Accept", "image/avif,image/webp,image/apng,image/*,*/*;q=0.8")
                .header("Accept-Language", "en-US,en;q=0.9")
                .header("Sec-Fetch-Dest", "image")

            for ((key, value) in task.headers) {
                reqBuilder.header(key, value)
            }

            if (task.cookies.isNotEmpty()) {
                reqBuilder.header("Cookie", task.cookies)
            }

            val response = client.newCall(reqBuilder.build()).execute()

            if (!response.isSuccessful) {
                System.err.println("HTTP error: ${response.code}")
                response.close()
                continue
            }

            val append = existingSize > 0
            val startTime = System.currentTimeMillis()
            FileOutputStream(finalName, append).use { fos ->
                response.body?.byteStream()?.use { input ->
                    val buffer = ByteArray(8192)
                    var totalRead = existingSize
                    val totalSize = existingSize + (response.body?.contentLength() ?: 0)
                    var bytesRead: Int
                    var lastUpdate = System.currentTimeMillis()
                    var lastBytes = totalRead

                    while (input.read(buffer).also { bytesRead = it } != -1) {
                        fos.write(buffer, 0, bytesRead)
                        totalRead += bytesRead
                        val now = System.currentTimeMillis()
                        if (totalSize > 0 && now - lastUpdate > 250) {
                            val pct = totalRead.toFloat() / totalSize.toFloat()
                            val elapsed = ((now - startTime) / 1000.0).coerceAtLeast(0.1)
                            val speed = ((totalRead - existingSize) / elapsed).toFloat()
                            val speedMbps = speed / (1024f * 1024f)
                            val dlMb = totalRead / (1024.0 * 1024.0)
                            val totMb = totalSize / (1024.0 * 1024.0)
                            val remaining = totalSize - totalRead
                            val eta = if (speed > 0) (remaining / speed).toLong() else -1L

                            if (showProgress) {
                                print("\r[%.1f%%] %.1f MB / %.1f MB  %.1f MB/s".format(
                                    pct * 100, dlMb, totMb, speedMbps))
                            }
                            onProgress?.invoke(ProgressInfo(pct, dlMb.toFloat(), totMb.toFloat(), speedMbps, eta))
                            lastUpdate = now
                        }
                    }
                }
            }

            response.close()

            if (finalName.endsWith(".zip") && task.frameDelays.isNotEmpty()) {
                processUgoiraSafe(finalName, task.frameDelays)
            }

            return@withContext true
        }
        return@withContext false
    }

    private suspend fun processUgoiraSafe(zipFile: String, frameDelays: List<Int>) {
        try {
            val folder = "${zipFile}_frames"
            val mp4File = zipFile.removeSuffix(".zip") + ".mp4"

            File(folder).mkdirs()
            File(mp4File).parentFile?.mkdirs()

            withContext(Dispatchers.IO) {
                val unzipProc = Runtime.getRuntime().exec(arrayOf("unzip", "-o", zipFile, "-d", folder))
                unzipProc.waitFor()
                if (unzipProc.exitValue() != 0) {
                    System.err.println("unzip not available — keeping ZIP as-is")
                    return@withContext
                }
            }

            println("Converting ${frameDelays.size} frames to MP4...")
            val ok = if (context != null) {
                withContext(Dispatchers.Main) {
                    UgoiraEncoder.encode(context!!, folder, frameDelays, mp4File)
                }
            } else {
                false
            }

            if (ok) {
                File(zipFile).delete()
                File(folder).deleteRecursively()
                println("Ugoira done: $mp4File")
            } else {
                System.err.println("Ugoira encoding failed — keeping ZIP + frames in $folder")
            }
        } catch (e: Exception) {
            System.err.println("Ugoira processing failed: ${e.message}")
        }
    }

    fun extractFilenameFromUrl(url: String): String {
        val lastSlash = url.lastIndexOf('/')
        if (lastSlash == -1) return "downloaded_file"
        var name = url.substring(lastSlash + 1)
        val qpos = name.indexOf('?')
        if (qpos != -1) name = name.substring(0, qpos)
        return name.ifEmpty { "downloaded_file" }
    }

    fun sanitizeFilename(name: String): String {
        val invalid = "\\/:*?\"<>|$`;&|(){}!'#\n\r\t"
        var safe = name.map { c ->
            if (c.code < 0x20 || c in invalid) '_' else c
        }.joinToString("")
        if (safe.isEmpty() || safe == "." || safe == "..") safe = "downloaded_file"
        return safe
    }
}
