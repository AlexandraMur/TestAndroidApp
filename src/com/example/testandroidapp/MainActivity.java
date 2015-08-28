package com.example.testandroidapp;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;

public class MainActivity extends Activity{
	private Thread mThread;
	private MyDownloader mDownloader;
	private static final String TAG = "MainActivity";
	private long args;
	
	static {
		System.loadLibrary("test");
	}
	
	private native void nativeStartDownloading();
	private native void nativeStopDownloading();
	private native int nativeInit();
	private native void nativeDeinit();
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		Log.d(TAG, "onCreate");
	}
	
	@Override
	protected void onResume(){
		super.onResume();
		Log.d(TAG, "onResume");
		
		nativeInit();
		setContentView(com.example.testandroidapp.R.layout.activity_main);
		
		View download_button = findViewById(com.example.testandroidapp.R.id.button1);
		download_button.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				Log.d(TAG, "Button DOWNLOAD pushed");
				nativeStartDownloading();
			}
		});
		
		View stop_button = findViewById(com.example.testandroidapp.R.id.button2);
		stop_button.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				Log.d(TAG, "Button STOP pushed");
				nativeStopDownloading();
			}
		});
	}
	
	@Override
	protected void onPause(){
		super.onPause();
		Log.d(TAG, "onPause");
		nativeDeinit();
	}
}
