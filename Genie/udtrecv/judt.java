package com.dragonflow;

public class judt {
	
	
	public native int startup();
	
	public native int cleanup();
	//return socket
	public native int listen(int port);
	
	public native int accept(int sock);
	public native char[] recv(int sock, int size);
	public native int	recvInt(int sock);
	public native long recvLong(int sock);
	
	public native int send(int sock, char[] buf, int size);
	public native int sendInt(int sock,int data);
	public native int sendLong(int sock, long data);
	//return socket
	public native int connect(String host,int port);
	
	/**
	 * @param args
	 */
	public static void main(String[] args) {


	}

}
