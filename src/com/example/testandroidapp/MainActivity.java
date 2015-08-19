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
	
	private native void startDownloading(long args);
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(com.example.testandroidapp.R.layout.activity_main);
		
		View button1 = findViewById(com.example.testandroidapp.R.id.button1);
		button1.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				Log.d(TAG, "Button DOWNLOAD pushed");
				startDownloading(args);
			}
		});
		
		View button2 = findViewById(com.example.testandroidapp.R.id.button2);
		button2.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				Log.d(TAG, "Button STOP pushed");
			}
		});
	}
	
}
