package com.mediadownloader.ui.auth

import android.annotation.SuppressLint
import android.os.Bundle
import android.webkit.CookieManager
import android.webkit.WebView
import android.webkit.WebViewClient
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import java.io.File

class WebViewActivity : ComponentActivity() {

    private val sites = listOf(
        "Pixiv" to "https://www.pixiv.net/login.php",
        "Gelbooru" to "https://gelbooru.com/index.php?page=account&s=login"
    )

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val initialSite = sites.firstOrNull { it.first.equals(intent.getStringExtra("site"), true) }
            ?: sites.first()

        setContent {
            AuthScreen(
                sites = sites,
                initialSite = initialSite,
                onCookiesSaved = { site, _ -> finish() },
                filesDir = filesDir
            )
        }
    }
}

@SuppressLint("SetJavaScriptEnabled")
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun AuthScreen(
    sites: List<Pair<String, String>>,
    initialSite: Pair<String, String>,
    onCookiesSaved: (String, String) -> Unit,
    filesDir: File
) {
    var selectedSite by remember { mutableStateOf(initialSite.first) }
    var webView by remember { mutableStateOf<WebView?>(null) }
    val selectedUrl = sites.first { it.first == selectedSite }.second

    LaunchedEffect(selectedUrl) {
        webView?.loadUrl(selectedUrl)
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Login — $selectedSite") },
                actions = {
                    TextButton(onClick = {
                        val cookies = CookieManager.getInstance().getCookie(selectedUrl)
                        if (!cookies.isNullOrEmpty()) {
                            saveCookies(filesDir, selectedSite.lowercase(), cookies)
                        }
                        onCookiesSaved(selectedSite, cookies ?: "")
                    }) {
                        Text("Save")
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
                    .padding(horizontal = 12.dp, vertical = 8.dp),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                for ((name, _) in sites) {
                    FilterChip(
                        selected = selectedSite == name,
                        onClick = { selectedSite = name },
                        label = { Text(name) }
                    )
                }
            }

            HorizontalDivider()

            AndroidView(
                factory = { ctx ->
                    WebView(ctx).apply {
                        settings.javaScriptEnabled = true
                        settings.domStorageEnabled = true
                        webViewClient = WebViewClient()
                        loadUrl(selectedUrl)
                    }.also { webView = it }
                },
                modifier = Modifier.weight(1f)
            )
        }
    }
}

private fun saveCookies(filesDir: File, site: String, cookies: String) {
    val cookieDir = File(filesDir, "cookies")
    if (!cookieDir.exists()) cookieDir.mkdirs()
    File(cookieDir, "$site.txt").writeText(cookies)
}
