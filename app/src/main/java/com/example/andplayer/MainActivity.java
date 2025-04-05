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

import com.example.andplayer.lisnter.IOnPreparedListener;
import com.example.andplayer.lisnter.IPlayerListener;
import com.example.andplayer.opengl.AndGLSurfaceView;
import com.example.andplayer.service.AndPlayer;
import com.example.andplayer.ui.utils.DisplayUtil;

import java.io.File;
import java.util.ArrayList;
import java.util.List;


public class MainActivity extends AppCompatActivity {
    private AndPlayer mAndPlayer;
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

        mAndPlayer = new AndPlayer();
        mAndPlayer.setAndGLSurfaceView(andGLSurfaceView);

        setListeners();
        addFileIndex();
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
     * 设置各种listener
     */
    public void setListeners() {
        mAndPlayer.setPlayerListener(new IPlayerListener() {
            @Override
            public void onCurrentTime(int currentTime, int totalTime) {
                // isSeek 需要当前时间、总时间
                if (!isSeek && totalTime > 0) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            seekBar.setProgress(currentTime * 100 / totalTime);
                            tvTime.setText(DisplayUtil.secdsToDateFormat(currentTime)
                                    + "/" + DisplayUtil.secdsToDateFormat(totalTime));
                        }
                    });
                }
            }

            @Override
            public void onLoad(boolean load) {
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
                position = progress * mAndPlayer.getDuration() / 100;
            }
            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                isSeek = true;
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                mAndPlayer.seek(position);
                isSeek = false;
            }
        });
    }

    public void addFileIndex() {
        File file = new File(Environment.getExternalStorageDirectory(), "input.mkv");
        paths.add(file.getAbsolutePath());
//        file = new File(Environment.getExternalStorageDirectory(),"input.avi");
//        paths.add(file.getAbsolutePath());

//        file = new File(Environment.getExternalStorageDirectory(),"input.rmvb");
//        file = new File(Environment.getExternalStorageDirectory(),"input.mp4");
        file = new File(Environment.getExternalStorageDirectory(), "brave_960x540.flv");
        paths.add(file.getAbsolutePath());
//        paths.add("http://mn.maliuedu.com/music/input.mp4");
    }

    /**
     * 播放控制：开始 停止 暂停 恢复 切换
     */
    public void begin(View view) {
        mAndPlayer.setOnPreparedListener(new IOnPreparedListener() {
            @Override
            public void onPrepared() {
                mAndPlayer.start();  // 监听到Prepared()完成，就开始解码
            }
        });

        //File file = new File(Environment.getExternalStorageDirectory(), "brave_960x540.flv");
        //mAndPlayer.setSource(file.getAbsolutePath());
        //mAndPlayer.setSource("http://demo-videos.qnsdk.com/bbk-H265-50fps.mp4");
        //mAndPlayer.setSource("http://demo-videos.qnsdk.com/VR-Panorama-Equirect-Angular-4500k.mp4");
        mAndPlayer.setSource("http://vjs.zencdn.net/v/oceans.mp4");
        mAndPlayer.prepared();
    }
    public void pause(View view) {
        mAndPlayer.pause();
    }
    public void resume(View view) {
        mAndPlayer.resume();
    }
    public void stop(View view) {
        mAndPlayer.stop();
    }
    public void next(View view) {
        //mAndPlayer.playNext("http://sf1-hscdn-tos.pstatp.com/obj/media-fe/xgplayer_doc_video/flv/xgplayer-demo-360p.flv");
    }
    public void speed1(View view) {
        mAndPlayer.setSpeed(0.5f);
    }
    public void speed2(View view) {
        mAndPlayer.setSpeed(2.0f);
    }
    public void mute(View view) {
        if (!isMuted) {
            mAndPlayer.setMute(2);
            isMuted = true;
        } else {
            mAndPlayer.setMute(3);
            isMuted = false;
        }
    }
    public void highTone(View view) {
        mAndPlayer.setTone(2.0f);
    }
    public void lowTone(View view) {
        mAndPlayer.setTone(0.5f);
    }
    public void normal(View view) {
        mAndPlayer.setSpeed(1.0f);
        mAndPlayer.setTone(1.0f);
    }
}
