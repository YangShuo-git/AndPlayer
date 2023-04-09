package com.example.andplayer;

import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.media.MediaCodec;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    // Used to load the 'andplayer' library on application startup.
    static {
        System.loadLibrary("andplayer");
    }
    Surface surface;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        checkPermission();

        SurfaceView surfaceView = (SurfaceView) findViewById(R.id.surface);
        final SurfaceHolder surfaceViewHolder = surfaceView.getHolder();

        surfaceViewHolder.addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                surface = surfaceViewHolder.getSurface();
            }
            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {    }
            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {    }
        });
    }
    public boolean checkPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && checkSelfPermission(
                Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{
                    Manifest.permission.READ_EXTERNAL_STORAGE,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE,
                    Manifest.permission.CAMERA
            }, 1);
        }
        return false;
    }
    public void play(View view) {
        //获取文件路径，这里将文件放置在手机根目录下的sdcard目录
        String folderurl = new File(Environment.getExternalStorageDirectory(), "input.mp4").getAbsolutePath();
        play(folderurl, surface);
    }


    public native int play(String url, Surface surface);
}