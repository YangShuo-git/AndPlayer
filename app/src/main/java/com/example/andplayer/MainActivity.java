package com.example.andplayer;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import com.example.andplayer.ui.utils.DisplayUtil;
import com.example.andplayer.lisnter.IPlayerListener;
import com.example.andplayer.lisnter.IOnPreparedListener;
import com.example.andplayer.opengl.AndGLSurfaceView;
import com.example.andplayer.service.AndPlayer;


public class MainActivity extends AppCompatActivity {
    private AndPlayer andPlayer;
    private AndGLSurfaceView andGLSurfaceView;
    private SeekBar seekBar;
    private TextView tvTime;
    private Button muteBtn;
    private boolean isMuted = false;
    private int position;
    private boolean isSeek = false;
    List<String> paths = new ArrayList<>();

    static {
        System.loadLibrary("andplayer");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        andGLSurfaceView = findViewById(R.id.andglsurfaceview);
        seekBar = findViewById(R.id.seekbar);
        tvTime = findViewById(R.id.tv_time);
        muteBtn = findViewById(R.id.mute);
        checkPermission();

        andPlayer = new AndPlayer();
        andPlayer.setAndGLSurfaceView(andGLSurfaceView);

        File file = new File(Environment.getExternalStorageDirectory(),"input.mkv");
        paths.add(file.getAbsolutePath());
//        file = new File(Environment.getExternalStorageDirectory(),"input.avi");
//        paths.add(file.getAbsolutePath());

//        file = new File(Environment.getExternalStorageDirectory(),"input.rmvb");
//        file = new File(Environment.getExternalStorageDirectory(),"input.mp4");
        file = new File(Environment.getExternalStorageDirectory(),"brave_960x540.flv");
        paths.add(file.getAbsolutePath());
//        paths.add("http://mn.maliuedu.com/music/input.mp4");
        andPlayer.setPlayerListener(new IPlayerListener() {
            @Override
            public void onLoad(boolean load) {

            }
            @Override
            public void onCurrentTime(int currentTime, int totalTime) {
                // isSeek 需要当前时间、总时间
                if(!isSeek && totalTime > 0)
                {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            seekBar.setProgress(currentTime * 100 / totalTime);
                            tvTime.setText( DisplayUtil.secdsToDateFormat(currentTime)
                                    + "/" + DisplayUtil.secdsToDateFormat(totalTime));
                        }
                    });
                }
            }
            @Override
            public void onError(int code, String msg) {

            }
            @Override
            public void onPause(boolean pause) {

            }
            @Override
            public void onDbValue(int db) {

            }
            @Override
            public void onComplete() {

            }
            @Override
            public String onNext() {
                return null;
            }
        });

        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                position = progress * andPlayer.getDuration() / 100;
            }
            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                isSeek = true;
            }
            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                andPlayer.seek(position);
                isSeek = false;
            }
        });
    }

    public boolean checkPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && checkSelfPermission(
                Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{
                    Manifest.permission.READ_EXTERNAL_STORAGE,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE
            }, 1);

        }
        return false;
    }

    /**
     * 播放控制：开始 停止 暂停 恢复 切换
     */
    public void begin(View view) {
        andPlayer.setOnPreparedListener(new IOnPreparedListener() {
            @Override
            public void onPrepared() {
                andPlayer.start();  // 监听到Prepared()完成，就开始解码
            }
        });

        File file = new File(Environment.getExternalStorageDirectory(),"brave_960x540.flv");
        andPlayer.setSource(file.getAbsolutePath());

        // andPlayer.setSource("http://sf1-hscdn-tos.pstatp.com/obj/media-fe/xgplayer_doc_video/flv/xgplayer-demo-360p.flv");
        andPlayer.prepared();
    }
    public void pause(View view) {
        andPlayer.pause();
    }
    public void resume(View view) {
        andPlayer.resume();
    }
    public void stop(View view) {
        andPlayer.stop();
    }
    public void next(View view) {
        //wlPlayer.playNext("/mnt/shared/Other/testvideo/楚乔传第一集.mp4");
    }
    public void speed1(View view) {
        andPlayer.setSpeed(0.5f);
    }
    public void speed2(View view) {
        andPlayer.setSpeed(2.0f);
    }
    public void mute(View view) {
        if (!isMuted) {
            andPlayer.setMute(2);
            isMuted = true;
        } else {
            andPlayer.setMute(3);
            isMuted = false;
        }
    }
    public void highTone(View view) {
        andPlayer.setTone(2.0f);
    }
    public void lowTone(View view) {
        andPlayer.setTone(0.5f);
    }
    public void normal(View view) {
        andPlayer.setSpeed(1.0f);
        andPlayer.setTone(1.0f);
    }
}
