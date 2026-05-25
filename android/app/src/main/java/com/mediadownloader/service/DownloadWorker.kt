package com.mediadownloader.service

import android.content.Context
import androidx.work.*
import com.mediadownloader.config.Config
import com.mediadownloader.download.Downloader
import com.mediadownloader.extractors.ExtractorManager
import com.mediadownloader.ytdlp.YtDlpRunner
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.util.concurrent.TimeUnit

class DownloadWorker(
    context: Context,
    params: WorkerParameters
) : CoroutineWorker(context, params) {

    override suspend fun doWork(): Result = withContext(Dispatchers.IO) {
        val url = inputData.getString(KEY_URL) ?: return@withContext Result.failure()
        createChannelIfNeeded()
        val downloadDir = Config.resolveDownloadDir(applicationContext)

        setForeground(createForegroundInfo("Preparing..."))

        val isDirectUrl = url.endsWith(".jpg") || url.endsWith(".png") ||
            url.endsWith(".jpeg") || url.endsWith(".gif") ||
            url.endsWith(".webp") || url.endsWith(".mp4") || url.endsWith(".webm")

        val manager = ExtractorManager()
        val tasks = if (isDirectUrl) {
            listOf(com.mediadownloader.extractors.DownloadTask(url = url))
        } else {
            try {
                manager.extract(url)
            } catch (e: Exception) {
                return@withContext Result.failure()
            }
        }

        if (tasks.isEmpty()) return@withContext Result.failure()

        val downloader = Downloader(applicationContext)
        var allOk = true

        for (task in tasks) {
            val filename = if (task.filename.isNotEmpty()) {
                "$downloadDir/${downloader.sanitizeFilename(task.filename)}"
            } else {
                "$downloadDir/${downloader.extractFilenameFromUrl(task.url)}"
            }

            val dir = java.io.File(filename).parentFile
            if (dir != null && !dir.exists()) dir.mkdirs()

            setForeground(createForegroundInfo(task.filename.ifEmpty { task.url }))

            val ok = if (task.useYtdlp) {
                YtDlpRunner.download(
                    url = task.url,
                    formatId = task.formatId,
                    outputDir = downloadDir
                ).success
            } else {
                downloader.download(
                    task = task,
                    outputFile = filename,
                    showProgress = false
                )
            }

            if (!ok) allOk = false
        }

        if (allOk) Result.success() else Result.failure()
    }

    private fun createChannelIfNeeded() {
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
            val nm = applicationContext.getSystemService(Context.NOTIFICATION_SERVICE) as android.app.NotificationManager
            val channel = android.app.NotificationChannel(
                DownloadService.CHANNEL_ID,
                "Downloads",
                android.app.NotificationManager.IMPORTANCE_LOW
            )
            nm.createNotificationChannel(channel)
        }
    }

    private fun createForegroundInfo(content: String): ForegroundInfo {
        val openIntent = android.app.PendingIntent.getActivity(
            applicationContext,
            0,
            android.content.Intent(applicationContext, com.mediadownloader.MainActivity::class.java),
            android.app.PendingIntent.FLAG_UPDATE_CURRENT or android.app.PendingIntent.FLAG_IMMUTABLE
        )

        val notification = androidx.core.app.NotificationCompat.Builder(
            applicationContext,
            DownloadService.CHANNEL_ID
        )
            .setContentTitle("Media Downloader")
            .setContentText(content)
            .setSmallIcon(android.R.drawable.stat_sys_download)
            .setOngoing(true)
            .setContentIntent(openIntent)
            .build()

        return ForegroundInfo(DownloadService.NOTIFICATION_ID, notification)
    }

    companion object {
        const val KEY_URL = "url"

        fun enqueue(context: Context, url: String) {
            val constraints = Constraints.Builder()
                .setRequiredNetworkType(NetworkType.CONNECTED)
                .build()

            val inputData = Data.Builder()
                .putString(KEY_URL, url)
                .build()

            val request = OneTimeWorkRequestBuilder<DownloadWorker>()
                .setConstraints(constraints)
                .setInputData(inputData)
                .setBackoffCriteria(
                    BackoffPolicy.EXPONENTIAL,
                    1,
                    TimeUnit.MINUTES
                )
                .build()

            WorkManager.getInstance(context).enqueue(request)
        }
    }
}
