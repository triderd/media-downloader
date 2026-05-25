package com.mediadownloader.ui

import android.app.Application
import android.media.MediaScannerConnection
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.mediadownloader.config.Config
import com.mediadownloader.download.Downloader
import com.mediadownloader.download.ProgressInfo
import com.mediadownloader.extractors.DownloadTask
import com.mediadownloader.extractors.ExtractorManager
import com.mediadownloader.ytdlp.YtDlpRunner
import com.mediadownloader.service.DownloadService
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

enum class DownloadStatus {
    QUEUED, EXTRACTING, DOWNLOADING, COMPLETED, FAILED
}

data class DownloadItem(
    val id: Long = System.currentTimeMillis(),
    val url: String,
    val filename: String = "",
    val status: DownloadStatus = DownloadStatus.QUEUED,
    val progress: Float = 0f,
    val downloadedMb: Float = 0f,
    val totalMb: Float = 0f,
    val speedMbps: Float = 0f,
    val etaSeconds: Long = -1L,
    val title: String = "",
    val thumbnail: String = "",
    val error: String = ""
)

class DownloadViewModel(application: Application) : AndroidViewModel(application) {
    private val extractorManager = ExtractorManager()

    private val _items = MutableStateFlow<List<DownloadItem>>(emptyList())
    val items: StateFlow<List<DownloadItem>> = _items.asStateFlow()

    private val _inputUrl = MutableStateFlow("")
    val inputUrl: StateFlow<String> = _inputUrl.asStateFlow()

    private val _darkTheme = MutableStateFlow(true)
    val darkTheme: StateFlow<Boolean> = _darkTheme.asStateFlow()

    private val _logLines = MutableStateFlow<List<String>>(emptyList())
    val logLines: StateFlow<List<String>> = _logLines.asStateFlow()

    private val _showLogs = MutableStateFlow(false)
    val showLogs: StateFlow<Boolean> = _showLogs.asStateFlow()

    fun setInputUrl(url: String) { _inputUrl.value = url }

    fun toggleDarkTheme() { _darkTheme.value = !_darkTheme.value }
    fun toggleLogs() { _showLogs.value = !_showLogs.value }

    private fun addLog(msg: String) {
        val ts = java.text.SimpleDateFormat("HH:mm:ss", java.util.Locale.getDefault())
            .format(java.util.Date())
        _logLines.value = listOf("$ts  $msg") + _logLines.value.take(99)
    }

    fun retryFailed(item: DownloadItem) {
        _inputUrl.value = item.url
        startDownload(item.url)
    }

    fun startDownload(url: String) {
        addLog("START: $url")
        val item = DownloadItem(url = url, status = DownloadStatus.QUEUED)
        _items.value = listOf(item) + _items.value

        viewModelScope.launch {
            processItem(item)
        }
    }

    fun startBackgroundDownload(url: String) {
        DownloadService.start(getApplication(), listOf(url))
    }

    fun startCurrentDownload() {
        val raw = _inputUrl.value.trim()
        if (raw.isEmpty()) return

        val urls = raw.split("\\s+".toRegex()).filter { it.isNotBlank() }
        for (url in urls) {
            startDownload(url)
        }
        _inputUrl.value = ""
    }

    fun clearCompleted() {
        _items.value = _items.value.filter {
            it.status == DownloadStatus.DOWNLOADING ||
            it.status == DownloadStatus.EXTRACTING ||
            it.status == DownloadStatus.QUEUED
        }
    }

    private suspend fun updateItem(id: Long, update: (DownloadItem) -> DownloadItem) {
        _items.value = _items.value.map { if (it.id == id) update(it) else it }
    }

