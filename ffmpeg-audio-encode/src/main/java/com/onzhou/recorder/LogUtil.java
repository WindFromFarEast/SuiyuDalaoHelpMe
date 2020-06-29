package com.onzhou.recorder;

import android.util.Log;

public class LogUtil {

    private static final String TAG = "XWX-";

    public static void d(String tag, String msg) {
        Log.d(TAG + tag, msg);
    }

    public static void i(String tag, String msg) {
        Log.i(TAG + tag, msg);
    }

    public static void formatD(String tag, String msg, Object... varargs) {
        String str = String.format(msg, varargs);
        d(tag, str);
    }

    public static void formatI(String tag, String msg, Object... varargs) {
        String str = String.format(msg, varargs);
        i(tag, str);
    }

    public static void formatE(String tag, String msg, Object... varargs) {
        String str = String.format(msg, varargs);
        e(tag, str);
    }

    public static void e(String tag, String msg) {
        Log.e(TAG + tag, msg);
    }

}
