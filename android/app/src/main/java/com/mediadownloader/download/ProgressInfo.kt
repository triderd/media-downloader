package com.mediadownloader.download

data class ProgressInfo(
    val progress: Float,
    val downloadedMb: Float,
    val totalMb: Float,
    val speedMbps: Float,
    val etaSeconds: Long
)
