package com.mediadownloader.download

import android.content.Context
import android.net.Uri
import android.util.Log
import androidx.media3.common.MediaItem
import androidx.media3.transformer.Composition
import androidx.media3.transformer.EditedMediaItem
import androidx.media3.transformer.EditedMediaItemSequence
import androidx.media3.transformer.ExportException
import androidx.media3.transformer.ExportResult
import androidx.media3.transformer.Transformer
import java.io.File
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit

object UgoiraEncoder {
    private const val TAG = "UgoiraEncoder"

    fun encode(context: Context, framesDir: String, frameDelays: List<Int>, outputFile: String): Boolean {
        try {
            val frameFiles = File(framesDir).listFiles()
                ?.filter { it.extension.equals("jpg", true) || it.extension.equals("jpeg", true) }
                ?.sortedBy { it.name }
            if (frameFiles == null || frameFiles.isEmpty()) {
                Log.e(TAG, "No frame files in $framesDir")
                return false
            }
            if (frameFiles.size != frameDelays.size) {
                Log.e(TAG, "Frame count mismatch: ${frameFiles.size} vs ${frameDelays.size}")
                return false
            }

            Log.i(TAG, "Building ${frameFiles.size} EditedMediaItems...")
            val items = frameFiles.mapIndexed { index, file ->
                val durationUs = frameDelays[index] * 1000L
                EditedMediaItem.Builder(
                    MediaItem.fromUri(Uri.fromFile(file))
                ).setDurationUs(durationUs).build()
            }

            val sequence = EditedMediaItemSequence(items)
            val composition = Composition.Builder(sequence).build()
            Log.i(TAG, "Composition built")

            val transformer = Transformer.Builder(context).build()
            Log.i(TAG, "Transformer created")

            val latch = CountDownLatch(1)
            var success = false
            var errorMsg = ""

            transformer.addListener(object : Transformer.Listener {
                override fun onCompleted(composition: Composition, result: ExportResult) {
                    Log.i(TAG, "Transform completed, durationMs=${result.durationMs}")
                    success = true
                    latch.countDown()
                }

                override fun onError(
                    composition: Composition,
                    result: ExportResult,
                    exception: ExportException
                ) {
                    errorMsg = exception.message ?: "unknown"
                    Log.e(TAG, "Transform error: $errorMsg, errorCode=${exception.errorCode}")
                    latch.countDown()
                }
            })

            Log.i(TAG, "Starting transform to $outputFile...")
            transformer.start(composition, outputFile)

            val finished = latch.await(60, TimeUnit.SECONDS)
            transformer.cancel()

            if (!finished) {
                Log.e(TAG, "Transform timed out after 60s")
                return false
            }
            if (!success) {
                Log.e(TAG, "Transform failed: $errorMsg")
                return false
            }

            val resultFile = File(outputFile)
            val ok = resultFile.exists() && resultFile.length() > 0
            Log.i(TAG, "Result: exists=${resultFile.exists()}, size=${resultFile.length()}")
            return ok
        } catch (e: Exception) {
            Log.e(TAG, "Ugoira exception: ${e.message}", e)
            return false
        }
    }
}
