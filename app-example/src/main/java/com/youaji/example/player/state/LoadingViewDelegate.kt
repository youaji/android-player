@file:Suppress("unused")
package com.youaji.example.player.state

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ProgressBar
import com.youaji.example.player.R
import com.youaji.libs.ui.state.LoadingStateView
import com.youaji.libs.ui.state.ViewType

/**
 * @author youaji
 * @since 2023/1/1
 */
class LoadingViewDelegate : LoadingStateView.ViewDelegate(ViewType.LOADING) {

    override fun onCreateView(inflater: LayoutInflater, parent: ViewGroup): View =
        inflater.inflate(R.layout.layout_state_loading, parent, false)
            .apply {
                findViewById<ProgressBar>(R.id.progress_bar)
            }

}