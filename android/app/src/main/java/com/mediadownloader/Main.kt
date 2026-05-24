package com.mediadownloader

import com.mediadownloader.config.Config
import com.mediadownloader.download.Downloader
import com.mediadownloader.extractors.DownloadTask
import com.mediadownloader.extractors.ExtractorManager
import com.mediadownloader.ytdlp.YtDlpRunner
import kotlinx.coroutines.async
import kotlinx.coroutines.awaitAll
import kotlinx.coroutines.coroutineScope

suspend fun main(args: Array<String>) {
    Config.load()

    var urls = mutableListOf<String>()
    var i = 0
    while (i < args.size) {
        when (args[i]) {
            "-h", "--help" -> { printUsage(); return }
            "-f" -> {
                if (i + 1 < args.size) {
                    i++
                    urls.addAll(readUrlsFromFile(args[i]))
                } else {
                    System.err.println("Missing filename after -f")
                    return
                }
            }
            else -> urls.add(args[i])
        }
        i++
    }

    if (urls.isEmpty() && args.isEmpty()) {
        print("Page URL: ")
        val line = readlnOrNull()?.trim() ?: return
        if (line.isNotEmpty()) urls.add(line)
    }

    if (urls.isEmpty()) {
        System.err.println("No URLs provided")
        printUsage()
        return
    }

    println("\u001B[1;36mMedia Downloader\u001B[0m\n")

    var totalOk = 0
    var totalFail = 0

    for ((idx, url) in urls.withIndex()) {
        if (urls.size > 1) {
            println("\u001B[90m------------------------------------------------------------\u001B[0m")
            println("URL ${idx + 1}/${urls.size}: $url")
        }
        val (ok, fail) = processUrl(url)
        totalOk += ok
        totalFail += fail
    }

    if (urls.size > 1 || totalOk + totalFail > 1) {
        println("\n\u001B[1;32mTotal: $totalOk completed, $totalFail failed\u001B[0m")
    }
}

suspend fun processUrl(url: String): Pair<Int, Int> {
    val manager = ExtractorManager()
    val downloader = Downloader()

    val isDirectUrl = url.endsWith(".jpg") || url.endsWith(".png") || url.endsWith(".jpeg") ||
        url.endsWith(".gif") || url.endsWith(".webp") || url.endsWith(".mp4") || url.endsWith(".webm")

    if (isDirectUrl) {
        val filename = url.substringAfterLast('/').let {
            it.substringBefore('?').ifEmpty { "downloaded_file" }
        }
        val ok = downloader.download(
            DownloadTask(url = url, filename = filename),
            filename
        )
        return if (ok) Pair(1, 0) else Pair(0, 1)
    }

    println("\u001B[1;33mExtracting media URLs...\u001B[0m")
    val tasks = manager.extract(url)

    if (tasks.isEmpty()) {
        println("No media found")
        return Pair(0, 1)
    }

    val isPlaylist = tasks.size > 1
    val baseDir = Config.getDownloadDir()
    val downloadFolder = if (isPlaylist) {
        val folder = "$baseDir/${downloader.sanitizeFilename(tasks[0].playlistTitle)}"
        java.io.File(folder).mkdirs()
        folder
    } else ""

    println("\u001B[1;32mFound ${tasks.size} media files\u001B[0m")

    var completed = 0
    var failed = 0
    val concurrent = Config.getConcurrentDownloads()

    if (concurrent == 1 || tasks.size == 1) {
        for (task in tasks) {
            println("\u001B[90m------------------------------------------------------------\u001B[0m")
            println(task.url)

            val filename = if (downloadFolder.isEmpty())
                downloader.sanitizeFilename(task.filename)
            else
                "$downloadFolder/${downloader.sanitizeFilename(task.filename)}"

            val ok = if (task.useYtdlp) {
                println("Running yt-dlp...")
                YtDlpRunner.download(
                    url = task.url,
                    formatId = task.formatId,
                    outputDir = if (downloadFolder.isEmpty()) Config.getDownloadDir() else downloadFolder
                )
            } else {
                downloader.download(task, filename)
            }

            if (ok) completed++ else {
                failed++
                println("\u001B[1;31mFailed: ${task.url}\u001B[0m")
            }
        }
    } else {
        val results = coroutineScope {
            tasks.map { task ->
                async {
                    val localDl = Downloader()
                    val filename = if (downloadFolder.isEmpty())
                        localDl.sanitizeFilename(task.filename)
                    else
                        "$downloadFolder/${localDl.sanitizeFilename(task.filename)}"

                    println("\u001B[90mDownloading: ${task.url}\u001B[0m")
                    val ok = if (task.useYtdlp) {
                        YtDlpRunner.download(
                            url = task.url,
                            formatId = task.formatId,
                            outputDir = if (downloadFolder.isEmpty()) Config.getDownloadDir() else downloadFolder
                        )
                    } else {
                        localDl.download(task, filename, showProgress = false)
                    }
                    if (!ok) println("\u001B[1;31mFailed: ${task.url}\u001B[0m")
                    ok
                }
            }.awaitAll()
        }
        for (ok in results) {
            if (ok) completed++ else failed++
        }
    }

    return Pair(completed, failed)
}

fun readUrlsFromFile(path: String): List<String> {
    val file = java.io.File(path)
    if (!file.exists()) return emptyList()
    return file.readLines().map { it.trim() }.filter { it.isNotEmpty() && !it.startsWith('#') }
}

fun printUsage() {
    println("Media Downloader")
    println()
    println("Usage:")
    println("  mdw                  Interactive mode")
    println("  mdw <url>            Download a single URL")
    println("  mdw <url1> <url2>... Download multiple URLs")
    println("  mdw -f <file>        Read URLs from file")
    println("  mdw -h, --help       Show this help")
}
