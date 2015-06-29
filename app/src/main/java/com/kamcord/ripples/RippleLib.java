package com.kamcord.ripples;

import android.app.Activity;

public class RippleLib
{
    static
    {
        System.loadLibrary("ripples");
    }

    public static native void InitRipple(
            int width, int height, int textureWidth, int textureHeight, int[] pixels, int versionEnum);

    public static native void DestroyRipple();

    public static native void DrawRipple(int versionEnum);

    public static native void TouchRipple(float x, float y);

    // from RippleKamcord.c
    public static native void initKamcord(Activity activity);

    public static native void startRecording();

    public static native void stopRecording();

    public static native void pauseRecording();

    public static native void resumeRecording();

    public static native void showView();
}
