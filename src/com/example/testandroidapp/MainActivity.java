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
	
	private native void nativeStartDownloading(long args);
	private native void nativeStopDownloading(long args);
	private native long nativeInit();
	private native void nativeDeinit(long args);
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		Log.d(TAG, "onCreate");
		
		setContentView(com.example.testandroidapp.R.layout.activity_main);
		View download_button = findViewById(com.example.testandroidapp.R.id.download);

		download_button.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				Log.d(TAG, "Button DOWNLOAD pushed");
				nativeStartDownloading(args);
			}
		});
		
		View stop_button = findViewById(com.example.testandroidapp.R.id.stop);
		stop_button.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				Log.d(TAG, "Button STOP pushed");
				nativeStopDownloading(args);
			}
		});
	}
	
	@Override
	protected void onResume(){
		super.onResume();
		Log.d(TAG, "onResume");
		this.args = nativeInit();
	}
	
	@Override
	protected void onPause(){
		super.onPause();
		Log.d(TAG, "onPause");
		nativeDeinit(this.args);
	}
}
