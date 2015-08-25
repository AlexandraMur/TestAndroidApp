package com.example.testandroidapp;

import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;

import android.util.Log;

public class MyDownloader {
	private static final int DOWNLOADER_STATUS_ERROR = -1;
	private static final int DOWNLOADER_STATUS_OK = 0;
	private static final String TAG = "MyDownloader";
	private static final int BUFFER_SIZE = 4096;

	private native int writeCallback(byte buffer[], int size, long args);
	private native void progressCallback(int sizeTotal, int sizeCurr, long args);
	private int stop;
	
	public MyDownloader(){
		stop = 0;
	}
	
	public int download(String sUrl, int custom_timeout, long args) {
		int status = DOWNLOADER_STATUS_ERROR;
		try {
			URL url = new URL(sUrl);
			HttpURLConnection connection = (HttpURLConnection)url.openConnection();
			
			int contentLength = 0;
			String values = connection.getHeaderField("Content-Length");
			
			if (values != null && !values.isEmpty()) {
				contentLength = Integer.parseInt(values);
			}			
			
			int counter = 0;
			int timeout = 1000 * 2;
			connection.setConnectTimeout(timeout);
			connection.setReadTimeout(timeout);
			int responseCode = HttpURLConnection.HTTP_OK;
			
			do {
				try {
					connection.connect();
				}catch(IOException e){
					connection.disconnect();
					counter += custom_timeout;
					continue;
				}
				counter = 0;
				responseCode = connection.getResponseCode();
				
			} while (responseCode != HttpURLConnection.HTTP_OK && counter <= custom_timeout);
			
			if (counter > custom_timeout){
				return status;
			}
			
			counter = 0;
			InputStream inputStream = connection.getInputStream();
			
			int bytesRead = 0;
	        int currentBytes = 0;
	        byte[] buffer = new byte[BUFFER_SIZE];
	        status = DOWNLOADER_STATUS_OK;
	        
			while ((counter <= custom_timeout) && (bytesRead != -1)) {
				
				if (stop == 1){
	            	Log.i(TAG, "DISCONNECTED");
	            	status = DOWNLOADER_STATUS_ERROR;
	            	break;
	            }
				
				try {
					bytesRead = inputStream.read(buffer);
					currentBytes += bytesRead;
		            progressCallback(contentLength, currentBytes, args);
		            writeCallback(buffer, bytesRead, args);
				} catch(Exception e){
					Log.e(TAG, e.toString());
					status = DOWNLOADER_STATUS_ERROR;
					counter += custom_timeout;
					continue;
				}
				counter = 0;
				status = DOWNLOADER_STATUS_OK;
			}
				
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
		return status;
	}
	
	public void disconnect(){
		stop = 1;
	}
}