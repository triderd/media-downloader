package com.mediadownloader.ytdlp

import kotlinx.coroutines.*
import java.io.File
import java.util.concurrent.atomic.AtomicInteger

object YtDlpRunner {

    data class YtDlpResult(
        val success: Boolean,
        val error: String = "",
        val output: String = ""
    )

    data class ProgressInfo(
        val pct: Float,
        val downloaded: Long,
        val total: Long,
        val speed: Long,
        val eta: Long
    )

    private val counter = AtomicInteger(0)

    suspend fun download(
        url: String,
        formatId: String = "bestvideo+bestaudio/best",
        outputDir: String = ".",
        onProgress: ((ProgressInfo) -> Unit)? = null
    ): YtDlpResult = withContext(Dispatchers.IO) {
        try {
            val outputTemplate = if (outputDir == ".") {
                "%(title)s.%(ext)s"
            } else {
                "$outputDir/%(title)s.%(ext)s"
            }

            val dir = File("/data/data/com.mediadownloader/files")
            if (!dir.exists()) dir.mkdirs()

            val argsFile = File(dir, "ytdlp_args.json")
            val resultFile = File(dir, "ytdlp_args.json.result")
            val progressFile = File(dir, "ytdlp_args.json.progress")
            resultFile.delete()
            progressFile.delete()

            val safeUrl = url.replace("\\", "\\\\").replace("\"", "\\\"")
            val safeTmpl = outputTemplate.replace("\\", "\\\\").replace("\"", "\\\"")
            argsFile.writeText("""{"url":"$safeUrl","fmt":"$formatId","tmpl":"$safeTmpl"}""")

            val n = counter.getAndIncrement() % 8

            val progressJob = launch(Dispatchers.IO) {
                while (isActive) {
                    try {
                        if (progressFile.exists()) {
                            val text = progressFile.readText()
                            val json = org.json.JSONObject(text)
                            onProgress?.invoke(ProgressInfo(
                                json.optDouble("pct", 0.0).toFloat(),
                                json.optLong("downloaded", 0),
                                json.optLong("total", 0),
                                json.optLong("speed", 0),
                                json.optLong("eta", 0)
                            ))
                        }
                    } catch (_: Exception) {}
                    delay(150)
                }
            }

            val py = com.chaquo.python.Python.getInstance()
            py.getModule("ytdlp_$n")

            progressJob.cancel()

            if (resultFile.exists()) {
                val json = resultFile.readText()
                resultFile.delete()
                val code = org.json.JSONObject(json).optInt("code", 1)
                val err = org.json.JSONObject(json).optString("err", "")
                if (code != 0) YtDlpResult(false, err.ifEmpty { "code=$code" }, err)
                else YtDlpResult(true)
            } else {
                YtDlpResult(false, "No result from Python module")
            }
        } catch (e: com.chaquo.python.PyException) {
            YtDlpResult(false, e.message ?: "PyException")
        } catch (e: Exception) {
            YtDlpResult(false, "${e.javaClass.simpleName}: ${e.message}")
        }
    }
}
