package com.example.andplayer;

import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.content.pm.PackageManager;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.media.MediaCodec;
import android.widget.Button;
import android.widget.Toast;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    // 首先要加载jni的库
    static {
        System.loadLibrary("andplayer");
    }
    Surface surface;
    private AudioTrack audioTrack;
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

        Button btn = (Button) findViewById(R.id.btn);
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                File file = new File(Environment.getExternalStorageDirectory(), "dngl.mp3");
                MainActivity.this.playSound(file.getAbsolutePath());
            }
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

    //  native层回调该函数  创建AudioTrack
    public void createTrack(int sampleRateInHz, int channels) {
        Toast.makeText(this, "初始化音频播放器", Toast.LENGTH_SHORT).show();

        int channaleConfig;  // 就是声道数
        if (channels == 1) {
            channaleConfig = AudioFormat.CHANNEL_OUT_MONO;
        } else if (channels == 2) {
            channaleConfig = AudioFormat.CHANNEL_OUT_STEREO;
        }else {
            channaleConfig = AudioFormat.CHANNEL_OUT_MONO;
        }
        int buffersize = AudioTrack.getMinBufferSize(sampleRateInHz, channaleConfig, AudioFormat.ENCODING_PCM_16BIT);

        audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRateInHz, channaleConfig, AudioFormat.ENCODING_PCM_16BIT,
                buffersize,
                AudioTrack.MODE_STREAM);
        audioTrack.play();
    }
    //  native层回调该函数  播放音频，需要buffer pcm数据内容 pcm实际长度  主线程不仅解码，而且播放，这种方式有问题
    public void playTrack(byte[] buffer, int length) {
        if (audioTrack != null && audioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
            audioTrack.write(buffer, 0, length);
        }
    }


    public native int play(String url, Surface surface);
    public native int playSound(String url);
}