package com.youaji.example.player.fragment

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.GridLayoutManager
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.viewbinding.ViewBinding
import com.youaji.example.player.databinding.FragmentListBinding
import com.youaji.libs.ui.adapter.RecycleViewAdapter
import com.youaji.libs.ui.basic.BasicBindingFragment
import com.youaji.libs.util.dp
import com.youaji.libs.widget.recyclerView.decoration.GridItemDecoration
import com.youaji.libs.widget.recyclerView.decoration.LinearItemDecoration

class ListFragment<E : Any, VB : ViewBinding>(
    private val spanCount: Int,
    private var adapter: RecycleViewAdapter<E, VB>,
    private var dataList: List<E> = listOf(),
    private var callback: ((position: Int, item: E) -> Unit)? = null
) : BasicBindingFragment<FragmentListBinding>() {

    companion object {
        @JvmStatic
        fun <E : Any, VB : ViewBinding> newInstance(
            adapter: RecycleViewAdapter<E, VB>,
            dataList: List<E> = listOf(),
            callback: ((position: Int, item: E) -> Unit)? = null
        ) = ListFragment(-1, adapter, dataList, callback)

        @JvmStatic
        fun <E : Any, VB : ViewBinding> newGridInstance(
            spanCount: Int,
            adapter: RecycleViewAdapter<E, VB>,
            dataList: List<E> = listOf(),
            callback: ((position: Int, item: E) -> Unit)? = null
        ) = ListFragment(spanCount, adapter, dataList, callback)
    }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
        return super.onCreateView(inflater, container, savedInstanceState)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        if (spanCount > 0) {
            binding.list.layoutManager = GridLayoutManager(context, spanCount)
            binding.list.addItemDecoration(GridItemDecoration(spanCount = spanCount, space = 5.dp.toInt(), includeEdge = true))
        } else {
            binding.list.layoutManager = LinearLayoutManager(context)
            binding.list.addItemDecoration(LinearItemDecoration(space = 5.dp.toInt(), firstSpace = 5.dp.toInt()))
        }

        binding.list.adapter = adapter
        adapter.itemClickListener = RecycleViewAdapter.OnItemClickListener { position, bean ->
            callback?.invoke(position, bean)
        }
        adapter.dataList = dataList.toMutableList()
    }

}