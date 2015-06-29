package com.kamcord.ripples;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.media.AudioAttributes;
import android.media.AudioManager;
import android.media.SoundPool;
import android.media.SoundPool.OnLoadCompleteListener;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.view.View;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.kamcord.ripples.RippleView.GLVersion;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Timer;
import java.util.TimerTask;

public class RippleActivity extends FragmentActivity
{
    RippleView mView;

    public TextView rippleMessage;
    private SoundPool soundPool;
    private int soundID;
    boolean plays = false, loaded = false;
    float actVolume, maxVolume, volume;
    AudioManager audioManager;
    int counter;


    Timer updateTimer;

    @SuppressLint("SimpleDateFormat")
	static SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd hh:mm:ss.SSS");
    private Runnable setDateTime = new Runnable(){
    	@Override
    	public void run(){    		
    		TextView dtf = ((TextView)findViewById(R.id.dateTimeField));
    		dtf.setText(sdf.format(new Date()));
    	}
    };
    
    @TargetApi(Build.VERSION_CODES.HONEYCOMB)
    @Override
    protected void onCreate(Bundle bundle) {


        super.onCreate(bundle);

        mView = new RippleView(getApplication(), GLVersion.ES_2_0);

        setContentView(R.layout.activity_main);
        RelativeLayout frameLayout = (RelativeLayout) findViewById(R.id.mainlayout);


        rippleMessage = (TextView) findViewById(R.id.ripplelistener_msg);

        frameLayout.addView(mView, 0);
        // AudioManager audio settings for adjusting the volume
        counter = 0;
        audioManager = (AudioManager) getSystemService(AUDIO_SERVICE);
        actVolume = (float) audioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
        maxVolume = (float) audioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
        volume = actVolume / maxVolume;

        //Hardware buttons setting to adjust the media sound
        this.setVolumeControlStream(AudioManager.STREAM_MUSIC);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            soundPool = createNewSoundPool();
        } else {
            soundPool = createOldSoundPool();
        }

        //new SoundPool(10, AudioManager.STREAM_MUSIC, 0);

        soundPool.setOnLoadCompleteListener(new OnLoadCompleteListener() {
            @Override
            public void onLoadComplete(SoundPool soundPool, int sampleId, int status) {
                    loaded = true;
                    playLoop();
            }
        });


    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    private SoundPool createNewSoundPool(){
        AudioAttributes aAttr = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                .build();
        return (new SoundPool.Builder().setAudioAttributes(aAttr).build());
    }

    @SuppressWarnings("deprecation")
    @TargetApi(Build.VERSION_CODES.FROYO)
    protected SoundPool createOldSoundPool(){

        return (new SoundPool(5,AudioManager.STREAM_MUSIC,0));
    }

    @Override
    protected void onPause()
    {
        pauseSound();
        TextView dtf = ((TextView)findViewById(R.id.dateTimeField));
        if(updateTimer != null){
        	updateTimer.cancel();
        	
        }
        mView.onPause();
        super.onPause();
    }

    @Override
    protected void onResume()
    {
    	super.onResume();
    	mView.onResume();
    	updateTimer = new Timer();
    	updateTimer.schedule(
    			new TimerTask(){
    				public void run(){
    					TextView dtf = ((TextView)findViewById(R.id.dateTimeField));
    					dtf.post(setDateTime);
    					}
    				}, 0, 500);

        soundID = soundPool.load(this, R.raw.tone, 1);
    }

   @Override
   protected void onDestroy(){
	   stopSound();
       soundPool.release();
	   super.onDestroy();
   }
    
    @SuppressLint("NewApi")
    @Override
    public void onWindowFocusChanged(boolean hasFocus)
    {
        super.onWindowFocusChanged(hasFocus);
        if( hasFocus && Build.VERSION.SDK_INT >= 19 )
        {
            getWindow().getDecorView().setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION // hide nav bar
                            | View.SYSTEM_UI_FLAG_FULLSCREEN // hide status bar
                            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        }
    }
    
    public void playSound() {
		// Is the sound loaded does it already play?
		if (loaded && !plays) {
			soundPool.play(soundID, volume, volume, 1, 0, 1f);
			plays = true;
		}
	}

    public void playLoop() {
		// Is the sound loaded does it already play?
		if (loaded && !plays) {
			// the sound will play for ever if we put the loop parameter -1
			soundPool.play(soundID, volume, volume, 1, -1, 1f);
			plays = true;
		}
	}
    public void pauseSound() {
		if (plays) {
			soundPool.pause(soundID);
			plays = false;
        }
	}

	public void stopSound() {
		if (plays) {
			soundPool.stop(soundID);
			plays = false;
		}
	}
}
