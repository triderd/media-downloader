package com.mediadownloader.download

import android.content.Context
import android.net.Uri
import androidx.media3.common.MediaItem
import androidx.media3.transformer.Composition
import androidx.media3.transformer.EditedMediaItem
import androidx.media3.transformer.EditedMediaItemSequence
import androidx.media3.transformer.ExportException
import androidx.media3.transformer.ExportResult
import androidx.media3.transformer.Transformer
import java.io.File
import java.util.concurrent.CountDownLatch

object UgoiraEncoder {

    fun encode(context: Context, framesDir: String, frameDelays: List<Int>, outputFile: String): Boolean {
        try {
            val frameFiles = File(framesDir).listFiles()
                ?.filter { it.extension.equals("jpg", true) || it.extension.equals("jpeg", true) }
                ?.sortedBy { it.name }
                ?: return false

            if (frameFiles.isEmpty() || frameFiles.size != frameDelays.size) return false

            val items = frameFiles.mapIndexed { index, file ->
                val durationUs = frameDelays[index] * 1000L
                EditedMediaItem.Builder(
                    MediaItem.fromUri(Uri.fromFile(file))
                ).setDurationUs(durationUs).build()
            }

            val composition = Composition.Builder(
                EditedMediaItemSequence(items)
            ).build()

            val transformer = Transformer.Builder(context).build()

            val latch = CountDownLatch(1)
            var success = false

            transformer.addListener(object : Transformer.Listener {
                override fun onCompleted(composition: Composition, result: ExportResult) {
                    success = true
                    latch.countDown()
                }

                override fun onError(
                    composition: Composition,
                    result: ExportResult,
                    exception: ExportException
                ) {
                    latch.countDown()
                }
            })

            transformer.start(composition, outputFile)
            latch.await()
            transformer.cancel()

            return success && File(outputFile).exists() && File(outputFile).length() > 0
        } catch (e: Exception) {
            System.err.println("Ugoira transformer error: ${e.message}")
            return false
        }
    }
}
