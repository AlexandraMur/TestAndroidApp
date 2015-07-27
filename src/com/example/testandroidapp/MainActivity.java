package com.example.testandroidapp;

import android.app.Activity;
import android.os.Bundle;
import com.example.testandroidapp.MyDownloader;

public class MainActivity extends Activity {
	private Thread mThread;
	private MyDownloader mDownloader;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		mDownloader = new MyDownloader();
		mDownloader.setUrl("http://stackoverflow.com/questions/2250112/why-doesnt-logcat-show-anything-in-my-android");
		
		mThread = new Thread(mDownloader);
		mThread.start();
	}
}
