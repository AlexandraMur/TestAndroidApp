package com.example.testandroidapp;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import android.util.Log;

public class MyDownloader implements Runnable {
	private static final String TAG = "MyDownloader";
	private static final int BUFFER_SIZE = 4096;
	private String mUrl;
	
	
	static {
		System.loadLibrary("callbacks");
	}
	
	private native void writeCallback(byte buffer[], int size);
	private native void progressCallback(int sizeTotal, int sizeCurr);
	
	MyDownloader() {
		mUrl = null;
	}
	
	private void download(String sUrl) {
		
		try {
			URL url = new URL(sUrl);
			HttpURLConnection connection = (HttpURLConnection)url.openConnection();
			//throws exception. rewrite
			//int contentLength = (int)Long.parseLong(connection.getHeaderField("Content-Length"));
			connection.setConnectTimeout(1000 * 2);
			connection.setReadTimeout(1000 * 2);
	        connection.connect();
	        
	        int responseCode = connection.getResponseCode();
	        if (responseCode != HttpURLConnection.HTTP_OK) {
	        	connection.disconnect();
	        	Log.e(TAG,"Error connection");
	        	return;
	        }
	        
	        InputStream inputStream = connection.getInputStream();
	    	ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
	        
	        int bytesRead = -1;
	        int currentBytes = 0;
	        
	        byte[] buffer = new byte[BUFFER_SIZE];
	        while ((bytesRead = inputStream.read(buffer)) != -1) {
	        	currentBytes += bytesRead;
	            outputStream.write(buffer, 0, bytesRead);
	            progressCallback(/*contentLength*/0, currentBytes);
	        }
	        
	        outputStream.close();
	        writeCallback(outputStream.toByteArray(), currentBytes);
	        
	        inputStream.close();
			connection.disconnect();
		} catch (NullPointerException nullErr){
			Log.e(TAG, nullErr.toString());
		} catch (SecurityException secErr){
			Log.e(TAG, secErr.toString());
		} catch (MalformedURLException urlErr){
			Log.e(TAG, urlErr.toString());
		} catch (IOException ioErr) {
			Log.e(TAG, ioErr.toString());
        } catch (Exception err) {
        	Log.e(TAG, err.toString());
        }
       
	}

	@Override
	public void run() {
		this.download(mUrl);
	}
	
	public void setUrl(String url) {
		mUrl = url;	
	}
}