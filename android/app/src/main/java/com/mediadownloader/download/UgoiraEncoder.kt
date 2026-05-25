package com.mediadownloader.download

import android.graphics.BitmapFactory
import android.media.MediaCodec
import android.media.MediaCodecInfo
import android.media.MediaCodecList
import android.media.MediaFormat
import android.media.MediaMuxer
import java.io.File
import java.io.FileOutputStream
import java.nio.ByteBuffer

object UgoiraEncoder {

    private const val MIME_TYPE = "video/avc"
    private const val FRAME_RATE = 30
    private const val I_FRAME_INTERVAL = 1

    fun encode(framesDir: String, frameDelays: List<Int>, outputFile: String): Boolean {
        try {
            val frameFiles = File(framesDir).listFiles()
                ?.filter { it.extension.equals("jpg", true) || it.extension.equals("jpeg", true) }
                ?.sortedBy { it.name }
                ?: return false

            if (frameFiles.isEmpty() || frameFiles.size != frameDelays.size) return false

            val firstFrame = BitmapFactory.decodeFile(frameFiles[0].absolutePath)
            val width = (firstFrame.width / 16) * 16
            val height = (firstFrame.height / 16) * 16
            firstFrame.recycle()

            if (width <= 0 || height <= 0) return false

            val codec = MediaCodec.createByCodecName(findEncoder())
            val format = MediaFormat.createVideoFormat(MIME_TYPE, width, height).apply {
                setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible)
                setInteger(MediaFormat.KEY_BIT_RATE, 2000000)
                setInteger(MediaFormat.KEY_FRAME_RATE, FRAME_RATE)
                setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, I_FRAME_INTERVAL)
            }

            codec.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE)
            codec.start()

            val muxer = MediaMuxer(outputFile, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4)
            var trackIndex = -1
            var muxerStarted = false
            var presentationTimeUs = 0L

            val bufferInfo = MediaCodec.BufferInfo()
            var eos = false
            var inputDone = false
            var frameIndex = 0

            while (!eos) {
                val inIndex = codec.dequeueInputBuffer(10000)
                if (inIndex >= 0 && !inputDone) {
                    if (frameIndex < frameFiles.size) {
                        val bitmap = BitmapFactory.decodeFile(frameFiles[frameIndex].absolutePath)
                        val scaled = android.graphics.Bitmap.createScaledBitmap(bitmap, width, height, true)
                        bitmap.recycle()

                        val inputBuffer = codec.getInputBuffer(inIndex)!!
                        val yuvData = bitmapToYuv420(scaled, width, height)
                        scaled.recycle()

                        inputBuffer.clear()
                        inputBuffer.put(yuvData)
                        codec.queueInputBuffer(inIndex, 0, yuvData.size, presentationTimeUs, 0)

                        presentationTimeUs += (frameDelays[frameIndex] * 1000L)
                        frameIndex++
                    } else {
                        codec.queueInputBuffer(inIndex, 0, 0, presentationTimeUs, MediaCodec.BUFFER_FLAG_END_OF_STREAM)
                        inputDone = true
                    }
                }

                val outIndex = codec.dequeueOutputBuffer(bufferInfo, 10000)
                when {
                    outIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED -> {
                        if (!muxerStarted) {
                            trackIndex = muxer.addTrack(codec.outputFormat)
                            muxer.start()
                            muxerStarted = true
                        }
                    }
                    outIndex >= 0 -> {
                        val outputBuffer = codec.getOutputBuffer(outIndex)!!
                        if (bufferInfo.flags and MediaCodec.BUFFER_FLAG_CODEC_CONFIG != 0) {
                            bufferInfo.size = 0
                        }
                        if (bufferInfo.size > 0 && muxerStarted) {
                            outputBuffer.position(bufferInfo.offset)
                            outputBuffer.limit(bufferInfo.offset + bufferInfo.size)
                            muxer.writeSampleData(trackIndex, outputBuffer, bufferInfo)
                        }
                        codec.releaseOutputBuffer(outIndex, false)
                        if (bufferInfo.flags and MediaCodec.BUFFER_FLAG_END_OF_STREAM != 0) {
                            eos = true
                        }
                    }
                }
            }

            codec.stop()
            codec.release()
            muxer.stop()
            muxer.release()

            File(outputFile).let { mp4 ->
                if (mp4.exists() && mp4.length() > 0) {
                    return true
                }
            }
            return false
        } catch (e: Exception) {
            System.err.println("Ugoira encoder error: ${e.message}")
            return false
        }
    }

    private fun findEncoder(): String {
        val codecList = MediaCodecList(MediaCodecList.REGULAR_CODECS)
        for (info in codecList.codecInfos) {
            if (!info.isEncoder) continue
            for (type in info.supportedTypes) {
                if (type.equals(MIME_TYPE, true)) {
                    return info.name
                }
            }
        }
        throw RuntimeException("No H.264 encoder found")
    }

    private fun bitmapToYuv420(bitmap: android.graphics.Bitmap, width: Int, height: Int): ByteArray {
        val pixels = IntArray(width * height)
        bitmap.getPixels(pixels, 0, width, 0, 0, width, height)

        val ySize = width * height
        val uvSize = (width / 2) * (height / 2)
        val yuv = ByteArray(ySize + uvSize * 2)

        var yIndex = 0
        var uIndex = ySize
        var vIndex = ySize + uvSize

        for (j in 0 until height) {
            for (i in 0 until width) {
                val pixel = pixels[j * width + i]
                val r = (pixel shr 16) and 0xFF
                val g = (pixel shr 8) and 0xFF
                val b = pixel and 0xFF

                yuv[yIndex++] = ((66 * r + 129 * g + 25 * b + 128) shr 8 + 16).toByte()

                if (j % 2 == 0 && i % 2 == 0) {
                    yuv[uIndex++] = ((-38 * r - 74 * g + 112 * b + 128) shr 8 + 128).toByte()
                    yuv[vIndex++] = ((112 * r - 94 * g - 18 * b + 128) shr 8 + 128).toByte()
                }
            }
        }
        return yuv
    }
}