    private suspend fun processItem(item: DownloadItem) = withContext(Dispatchers.IO) {
        try {
            updateItem(item.id) { it.copy(status = DownloadStatus.EXTRACTING) }

            val isDirectUrl = item.url.endsWith(".jpg") ||
                item.url.endsWith(".png") ||
                item.url.endsWith(".jpeg") ||
                item.url.endsWith(".gif") ||
                item.url.endsWith(".webp") ||
                item.url.endsWith(".mp4") ||
                item.url.endsWith(".webm")

            val tasks = if (isDirectUrl) {
                listOf(DownloadTask(url = item.url))
            } else {
                extractorManager.extract(item.url)
            }

            if (tasks.isEmpty()) {
                addLog("FAIL: No media found for ${item.url}")
                updateItem(item.id) {
                    it.copy(status = DownloadStatus.FAILED, error = "No media found")
                }
                return@withContext
            }

            addLog("FOUND: ${tasks.size} files for ${item.url}")
            val downloader = Downloader(getApplication())
            val downloadDir = Config.resolveDownloadDir(getApplication())
            var allOk = true
            var lastError = ""

            for (task in tasks) {
                val filename = if (task.filename.isNotEmpty()) {
                    "$downloadDir/${downloader.sanitizeFilename(task.filename)}"
                } else {
                    "$downloadDir/${downloader.extractFilenameFromUrl(task.url)}"
                }

                val dir = java.io.File(filename).parentFile
                if (dir != null && !dir.exists()) dir.mkdirs()

                updateItem(item.id) {
                    it.copy(status = DownloadStatus.DOWNLOADING, filename = task.filename, progress = 0f)
                }

                if (task.useYtdlp) {
                    lastError = ""
                    updateItem(item.id) { it.copy(progress = -1f, downloadedMb = 0f, totalMb = 0f, speedMbps = 0f, etaSeconds = -1L) }
                    val result = YtDlpRunner.download(
                        url = task.url,
                        formatId = task.formatId,
                        outputDir = downloadDir,
                        onProgress = { info ->
                            viewModelScope.launch {
                                updateItem(item.id) {
                                    it.copy(
                                        progress = if (info.total > 0) info.pct else -1f,
                                        downloadedMb = info.downloaded / (1024f * 1024f),
                                        totalMb = info.total / (1024f * 1024f),
                                        speedMbps = info.speed / (1024f * 1024f),
                                        etaSeconds = info.eta,
                                        title = if (info.title.isNotEmpty()) info.title else it.title,
                                        thumbnail = if (info.thumbnail.isNotEmpty()) info.thumbnail else it.thumbnail
                                    )
                                }
                            }
                        }
                    )
                    if (!result.success) {
                        allOk = false
                        lastError = result.error.ifEmpty { "yt-dlp failed" }
                    } else {
                        updateItem(item.id) { it.copy(progress = 1f) }
                    }
                } else {
                    val ok = downloader.download(
                        task = task,
                        outputFile = filename,
                        showProgress = false,
                        onProgress = { info ->
                            viewModelScope.launch {
                                updateItem(item.id) {
                                    it.copy(
                                        progress = info.progress,
                                        downloadedMb = info.downloadedMb,
                                        totalMb = info.totalMb,
                                        speedMbps = info.speedMbps,
                                        etaSeconds = info.etaSeconds
                                    )
                                }
                            }
                        }
                    )
                    if (!ok) {
                        allOk = false
                        lastError = "Download failed"
                    }
                }
            }

            updateItem(item.id) {
                if (allOk) {
                    MediaScannerConnection.scanFile(
                        getApplication(),
                        arrayOf(downloadDir),
                        null, null
                    )
                    it.copy(status = DownloadStatus.COMPLETED, progress = 1f)
                } else it.copy(status = DownloadStatus.FAILED, error = lastError)
            }
            addLog(if (allOk) "DONE: ${item.url}" else "FAIL: ${item.url} — $lastError")
        } catch (e: Exception) {
            addLog("CRASH: ${item.url} — ${e.message}")
            updateItem(item.id) {
                it.copy(status = DownloadStatus.FAILED, error = e.message ?: "Unknown error")
            }
        }
    }
}
