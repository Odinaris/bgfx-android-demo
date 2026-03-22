package com.odinaris.bgfx_android_demo

import android.os.Bundle
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.appcompat.app.AppCompatActivity
import com.odinaris.bgfx_android_demo.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity(), SurfaceHolder.Callback {

    private lateinit var binding: ActivityMainBinding
    private var isBgfxInitialized = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // Setup SurfaceView for bgfx rendering
        binding.surfaceView.holder.addCallback(this)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        val width = binding.surfaceView.width
        val height = binding.surfaceView.height
        isBgfxInitialized = initBgfx(width, height, holder.surface)
        if (isBgfxInitialized) {
            // Start rendering loop
            startRenderLoop()
        }
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        // Handle surface changes if needed
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        if (isBgfxInitialized) {
            shutdownBgfx()
            isBgfxInitialized = false
        }
    }

    private fun startRenderLoop() {
        if (isBgfxInitialized) {
            // Render one frame
            renderBgfxFrame()
            // Schedule next frame
            binding.surfaceView.post { startRenderLoop() }
        }
    }

    override fun onPause() {
        super.onPause()
        if (isBgfxInitialized) {
            shutdownBgfx()
            isBgfxInitialized = false
        }
    }

    override fun onResume() {
        super.onResume()
        if (!isBgfxInitialized) {
            val holder = binding.surfaceView.holder
            if (holder.surface.isValid) {
                surfaceCreated(holder)
            }
        }
    }

    /**
     * Initialize bgfx renderer
     * @param width Surface width
     * @param height Surface height
     * @param surface Android Surface object
     * @return true if initialization succeeded
     */
    external fun initBgfx(width: Int, height: Int, surface: android.view.Surface): Boolean

    /**
     * Shutdown bgfx renderer
     */
    external fun shutdownBgfx()

    /**
     * Render one frame
     */
    external fun renderBgfxFrame()

    /**
     * Check if bgfx is initialized
     */
    external fun isBgfxInitialized(): Boolean

    companion object {
        init {
            System.loadLibrary("bgfx_android_demo")
        }
    }
}
