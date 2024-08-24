@file:Suppress("unused")
package com.youaji.example.player.state

import android.content.Context
import androidx.startup.Initializer
import com.youaji.libs.ui.state.LoadingStateView

internal class StateInitializer : Initializer<Unit> {

    override fun create(context: Context) {
        LoadingStateView.setViewDelegatePool {
            register(
                ToolbarViewDelegate(),
                LoadingViewDelegate(),
                EmptyViewDelegate(),
                ErrorViewDelegate(),
            )
        }
    }

    override fun dependencies() = emptyList<Class<Initializer<*>>>()
}
