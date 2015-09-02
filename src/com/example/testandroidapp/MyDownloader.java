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
	private static final int DOWNLOADER_STATUS_URL_ERROR = 1;
	private static final int DOWNLOADER_STATUS_CONNECTION_TIMEOUT_ERROR = 2;
	private static final int DOWNLOADER_STATUS_RECIEVEDATA_TIMEOUT_ERROR = 3;
	private static final String TAG = "MyDownloader";
	private static final int BUFFER_SIZE = 4096;

	private native void writeCallback(byte buffer[], int size, long args);
	private native void progressCallback(int sizeTotal, int sizeCurr, long args);
	private native boolean isShutdown(long args);
	
	public MyDownloader(){}
	
	public int download(String sUrl, int customTimeoutConnection, int customTimeoutRecieve, long args) {
		int status = DOWNLOADER_STATUS_ERROR;
		int customTimeoutConnectionInMillsec = customTimeoutConnection * 1000;
		int customRecieveConnectionInMillsec = customTimeoutRecieve * 1000;
		try {
			URL url = new URL(sUrl);
			HttpURLConnection connection = (HttpURLConnection)url.openConnection();
			
			int contentLength = 0;
			String values = connection.getHeaderField("Content-Length");
			
			if (values != null && !values.isEmpty()) {
				contentLength = Integer.parseInt(values);
			}			
			
			int counter = 0;
			int timeout = 1000 * 1;
			connection.setConnectTimeout(timeout);
			connection.setReadTimeout(timeout);
			int responseCode = HttpURLConnection.HTTP_OK;
			
			boolean shutdown = isShutdown(args);
			if (shutdown) {
				return DOWNLOADER_STATUS_CONNECTION_TIMEOUT_ERROR;
			}
			
			do {
				try {
					connection.connect();
				} catch(IOException e) {
					counter += timeout;
					continue;
				}
				counter = 0;
				responseCode = connection.getResponseCode();
			} while (responseCode != HttpURLConnection.HTTP_OK && counter <= customTimeoutConnectionInMillsec && !isShutdown(args));
				
			if (counter > customTimeoutConnectionInMillsec) {
				status = DOWNLOADER_STATUS_CONNECTION_TIMEOUT_ERROR;
				return status;
			}
			
			counter = 0;
			InputStream inputStream = connection.getInputStream();
			
			int bytesRead = 0;
	        int currentBytes = 0;
	        byte[] buffer = new byte[BUFFER_SIZE];
	        shutdown = isShutdown(args);
			while ((counter <= customRecieveConnectionInMillsec) && (bytesRead != -1) && !(shutdown)) {	
				try {
					bytesRead = inputStream.read(buffer);
					currentBytes += bytesRead;
		            progressCallback(contentLength, currentBytes, args);
		            writeCallback(buffer, bytesRead, args);
				} catch(Exception e) {
					Log.e(TAG, e.toString());
					shutdown = isShutdown(args);
					status = DOWNLOADER_STATUS_RECIEVEDATA_TIMEOUT_ERROR;
					counter += timeout;
					continue;
				}
				counter = 0;
				status = DOWNLOADER_STATUS_OK;
				shutdown = isShutdown(args);
			}
			
			if (shutdown){
            	status = DOWNLOADER_STATUS_OK;
			}
			
	        inputStream.close();
			connection.disconnect();
		} catch (MalformedURLException urlErr){
			Log.e(TAG, urlErr.toString());
			status = DOWNLOADER_STATUS_URL_ERROR;
		} catch (IOException ioErr) {
			Log.e(TAG, ioErr.toString());
			status = DOWNLOADER_STATUS_ERROR;
        } catch (Exception err) {
        	Log.e(TAG, err.toString());
        	status = DOWNLOADER_STATUS_ERROR;
        }
		return status;
	}
}