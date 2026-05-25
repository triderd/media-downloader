package com.mediadownloader.ui

import android.content.Intent
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Clear
import androidx.compose.material.icons.filled.Download
import androidx.compose.material.icons.filled.ContentPaste
import androidx.compose.material.icons.filled.ContentCopy
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.RectangleShape
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalClipboardManager
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.AnnotatedString
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import coil3.compose.AsyncImage
import com.mediadownloader.ui.auth.WebViewActivity
import java.io.File

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MainScreen(viewModel: DownloadViewModel) {
    val items by viewModel.items.collectAsState()
    val inputUrl by viewModel.inputUrl.collectAsState()
    val darkTheme by viewModel.darkTheme.collectAsState()
    val clipboard = LocalClipboardManager.current
    val context = LocalContext.current
    var showMenu by remember { mutableStateOf(false) }

    val importCookieLauncher = rememberLauncherForActivityResult(
        ActivityResultContracts.OpenDocument()
    ) { uri ->
        if (uri != null) {
            try {
                val name = uri.lastPathSegment ?: "imported.txt"
                val content = context.contentResolver.openInputStream(uri)?.bufferedReader()?.readText() ?: ""
                val cookieDir = File(context.filesDir, "cookies")
                if (!cookieDir.exists()) cookieDir.mkdirs()
                File(cookieDir, name).writeText(content)
            } catch (_: Exception) {}
        }
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Media Downloader") },
                actions = {
                    if (items.any { it.status == DownloadStatus.COMPLETED || it.status == DownloadStatus.FAILED }) {
                        IconButton(onClick = { viewModel.clearCompleted() }) {
                            Icon(Icons.Default.Clear, contentDescription = "Clear")
                        }
                    }
                    IconButton(onClick = { viewModel.toggleDarkTheme() }) {
                        Text(if (darkTheme) "☀️" else "🌙", style = MaterialTheme.typography.titleMedium)
                    }
                    Box {
                        IconButton(onClick = { showMenu = true }) {
                            Icon(Icons.Default.MoreVert, contentDescription = "Menu")
                        }
                        DropdownMenu(
                            expanded = showMenu,
                            onDismissRequest = { showMenu = false }
                        ) {
                            DropdownMenuItem(
                                text = { Text("Login via WebView") },
                                onClick = {
                                    showMenu = false
                                    context.startActivity(Intent(context, WebViewActivity::class.java))
                                }
                            )
                            DropdownMenuItem(
                                text = { Text("Import cookies") },
                                onClick = {
                                    showMenu = false
                                    importCookieLauncher.launch(arrayOf("*/*"))
                                }
                            )
                        }
                    }
                }
            )
        }
    ) { padding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
        ) {
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(12.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = inputUrl,
                    onValueChange = { viewModel.setInputUrl(it) },
                    modifier = Modifier.weight(1f),
                    placeholder = { Text("Paste URL(s) — space-separated") },
                    singleLine = true,
                    keyboardOptions = KeyboardOptions(imeAction = ImeAction.Go),
                    keyboardActions = KeyboardActions(onGo = { viewModel.startCurrentDownload() })
                )

                Spacer(modifier = Modifier.width(8.dp))

                FilledIconButton(onClick = {
                    val text = clipboard.getText()?.text ?: ""
                    if (text.isNotEmpty()) viewModel.setInputUrl(text)
                }) {
                    Icon(Icons.Default.ContentPaste, contentDescription = "Paste")
                }
            }

            Button(
                onClick = { viewModel.startCurrentDownload() },
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 12.dp),
                enabled = inputUrl.isNotBlank()
            ) {
                Icon(Icons.Default.Download, contentDescription = null)
                Spacer(modifier = Modifier.width(8.dp))
                Text("Download")
            }

            if (items.isEmpty()) {
                Box(
                    modifier = Modifier
                        .fillMaxSize()
                        .weight(1f),
                    contentAlignment = Alignment.Center
                ) {
                    Text(
                        "Enter a URL to start downloading",
                        style = MaterialTheme.typography.bodyLarge,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            } else {
                LazyColumn(
                    modifier = Modifier.weight(1f),
                    contentPadding = PaddingValues(12.dp),
                    verticalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    items(items, key = { it.id }) { item ->
                        DownloadItemCard(item)
                    }
                }
            }
        }
    }
}

