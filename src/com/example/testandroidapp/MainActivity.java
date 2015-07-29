package com.example.testandroidapp;

import android.R;
import android.app.Activity;
import android.os.Bundle;
import com.example.testandroidapp.MyDownloader;

public class MainActivity extends Activity {
	private Thread mThread;
	private MyDownloader mDownloader;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_list_item);
		
		mDownloader = new MyDownloader();
		mDownloader.setUrl("https://ru.wikipedia.org/wiki/SSE2");
		
		mThread = new Thread(mDownloader);
		mThread.start();
	}
}
