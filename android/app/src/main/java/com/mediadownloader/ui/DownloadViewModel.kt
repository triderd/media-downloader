package com.mediadownloader.ui

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.mediadownloader.config.Config
import com.mediadownloader.download.Downloader
import com.mediadownloader.extractors.DownloadTask
import com.mediadownloader.extractors.ExtractorManager
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
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

class DownloadViewModel : ViewModel() {
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

            val tasks = if (item.url.let {
                it.endsWith(".jpg") || it.endsWith(".png") || it.endsWith(".jpeg") ||
                it.endsWith(".gif") || it.endsWith(".webp") || it.endsWith(".mp4") ||
                it.endsWith(".webm")
            }) {
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
            val downloadDir = Config.getDownloadDir()
            var allOk = true

            for (task in tasks) {
                val filename = if (task.filename.isNotEmpty()) {
                    "$downloadDir/${downloader.sanitizeFilename(task.filename)}"
                } else {
                    "$downloadDir/${downloader.extractFilenameFromUrl(task.url)}"
                }

                val dir = java.io.File(filename).parentFile
                if (dir != null && !dir.exists()) dir.mkdirs()

                updateItem(item.id) {
                    it.copy(status = DownloadStatus.DOWNLOADING, filename = task.filename)
                }

                val ok = if (task.useYtdlp) {
                    ytdlpDownload(task)
                } else {
                    downloader.download(task, filename, showProgress = false)
                }

                if (!ok) {
                    allOk = false
                }
            }

            updateItem(item.id) {
                if (allOk) it.copy(status = DownloadStatus.COMPLETED, progress = 1f)
                else it.copy(status = DownloadStatus.FAILED, error = "Download failed")
            }
        } catch (e: Exception) {
            updateItem(item.id) {
                it.copy(status = DownloadStatus.FAILED, error = e.message ?: "Unknown error")
            }
        }
    }

    private fun ytdlpDownload(task: DownloadTask): Boolean {
        val cmd = "yt-dlp -f ${task.formatId} -o \"%(title)s.%(ext)s\" \"${task.url}\""
        val proc = Runtime.getRuntime().exec(arrayOf("sh", "-c", cmd))
        return proc.waitFor() == 0
    }
}
