package com.mediadownloader.ui

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.mediadownloader.config.Config
import com.mediadownloader.download.Downloader
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
    val error: String = ""
)

class DownloadViewModel(application: Application) : AndroidViewModel(application) {
    private val extractorManager = ExtractorManager()

    private val _items = MutableStateFlow<List<DownloadItem>>(emptyList())
    val items: StateFlow<List<DownloadItem>> = _items.asStateFlow()

    private val _inputUrl = MutableStateFlow("")
    val inputUrl: StateFlow<String> = _inputUrl.asStateFlow()

    fun setInputUrl(url: String) { _inputUrl.value = url }

    fun startDownload(url: String) {
        val item = DownloadItem(url = url, status = DownloadStatus.QUEUED)
        _items.value = _items.value + item

        viewModelScope.launch {
            processItem(item)
        }
    }

    fun startBackgroundDownload(url: String) {
        DownloadService.start(getApplication(), listOf(url))
    }

    fun startCurrentDownload() {
        val url = _inputUrl.value.trim()
        if (url.isNotEmpty()) {
            startDownload(url)
            _inputUrl.value = ""
        }
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
                updateItem(item.id) {
                    it.copy(status = DownloadStatus.FAILED, error = "No media found")
                }
                return@withContext
            }

            val downloader = Downloader()
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
                    val ok = YtDlpRunner.download(
                        url = task.url,
                        formatId = task.formatId,
                        outputDir = downloadDir,
                        onProgress = { pct ->
                            viewModelScope.launch {
                                updateItem(item.id) { it.copy(progress = pct) }
                            }
                        }
                    )
                    if (!ok) {
                        allOk = false
                        lastError = "yt-dlp failed"
                    } else {
                        updateItem(item.id) { it.copy(progress = 1f) }
                    }
                } else {
                    val ok = downloader.download(
                        task = task,
                        outputFile = filename,
                        showProgress = false,
                        onProgress = { pct ->
                            viewModelScope.launch {
                                updateItem(item.id) { it.copy(progress = pct) }
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
                if (allOk) it.copy(status = DownloadStatus.COMPLETED, progress = 1f)
                else it.copy(status = DownloadStatus.FAILED, error = lastError)
            }
        } catch (e: Exception) {
            updateItem(item.id) {
                it.copy(status = DownloadStatus.FAILED, error = e.message ?: "Unknown error")
            }
        }
    }
}
