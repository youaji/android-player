@file:Suppress("unused")
package com.youaji.example.player.state

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.appcompat.widget.AppCompatImageView
import androidx.appcompat.widget.AppCompatTextView
import com.youaji.example.player.R
import com.youaji.libs.ui.state.LoadingStateView
import com.youaji.libs.ui.state.ViewType

/**
 * @author youaji
 * @since 2023/1/1
 */
class EmptyViewDelegate : LoadingStateView.ViewDelegate(ViewType.EMPTY) {

    override fun onCreateView(inflater: LayoutInflater, parent: ViewGroup): View =
        inflater.inflate(R.layout.layout_state_empty, parent, false)
            .apply {
                findViewById<AppCompatTextView>(R.id.tv_empty_text)
                findViewById<AppCompatImageView>(R.id.imageView)
            }
}