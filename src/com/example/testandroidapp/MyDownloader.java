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

	private native void writeCallback(byte buffer[], int size, long args);
	private native void progressCallback(int sizeTotal, int sizeCurr, long args);
	private native boolean isShutdown();
	
	public MyDownloader(){}
	
	public int download(String sUrl, int custom_timeout_connection, int custom_timeout_recieve, long args) {
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
				} catch(IOException e) {
					connection.disconnect();
					counter += timeout;
					continue;
				}
				counter = 0;
				responseCode = connection.getResponseCode();
				
			} while (responseCode != HttpURLConnection.HTTP_OK && counter <= custom_timeout_connection);
			
			if (counter > custom_timeout_connection){
				return status;
			}
			
			counter = 0;
			InputStream inputStream = connection.getInputStream();
			
			int bytesRead = 0;
	        int currentBytes = 0;
	        byte[] buffer = new byte[BUFFER_SIZE];
	        status = DOWNLOADER_STATUS_OK;
	        boolean shutdown = isShutdown();
			while ((counter <= custom_timeout_recieve) && (bytesRead != -1) && !(shutdown)) {	
				try {
					bytesRead = inputStream.read(buffer);
					currentBytes += bytesRead;
		            progressCallback(contentLength, currentBytes, args);
		            writeCallback(buffer, bytesRead, args);
				} catch(Exception e) {
					Log.e(TAG, e.toString());
					status = DOWNLOADER_STATUS_ERROR;
					counter += timeout;
					continue;
				}
				counter = 0;
				status = DOWNLOADER_STATUS_OK;
				shutdown = isShutdown();
			}
			
			if (shutdown){
            	status = DOWNLOADER_STATUS_ERROR;
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
}