package com.youaji.example.player.dialog

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Rect
import android.os.Bundle
import android.view.Gravity
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.core.content.ContextCompat
import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentActivity
import androidx.viewpager2.adapter.FragmentStateAdapter
import androidx.viewpager2.widget.ViewPager2
import com.google.android.material.tabs.TabLayoutMediator
import com.youaji.example.player.R
import com.youaji.example.player.databinding.FragmentPlayerSettingBinding
import com.youaji.example.player.databinding.ItemTextBinding
import com.youaji.example.player.fragment.ListFragment
import com.youaji.libs.player.Background
import com.youaji.libs.player.Effect
import com.youaji.libs.player.Filter
import com.youaji.libs.player.WaterMarkLocation
import com.youaji.libs.ui.adapter.RecycleViewAdapter
import com.youaji.libs.ui.basic.BasicBindingDialogFragment
import com.youaji.libs.util.dp
import java.nio.ByteBuffer

class PlayerSetting : BasicBindingDialogFragment<FragmentPlayerSettingBinding>() {

    private lateinit var builder: Builder
    private var tabLayoutMediator: TabLayoutMediator? = null
    private val pageChangeCallback = object : ViewPager2.OnPageChangeCallback() {
        override fun onPageSelected(position: Int) {
            val tabCount: Int = binding.layoutTab.tabCount
            for (i in 0 until tabCount) {
                val tab = binding.layoutTab.getTabAt(i)
                val tabView = tab?.customView as TextView
                tabView.setTextColor(
                    ContextCompat.getColor(
                        builder.activity,
                        if (tab.position == position) R.color.black
                        else R.color.gray_80
                    )
                )
            }
        }
    }

