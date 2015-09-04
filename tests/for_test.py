import os
import subprocess
import threading

process = 0

def stop_server():
	os.killpg(process.pid, signal.SIGTERM)

def main():
	thread = threading.Timer(10.0, stop_server)
	thread.start();
	process = subprocess.call('python -m SimpleHTTPServer 8080');

if __name__ == '__main__':
	main()
