package com.mediadownloader

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.viewModels
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
            MainScreen(viewModel)
        }
    }
}