    private val waterMarkData = listOf(
        Pair("文字-左上", WaterMarkLocation.LEFT_TOP),
        Pair("文字-左下", WaterMarkLocation.LEFT_BOTTOM),
        Pair("文字-右上", WaterMarkLocation.RIGHT_TOP),
        Pair("文字-右下", WaterMarkLocation.RIGHT_BOTTOM),
        Pair("图片-左上", WaterMarkLocation.LEFT_TOP),
        Pair("图片-左下", WaterMarkLocation.LEFT_BOTTOM),
        Pair("图片-右上", WaterMarkLocation.RIGHT_TOP),
        Pair("图片-右下", WaterMarkLocation.RIGHT_BOTTOM),
    )

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (!::builder.isInitialized) {
            dismiss()
        }
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        val pages = listOf(
            Pair("Watermark", ListFragment.newGridInstance(3, ListAdapter(), waterMarkData.map { it.first }) { position, _ ->
                val info = waterMarkData[position]
                if (info.first.startsWith("文字")) {
                    setWaterMarkText("水印文字", 100, 100, info.second)
                } else {
                    setWaterMarkImage(R.mipmap.ic_launcher, info.second)
                }
            }),
            Pair("Background", ListFragment.newGridInstance(3, ListAdapter(), Background.values().toList().map { it.desc }) { position, _ ->
                builder.backgroundCallback?.invoke(Background.values()[position])
            }),
            Pair("Filter", ListFragment.newGridInstance(3, ListAdapter(), Filter.values().toList().map { it.desc }) { position, _ ->
                builder.filterCallback?.invoke(Filter.values()[position])
            }),
            Pair("Effect", ListFragment.newGridInstance(3, ListAdapter(), Effect.values().toList().map { it.desc }) { position, _ ->
                builder.effectCallback?.invoke(Effect.values()[position])
            }),
        )
        binding.pagerView.setOffscreenPageLimit(pages.size)
        binding.pagerView.adapter = object : FragmentStateAdapter(parentFragmentManager, lifecycle) {
            override fun getItemCount(): Int = pages.size
            override fun createFragment(position: Int): Fragment = pages[position].second
        }
        binding.pagerView.registerOnPageChangeCallback(pageChangeCallback)
        tabLayoutMediator = TabLayoutMediator(binding.layoutTab, binding.pagerView) { tab, position ->
            val textView = TextView(builder.activity)
            textView.text = pages[position].first
            textView.textSize = 16f
            textView.gravity = Gravity.CENTER
            tab.customView = textView
        }
        tabLayoutMediator?.attach()
    }

    override fun onDestroy() {
        tabLayoutMediator?.detach()
        binding.pagerView.unregisterOnPageChangeCallback(pageChangeCallback)
        super.onDestroy()
    }

    private fun setWaterMarkImage(resId: Int, location: WaterMarkLocation) {
        val options = BitmapFactory.Options()
        options.inPreferredConfig = Bitmap.Config.ARGB_8888
        val bitmap = BitmapFactory.decodeResource(resources, resId, options)
        val byteCount = bitmap.byteCount
        val buffer = ByteBuffer.allocate(byteCount)
        bitmap.copyPixelsToBuffer(buffer)
        buffer.position(0)

        val watermark = buffer.array()
        val watermarkWidth = bitmap.width
        val watermarkHeight = bitmap.height
        bitmap.recycle()

        builder.waterMarkCallback?.invoke(watermark, watermark.size, watermarkWidth, watermarkHeight, 7f, location)
    }

    private fun setWaterMarkText(text: String, width: Int, height: Int, location: WaterMarkLocation) {
        val textBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(textBitmap)
        val paint = Paint()
        paint.color = Color.argb(255, 0, 255, 0)
        paint.textSize = 28f
        paint.isAntiAlias = true
        paint.textAlign = Paint.Align.CENTER
        val rect = Rect(0, 0, width, height)
        val fontMetrics = paint.fontMetricsInt
        val baseline = (rect.bottom + rect.top - fontMetrics.bottom - fontMetrics.top) / 2
        canvas.drawText(text, rect.centerX().toFloat(), baseline.toFloat(), paint)
        val capacity = width * height * 4 // ARGB_8888一个像素4个字节
        val buffer = ByteBuffer.allocate(capacity)
        textBitmap.copyPixelsToBuffer(buffer)
        buffer.position(0)

        val watermark = buffer.array()
        val watermarkWidth = textBitmap.width
        val watermarkHeight = textBitmap.height
        textBitmap.recycle()

        builder.waterMarkCallback?.invoke(watermark, watermark.size, watermarkWidth, watermarkHeight, 6f, location)
    }

    fun show(): PlayerSetting {
        super.show(builder.activity.supportFragmentManager, "PlayerSetting")
        return this
    }

    class ListAdapter : RecycleViewAdapter<String, ItemTextBinding>() {
        private var checked = -1
        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder<ItemTextBinding> =
            ViewHolder(ItemTextBinding.inflate(LayoutInflater.from(parent.context), parent, false))

        override fun onBindViewHolder(holder: ViewHolder<ItemTextBinding>, position: Int, binding: ItemTextBinding, bean: String) {
            binding.text.text = bean
            val layoutParams = binding.text.layoutParams
            layoutParams.height = 40.dp.toInt()
            binding.text.layoutParams = layoutParams
            if (checked == position) {
                binding.text.setBackgroundResource(R.drawable.shape_c5_soblack)
                binding.text.setTextColor(ContextCompat.getColor(binding.root.context, R.color.white))
            } else {
                binding.text.setBackgroundResource(R.drawable.shape_c5_sowhite)
                binding.text.setTextColor(ContextCompat.getColor(binding.root.context, R.color.black))
            }
            binding.root.setOnClickListener {
                if (checked == position)
                    return@setOnClickListener
                val previousChecked = checked
                checked = position
                notifyItemChanged(checked)
                if (previousChecked >= 0) {
                    notifyItemChanged(previousChecked)
                }
                itemClickListener?.onItemClick(position, bean)
            }
        }
    }

    class Builder(val activity: FragmentActivity) {

        var backgroundCallback: ((background: Background) -> Unit)? = null
            private set
        var filterCallback: ((filter: Filter) -> Unit)? = null
            private set
        var effectCallback: ((effect: Effect) -> Unit)? = null
            private set
        var waterMarkCallback: ((dataArray: ByteArray, dataLen: Int, width: Int, height: Int, scale: Float, location: WaterMarkLocation) -> Unit)? = null
            private set

        fun setBackgroundCallback(setBackgroundCallback: (background: Background) -> Unit): Builder {
            this.backgroundCallback = setBackgroundCallback
            return this
        }

        fun setFilterCallback(setFilterCallback: (filter: Filter) -> Unit): Builder {
            this.filterCallback = setFilterCallback
            return this
        }

        fun setEffectCallback(setEffectCallback: (effect: Effect) -> Unit): Builder {
            this.effectCallback = setEffectCallback
            return this
        }

        fun setWaterMarkCallback(setWaterMarkCallback: (dataArray: ByteArray, dataLen: Int, width: Int, height: Int, scale: Float, location: WaterMarkLocation) -> Unit): Builder {
            this.waterMarkCallback = setWaterMarkCallback
            return this
        }

        fun build(): PlayerSetting {
            val dialog = PlayerSetting()
            dialog.setCanceledBack(true)
                .setCanceledOnTouchOutside(true)
                .setDimEnabled(false)
                .setBackgroundColor(ContextCompat.getColor(activity, R.color.white))
                .setGravity(Gravity.BOTTOM)
                .setWidthRatio(1f)
                .setHeightRatio(0.3f)
                .setLeftTopRadius(10.dp.toInt())
                .setRightTopRadius(10.dp.toInt())
                .setAnimations(android.R.style.Animation_InputMethod)
            dialog.builder = this
            return dialog
        }
    }
}