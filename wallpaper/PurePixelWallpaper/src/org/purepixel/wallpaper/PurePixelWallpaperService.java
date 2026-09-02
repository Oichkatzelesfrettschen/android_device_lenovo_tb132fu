/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: Apache-2.0
 */
package org.purepixel.wallpaper;

import android.app.WallpaperColors;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.service.wallpaper.WallpaperService;
import android.util.Log;
import android.view.SurfaceHolder;

import java.util.ArrayList;
import java.util.List;

/**
 * Event-driven default wallpaper for Pure Pixel.
 *
 * <p>The service deliberately has no animation loop, timer, network access, or
 * wake lock. It draws the matching light/dark bitmap only when a wallpaper
 * surface needs content or the system night mode changes.</p>
 */
public final class PurePixelWallpaperService extends WallpaperService {
    private static final String TAG = "PurePixelWallpaper";

    private final List<PurePixelEngine> mEngines = new ArrayList<>();

    private Bitmap mDarkBitmap;
    private Bitmap mLightBitmap;
    private WallpaperColors mDarkColors;
    private WallpaperColors mLightColors;
    private boolean mNightMode;

    @Override
    public void onCreate() {
        super.onCreate();
        mNightMode = isNightMode(getResources().getConfiguration());
    }

    @Override
    public Engine onCreateEngine() {
        final PurePixelEngine engine = new PurePixelEngine();
        mEngines.add(engine);
        return engine;
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        final boolean nightMode = isNightMode(newConfig);
        if (nightMode == mNightMode) {
            return;
        }

        mNightMode = nightMode;
        for (PurePixelEngine engine : new ArrayList<>(mEngines)) {
            engine.onThemeChanged();
        }
    }

    private static boolean isNightMode(Configuration configuration) {
        return (configuration.uiMode & Configuration.UI_MODE_NIGHT_MASK)
                == Configuration.UI_MODE_NIGHT_YES;
    }

    private Bitmap getSelectedBitmap() {
        if (mNightMode) {
            if (mDarkBitmap == null) {
                mDarkBitmap = BitmapFactory.decodeResource(
                        getResources(), R.drawable.pure_pixel_wallpaper_dark);
            }
            return mDarkBitmap;
        }

        if (mLightBitmap == null) {
            mLightBitmap = BitmapFactory.decodeResource(
                    getResources(), R.drawable.pure_pixel_wallpaper_light);
        }
        return mLightBitmap;
    }

    private WallpaperColors getSelectedColors() {
        if (mNightMode) {
            if (mDarkColors == null) {
                mDarkColors = WallpaperColors.fromBitmap(getSelectedBitmap());
            }
            return mDarkColors;
        }

        if (mLightColors == null) {
            mLightColors = WallpaperColors.fromBitmap(getSelectedBitmap());
        }
        return mLightColors;
    }

    private final class PurePixelEngine extends Engine {
        private final Paint mPaint = new Paint(
                Paint.ANTI_ALIAS_FLAG | Paint.DITHER_FLAG | Paint.FILTER_BITMAP_FLAG);
        private boolean mVisible;

        PurePixelEngine() {
            setTouchEventsEnabled(false);
            setOffsetNotificationsEnabled(false);
        }

        @Override
        public void onVisibilityChanged(boolean visible) {
            mVisible = visible;
            if (visible) {
                drawFrame();
            }
        }

        @Override
        public void onSurfaceCreated(SurfaceHolder holder) {
            super.onSurfaceCreated(holder);
            drawFrame();
        }

        @Override
        public void onSurfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            super.onSurfaceChanged(holder, format, width, height);
            drawFrame();
        }

        @Override
        public void onSurfaceRedrawNeeded(SurfaceHolder holder) {
            super.onSurfaceRedrawNeeded(holder);
            drawFrame();
        }

        @Override
        public WallpaperColors onComputeColors() {
            return getSelectedColors();
        }

        @Override
        public void onDestroy() {
            mEngines.remove(this);
            super.onDestroy();
        }

        void onThemeChanged() {
            if (mVisible) {
                drawFrame();
            }
            notifyColorsChanged();
        }

        private void drawFrame() {
            final SurfaceHolder holder = getSurfaceHolder();
            Canvas canvas = null;
            try {
                try {
                    canvas = holder.lockHardwareCanvas();
                } catch (RuntimeException e) {
                    canvas = holder.lockCanvas();
                }

                if (canvas == null) {
                    return;
                }

                final Bitmap bitmap = getSelectedBitmap();
                if (bitmap == null || bitmap.getWidth() <= 0 || bitmap.getHeight() <= 0) {
                    canvas.drawColor(Color.BLACK);
                    return;
                }

                final int width = canvas.getWidth();
                final int height = canvas.getHeight();
                final float scale = Math.max(
                        (float) width / bitmap.getWidth(),
                        (float) height / bitmap.getHeight());
                final float drawnWidth = bitmap.getWidth() * scale;
                final float drawnHeight = bitmap.getHeight() * scale;
                final float left = (width - drawnWidth) / 2.0f;
                final float top = (height - drawnHeight) / 2.0f;

                canvas.drawColor(Color.BLACK);
                canvas.drawBitmap(bitmap, null,
                        new RectF(left, top, left + drawnWidth, top + drawnHeight), mPaint);
            } catch (RuntimeException e) {
                Log.w(TAG, "Unable to draw wallpaper frame", e);
            } finally {
                if (canvas != null) {
                    holder.unlockCanvasAndPost(canvas);
                }
            }
        }
    }
}
