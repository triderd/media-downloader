package com.mediadownloader.download

import com.mediadownloader.extractors.DownloadTask
import kotlinx.coroutines.*
import okhttp3.OkHttpClient
import okhttp3.Request
import java.io.File
import java.io.FileOutputStream
import java.util.concurrent.TimeUnit

class Downloader {
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
        onProgress: ((Float) -> Unit)? = null
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
            FileOutputStream(finalName, append).use { fos ->
                response.body?.byteStream()?.use { input ->
                    val buffer = ByteArray(8192)
                    var totalRead = existingSize
                    val totalSize = existingSize + (response.body?.contentLength() ?: 0)
                    var bytesRead: Int
                    var lastUpdate = System.currentTimeMillis()

                    while (input.read(buffer).also { bytesRead = it } != -1) {
                        fos.write(buffer, 0, bytesRead)
                        totalRead += bytesRead
                        val now = System.currentTimeMillis()
                        if (totalSize > 0 && now - lastUpdate > 250) {
                            lastUpdate = now
                            val pct = totalRead.toFloat() / totalSize.toFloat()
                            if (showProgress) {
                                val dlMb = totalRead / 1024.0 / 1024.0
                                val totMb = totalSize / 1024.0 / 1024.0
                                print("\r[%.1f%%] %.1f MB / %.1f MB".format(pct * 100, dlMb, totMb))
                            }
                            onProgress?.invoke(pct)
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

    private fun processUgoiraSafe(zipFile: String, frameDelays: List<Int>) {
        try {
            val folder = "${zipFile}_frames"
            val mp4File = zipFile.removeSuffix(".zip") + ".mp4"

            File(folder).mkdirs()
            val unzipProc = Runtime.getRuntime().exec(arrayOf("unzip", "-o", zipFile, "-d", folder))
            unzipProc.waitFor()
            if (unzipProc.exitValue() != 0) {
                System.err.println("unzip not available — keeping ZIP as-is")
                return
            }

            val concatFile = File("$folder/concat.txt")
            concatFile.bufferedWriter().use { writer ->
                writer.write("ffconcat version 1.0\n")
                for (i in frameDelays.indices) {
                    val frameName = "%06d.jpg".format(i)
                    writer.write("file '$folder/$frameName'\n")
                    writer.write("duration ${frameDelays[i] / 1000.0}\n")
                }
            }

            println("Converting to MP4 (${frameDelays.size} frames)...")
            val ffmpegProc = Runtime.getRuntime().exec(arrayOf(
                "ffmpeg", "-y", "-f", "concat", "-safe", "0",
                "-i", "$folder/concat.txt",
                "-c:v", "libx264", "-pix_fmt", "yuv420p",
                "-vf", "fps=60,setpts=PTS-STARTPTS",
                mp4File
            ))
            ffmpegProc.waitFor()
            if (ffmpegProc.exitValue() != 0) {
                System.err.println("ffmpeg not available — keeping ZIP as-is")
            }
        } catch (e: Exception) {
            System.err.println("Ugoira processing failed (unzip/ffmpeg not on device): ${e.message}")
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