@Composable
fun StatusLabel(item: DownloadItem) {
    val statusColor = when (item.status) {
        DownloadStatus.COMPLETED -> MaterialTheme.colorScheme.primary
        DownloadStatus.FAILED -> MaterialTheme.colorScheme.error
        DownloadStatus.DOWNLOADING -> MaterialTheme.colorScheme.tertiary
        else -> MaterialTheme.colorScheme.onSurfaceVariant
    }
    val statusText = when (item.status) {
        DownloadStatus.QUEUED -> "Queued"
        DownloadStatus.EXTRACTING -> "Extracting"
        DownloadStatus.DOWNLOADING -> "Downloading"
        DownloadStatus.COMPLETED -> "Done"
        DownloadStatus.FAILED -> "Failed"
    }
    Text(
        text = statusText,
        style = MaterialTheme.typography.labelSmall,
        color = statusColor
    )
}

@Composable
fun DownloadItemCard(item: DownloadItem) {
    Card(modifier = Modifier.fillMaxWidth()) {
        Column(modifier = Modifier.padding(12.dp)) {
            if (item.thumbnail.isNotEmpty()) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    verticalAlignment = Alignment.Top
                ) {
                    AsyncImage(
                        model = item.thumbnail,
                        contentDescription = null,
                        modifier = Modifier
                            .size(72.dp)
                            .clip(RectangleShape),
                        contentScale = ContentScale.Crop
                    )
                    Spacer(modifier = Modifier.width(8.dp))
                    Column(modifier = Modifier.weight(1f)) {
                        if (item.title.isNotEmpty()) {
                            Text(
                                text = item.title,
                                style = MaterialTheme.typography.bodyMedium,
                                fontWeight = FontWeight.Medium,
                                maxLines = 2,
                                overflow = TextOverflow.Ellipsis
                            )
                            Spacer(modifier = Modifier.height(2.dp))
                        }
                        Text(
                            text = item.url,
                            style = MaterialTheme.typography.bodySmall,
                            fontFamily = FontFamily.Monospace,
                            maxLines = 2,
                            overflow = TextOverflow.Ellipsis
                        )
                    }
                }
                Spacer(modifier = Modifier.height(4.dp))
                StatusLabel(item)
            } else {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(
                        text = item.url,
                        modifier = Modifier.weight(1f),
                        style = MaterialTheme.typography.bodySmall,
                        fontFamily = FontFamily.Monospace,
                        maxLines = 2,
                        overflow = TextOverflow.Ellipsis
                    )
                    Spacer(modifier = Modifier.width(8.dp))
                    StatusLabel(item)
                }
            }

            if (item.status == DownloadStatus.DOWNLOADING || item.status == DownloadStatus.COMPLETED) {
                Spacer(modifier = Modifier.height(8.dp))
                if (item.progress < 0f) {
                    LinearProgressIndicator(modifier = Modifier.fillMaxWidth())
                } else {
                    LinearProgressIndicator(
                        progress = { item.progress },
                        modifier = Modifier.fillMaxWidth()
                    )
                }
                if (item.progress > 0f) {
                    Spacer(modifier = Modifier.height(4.dp))
                    val dlText = if (item.totalMb > 0f) {
                        "%.1f / %.1f MB".format(item.downloadedMb, item.totalMb)
                    } else {
                        "%.1f MB".format(item.downloadedMb)
                    }
                    val speedText = if (item.speedMbps > 0f) {
                        "  •  %.1f MB/s".format(item.speedMbps)
                    } else ""
                    val etaText = if (item.etaSeconds > 0L) {
                        val min = item.etaSeconds / 60
                        val sec = item.etaSeconds % 60
                        "  •  %d:%02d left".format(min, sec)
                    } else ""
                    Text(
                        text = "${(item.progress * 100).toInt()}%  —  $dlText$speedText$etaText",
                        style = MaterialTheme.typography.labelSmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }

            if (item.error.isNotEmpty()) {
                Spacer(modifier = Modifier.height(4.dp))
                Text(
                    text = item.error,
                    style = MaterialTheme.typography.bodySmall.copy(
                        fontFamily = FontFamily.Monospace
                    ),
                    color = MaterialTheme.colorScheme.error,
                    maxLines = 20,
                    overflow = TextOverflow.Clip
                )
                Spacer(modifier = Modifier.height(4.dp))
                Row(horizontalArrangement = Arrangement.End, modifier = Modifier.fillMaxWidth()) {
                    val clipboard = LocalClipboardManager.current
                    TextButton(onClick = {
                        clipboard.setText(AnnotatedString(item.error))
                    }) {
                        Icon(Icons.Default.ContentCopy, contentDescription = null, modifier = Modifier.size(16.dp))
                        Spacer(modifier = Modifier.width(4.dp))
                        Text("Copy error", style = MaterialTheme.typography.labelSmall)
                    }
                }
            }

            if (item.filename.isNotEmpty() && item.status == DownloadStatus.COMPLETED) {
                Spacer(modifier = Modifier.height(4.dp))
                Text(
                    text = "Saved: ${item.filename}",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        }
    }
}
