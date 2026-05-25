package com.mediadownloader

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.viewModels
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import com.mediadownloader.ui.MainScreen
import com.mediadownloader.ui.DownloadViewModel
import com.mediadownloader.auth.Paths

class MainActivity : ComponentActivity() {
    private val viewModel: DownloadViewModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        Paths.init(this)
        enableEdgeToEdge()
        setContent {
            val darkTheme by viewModel.darkTheme.collectAsState()
            val systemDark = isSystemInDarkTheme()

            MaterialTheme(
                colorScheme = if (darkTheme) darkColorScheme() else lightColorScheme()
            ) {
                MainScreen(viewModel)
            }
        }
    }
}
