package com.kamcord.ripples;

import java.util.HashMap;

import android.app.Application;

public class RippleApplication extends Application
{
    private static RippleApplication rippleSingleton;
    private HashMap<String, Boolean> booleanHashMap = new HashMap<String, Boolean>();

    public static RippleApplication getInstance()
    {
        return rippleSingleton;
    }

    @Override
    public void onCreate()
    {
        rippleSingleton = this;
        super.onCreate();
    }

    public HashMap<String, Boolean> getbooleanHashMap()
    {
        return booleanHashMap;
    }


}
