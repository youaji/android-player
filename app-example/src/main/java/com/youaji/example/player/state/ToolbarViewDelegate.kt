@file:Suppress("unused")
package com.youaji.example.player.state

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.core.view.isVisible
import com.youaji.example.player.databinding.LayoutStateToolbarBinding
import com.youaji.libs.ui.state.BasicToolbarViewDelegate
import com.youaji.libs.ui.state.NavBtnType
import com.youaji.libs.ui.state.ToolbarConfig
import com.youaji.libs.ui.state.toolbarExtras

var ToolbarConfig.rightTextColor: Int? by toolbarExtras()

/**
 * @author youaji
 * @since 2023/1/1
 */
class ToolbarViewDelegate : BasicToolbarViewDelegate() {
    private lateinit var binding: LayoutStateToolbarBinding

    override fun onCreateToolbar(inflater: LayoutInflater, parent: ViewGroup): View {
        binding = LayoutStateToolbarBinding.inflate(inflater, parent, false)
        return binding.root
    }

    override fun onBindToolbar(config: ToolbarConfig) {
        binding.apply {

            tvTitle.text = config.title


            when (config.navBtnType) {
                NavBtnType.ICON -> {
                    config.navIcon?.let {
                        ivLeft.setImageResource(it)
                    }
                    ivLeft.setOnClickListener(config.onNavClickListener)
                    tvLeft.visibility = View.GONE
                    ivLeft.visibility = View.VISIBLE
                }

                NavBtnType.TEXT -> {
                    tvLeft.text = config.navText
                    tvLeft.setOnClickListener(config.onNavClickListener)
                    tvLeft.visibility = View.VISIBLE
                    ivLeft.visibility = View.GONE
                }

                NavBtnType.ICON_TEXT -> {
                    config.navIcon?.let {
                        ivLeft.setImageResource(it)
                    }
                    tvLeft.text = config.navText
                    ivLeft.setOnClickListener(config.onNavClickListener)
                    tvLeft.setOnClickListener(config.onNavClickListener)
                    tvLeft.visibility = View.VISIBLE
                    ivLeft.visibility = View.VISIBLE
                }

                NavBtnType.NONE -> {
                    ivLeft.visibility = View.GONE
                    tvLeft.visibility = View.GONE
                }
            }

            tvRight.isVisible = config.rightText != null
            config.rightText?.let {
                tvRight.text = it
                tvRight.setOnClickListener(config.onRightTextClickListener)
                config.rightTextColor?.let { color -> tvRight.setTextColor(color) }
            }

            ivRight.isVisible = config.rightIcon != null
            config.rightIcon?.let {
                ivRight.setImageResource(it)
                ivRight.setOnClickListener(config.onRightIconClickListener)
            }
        }
    }
}