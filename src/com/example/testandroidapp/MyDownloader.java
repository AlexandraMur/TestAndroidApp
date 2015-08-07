package com.example.testandroidapp;

import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;

import android.util.Log;

public class MyDownloader {
	private static final String TAG = "MyDownloader";
	private static final int BUFFER_SIZE = 4096;

	private native void writeCallback(int size, long args);
	private native void progressCallback(byte buffer[], int sizeTotal, int sizeCurr, long args);
	
	public MyDownloader(){}
	
	public void download(String sUrl, long args) {
		Log.d(TAG,"Download function");
		
		try {
			URL url = new URL(sUrl);
			HttpURLConnection connection = (HttpURLConnection)url.openConnection();
			
			int contentLength = 0;
			String values = connection.getHeaderField("Content-Length");
			
			if (values != null && !values.isEmpty()) {
				contentLength = Integer.parseInt(values);
			}			
			
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
	        
	        int bytesRead = -1;
	        int currentBytes = 0;
	        
	        byte[] buffer = new byte[BUFFER_SIZE];
	        while ((bytesRead = inputStream.read(buffer)) != -1) {
	        	currentBytes += bytesRead;
	            progressCallback(buffer, contentLength, currentBytes, args);
	        }
	        writeCallback(currentBytes, args);
	        
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
}