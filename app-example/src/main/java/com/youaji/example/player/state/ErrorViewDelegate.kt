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
class ErrorViewDelegate : LoadingStateView.ViewDelegate(ViewType.ERROR) {

    override fun onCreateView(inflater: LayoutInflater, parent: ViewGroup): View =
        inflater.inflate(R.layout.layout_state_error, parent, false)
            .apply {
                findViewById<AppCompatTextView>(R.id.tv_error_text)
                findViewById<AppCompatTextView>(R.id.btn_reload)
                findViewById<AppCompatImageView>(R.id.imageView3)
                findViewById<View>(R.id.btn_reload).setOnClickListener {
                    onReloadListener?.onReload()
                }
            }
}