package com.mediadownloader.service

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat
import com.mediadownloader.MainActivity
import com.mediadownloader.config.Config
import com.mediadownloader.download.Downloader
import com.mediadownloader.extractors.DownloadTask
import com.mediadownloader.extractors.ExtractorManager
import com.mediadownloader.ytdlp.YtDlpRunner
import kotlinx.coroutines.*

class DownloadService : Service() {

    private val serviceScope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private lateinit var notificationManager: NotificationManager

    override fun onCreate() {
        super.onCreate()
        notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        createNotificationChannel()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        val urls = intent?.getStringArrayListExtra(EXTRA_URLS) ?: emptyList()
        if (urls.isEmpty()) {
            stopSelf()
            return START_NOT_STICKY
        }

        val notification = buildNotification("Preparing...", false)
        startForeground(NOTIFICATION_ID, notification)

        serviceScope.launch {
            processUrls(urls)
            stopForeground(STOP_FOREGROUND_REMOVE)
            stopSelf()
        }

        return START_NOT_STICKY
    }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onDestroy() {
        super.onDestroy()
        serviceScope.cancel()
    }

    private suspend fun processUrls(urls: List<String>) {
        val downloader = Downloader()
        val extractorManager = ExtractorManager()
        val downloadDir = Config.resolveDownloadDir(this)
        var completed = 0
        var failed = 0

        for (url in urls) {
            updateNotification(
                title = "Media Downloader",
                content = "Extracting (${urls.indexOf(url) + 1}/${urls.size})...",
                progress = null
            )

            val isDirectUrl = url.endsWith(".jpg") || url.endsWith(".png") ||
                url.endsWith(".jpeg") || url.endsWith(".gif") ||
                url.endsWith(".webp") || url.endsWith(".mp4") || url.endsWith(".webm")

            val tasks = if (isDirectUrl) {
                listOf(DownloadTask(url = url))
            } else {
                try {
                    extractorManager.extract(url)
                } catch (e: Exception) {
                    failed++
                    continue
                }
            }

            for (task in tasks) {
                val filename = if (task.filename.isNotEmpty()) {
                    "$downloadDir/${downloader.sanitizeFilename(task.filename)}"
                } else {
                    "$downloadDir/${downloader.extractFilenameFromUrl(task.url)}"
                }

                val dir = java.io.File(filename).parentFile
                if (dir != null && !dir.exists()) dir.mkdirs()

                updateNotification(
                    title = "Downloading",
                    content = task.filename.ifEmpty { task.url },
                    progress = null
                )

                val ok = if (task.useYtdlp) {
                    YtDlpRunner.download(
                        url = task.url,
                        formatId = task.formatId,
                        outputDir = downloadDir,
                        onProgress = { info ->
                            updateNotification(
                                title = "Downloading",
                                content = task.filename.ifEmpty { task.url },
                                progress = info.pct
                            )

                        }
                    ).success
                } else {
                    downloader.download(
                        task = task,
                        outputFile = filename,
                        showProgress = false,
                        onProgress = { info ->
                            updateNotification(
                                title = "Downloading",
                                content = task.filename.ifEmpty { task.url },
                                progress = info.progress
                            )
                        }
                    )
                }

                if (ok) completed++ else failed++
            }
        }

        showFinalNotification(completed, failed)
    }

    private fun buildNotification(
        content: String,
        showProgress: Boolean,
        progressValue: Int = 0
    ): android.app.Notification {
        val openIntent = PendingIntent.getActivity(
            this,
            0,
            Intent(this, MainActivity::class.java),
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )

        val builder = NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle("Media Downloader")
            .setContentText(content)
            .setSmallIcon(android.R.drawable.stat_sys_download)
            .setOngoing(true)
            .setContentIntent(openIntent)

        if (showProgress) {
            builder.setProgress(100, progressValue, false)
        } else {
            builder.setProgress(0, 0, true)
        }

        return builder.build()
    }

    private fun updateNotification(
        title: String,
        content: String,
        progress: Float?
    ) {
        val openIntent = PendingIntent.getActivity(
            this,
            0,
            Intent(this, MainActivity::class.java),
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )

        val builder = NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle(title)
            .setContentText(content)
            .setSmallIcon(android.R.drawable.stat_sys_download)
            .setOngoing(true)
            .setContentIntent(openIntent)

        if (progress != null) {
            builder.setProgress(100, (progress * 100).toInt(), false)
        } else {
            builder.setProgress(0, 0, true)
        }

        notificationManager.notify(NOTIFICATION_ID, builder.build())
    }

    private fun showFinalNotification(completed: Int, failed: Int) {
        val openIntent = PendingIntent.getActivity(
            this,
            0,
            Intent(this, MainActivity::class.java),
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )

        val message = when {
            failed == 0 -> "$completed file(s) downloaded"
            completed == 0 -> "All $failed download(s) failed"
            else -> "$completed downloaded, $failed failed"
        }

        val builder = NotificationCompat.Builder(this, COMPLETED_CHANNEL_ID)
            .setContentTitle("Downloads finished")
            .setContentText(message)
            .setSmallIcon(android.R.drawable.stat_sys_download_done)
            .setAutoCancel(true)
            .setContentIntent(openIntent)

        notificationManager.notify(COMPLETED_NOTIFICATION_ID, builder.build())
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                CHANNEL_ID,
                "Downloads",
                NotificationManager.IMPORTANCE_LOW
            )
            notificationManager.createNotificationChannel(channel)

            val completedChannel = NotificationChannel(
                COMPLETED_CHANNEL_ID,
                "Download Results",
                NotificationManager.IMPORTANCE_DEFAULT
            )
            notificationManager.createNotificationChannel(completedChannel)
        }
    }

    companion object {
        const val CHANNEL_ID = "downloads"
        const val COMPLETED_CHANNEL_ID = "download_results"
        const val NOTIFICATION_ID = 1001
        const val COMPLETED_NOTIFICATION_ID = 1002
        const val EXTRA_URLS = "extra_urls"

        fun start(context: Context, urls: List<String>) {
            val intent = Intent(context, DownloadService::class.java).apply {
                putStringArrayListExtra(EXTRA_URLS, ArrayList(urls))
            }
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                context.startForegroundService(intent)
            } else {
                context.startService(intent)
            }
        }
    }
}
