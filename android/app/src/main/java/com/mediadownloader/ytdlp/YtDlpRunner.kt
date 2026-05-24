package com.mediadownloader.ytdlp

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

object YtDlpRunner {

    suspend fun download(
        url: String,
        formatId: String = "bestvideo+bestaudio/best",
        outputDir: String = ".",
        onProgress: ((Float) -> Unit)? = null
    ): Boolean = withContext(Dispatchers.IO) {
        try {
            val outputTemplate = if (outputDir == ".") {
                "%(title)s.%(ext)s"
            } else {
                "$outputDir/%(title)s.%(ext)s"
            }

            val py = com.chaquo.python.Python.getInstance()
            val ytdlp = py.getModule("yt_dlp")
            ytdlp.callAttr(
                "main", listOf(
                    "-f", formatId,
                    "-o", outputTemplate,
                    "--no-progress",
                    url
                )
            )
            true
        } catch (e: com.chaquo.python.PyException) {
            System.err.println("yt-dlp error: ${e.message}")
            false
        } catch (e: Exception) {
            System.err.println("Python error: ${e.message}")
            false
        }
    }
}
