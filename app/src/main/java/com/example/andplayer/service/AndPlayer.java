package com.example.andplayer.service;

import android.text.TextUtils;
import android.util.Log;

import com.example.andplayer.lisnter.IOnPreparedListener;
import com.example.andplayer.lisnter.IPlayerListener;
import com.example.andplayer.opengl.AndGLSurfaceView;

public class AndPlayer {
    static final String TAG = "AndPlayer";

    static {
        System.loadLibrary("andplayer");
    }

    IOnPreparedListener onPreparedListener;
    private AndGLSurfaceView andGLSfView;
    private IPlayerListener playerListener;
    private int duration = 0;  // 总时长
    private String source; // 数据源

    public void setSource(String source) {
        this.source = source;
    }

    public void setAndGLSurfaceView(AndGLSurfaceView surfaceview) {
        this.andGLSfView = surfaceview;
        Log.i(TAG, "setAndGLSurfaceView: -------------" + this.hashCode());
    }

    /**
     * 设置prepared()、player的监听
     */
    public void setOnPreparedListener(IOnPreparedListener iOnPreparedListener) {
        this.onPreparedListener = iOnPreparedListener;
    }
    public void setPlayerListener(IPlayerListener playerListener) {
        this.playerListener = playerListener;
    }

    /**
     * native层调用的java方法
     * 如何设置java到native层的回调呢，可以通过这种方式
     */
    public void onCallPrepared() {
        if (onPreparedListener != null) {
            onPreparedListener.onPrepared();
        }
    }
    public void onCallTimeInfo(int currentTime, int totalTime)
    {
        duration = totalTime;
        if (playerListener == null) {
            return;
        }
        playerListener.onCurrentTime(currentTime, totalTime);
    }
    public void onCallRenderYUV(int width, int height, byte[] y, byte[] u, byte[] v)
    {
        // opengl渲染
        if( this.andGLSfView != null)
        {
            this.andGLSfView.setYUVData(width, height, y, u, v);
        }
    }
    public void onCallLoad(boolean load)
    {
        // 队列 网络 有问题    加载框
        // TODO
    }


    public void prepared()
    {
        if(TextUtils.isEmpty(source)) {
            Log.e(TAG, "prepared source is empty");
            return;
        }
        // 开启解封装线程
        new Thread(new Runnable() {
            @Override
            public void run() {
                Log.d(TAG, "start pthread to prepare");
                n_prepared(source);
            }
        }).start();
    }
    public void start()
    {
        if(TextUtils.isEmpty(source)) {
            Log.e(TAG, "start source is empty");
            return;
        }
        // 开启解码线程  调用Native层方法  开始解码播放
        new Thread(new Runnable() {
            @Override
            public void run() {
                n_start();
            }
        }).start();
    }

    public void pause() { n_pause(); }
    public void resume() { n_resume(); }
    public int getDuration() { return duration; }
    public void seek(int secds) { n_seek(secds); }

    public void setVolume(int percent) {
        n_volume(percent);
    }
    public void setMute(int mute) {
        n_mute(mute);
    }
    public void setSpeed(float speed) { n_speed(speed); }
    public void setTone(float tone) { n_setTone(tone); }
    public void stop()
    {
        new Thread(new Runnable() {
            @Override
            public void run() {
                n_stop();
            }
        }).start();
    }

    /**
     * java调用的native层接口
     */
    public native void n_prepared(String source);
    public native void n_start();
    public native void n_stop();
    private native void n_pause();
    private native void n_seek(int secds);
    private native void n_resume();
    private native void n_mute(int mute);
    private native void n_volume(int percent);
    private native void n_speed(float speed);
    private native void n_setTone(float tone);
}
