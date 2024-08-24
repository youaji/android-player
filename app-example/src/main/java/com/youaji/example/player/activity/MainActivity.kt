package com.youaji.example.player.activity

import android.os.Bundle
import com.youaji.example.player.R
import com.youaji.example.player.databinding.ActivityMainBinding
import com.youaji.libs.debug.DebugService
import com.youaji.libs.ui.basic.BasicBindingActivity
import com.youaji.libs.ui.state.NavBtnType
import com.youaji.libs.util.startActivity

class MainActivity : BasicBindingActivity<ActivityMainBinding>() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setToolbar {
            navBtnType = NavBtnType.NONE
            title = getString(R.string.app_name)
        }

        binding.btnSurface.setOnClickListener { startActivity<SurfaceActivity>() }
        binding.btnPlayer.setOnClickListener { startActivity<PlayerActivity>() }

        DebugService.get.setFloatButton(application)
    }

}