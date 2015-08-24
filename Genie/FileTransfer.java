package com.dragonflow;

import android.os.Handler;

public class FileTransfer {
	
	public interface Callback
	{		
		public abstract void onAccept(String szIpAddr, String szHostName, String szSendType, String szFileName, int nFileCount);
		public abstract void onSendFinished(String szMsg);
		public abstract void onRecvFinished(String sztrMsg);
		public abstract void onSendTransfer(long nFileTotalSize, long nCurrent, String szFileName);
		public abstract void onRecvTransfer(long nFileTotalSize, long nCurrent, String szFileName);
		public abstract void onRecvMessage(String szIpAddr, String szHostName, String szMsg);
	}
	
	
	public FileTransfer(Callback callback)
	{
		mCallback = callback;
		mCore = init(this);
	}
	public void ListenFileSend(int port)
	{
		listenFileSend(mCore, port);
	}
	public void SendMessage(String host,int port,String message, String hostname, String sendtype){
		sendMessage(mCore, host, port, message, hostname, sendtype);
	}
	public void SendUniFile(String host, int port, String folderName, String hostname, String sendtype){
		sendFile(mCore, host, port, folderName, hostname, sendtype);
	}
	public void SendMultiFiles(String host, int port, Object[] filelist, String hostname, String sendtype){
		sendMultiFiles(mCore, host, port, filelist, hostname, sendtype);
	}
	public void SendFolder(String host, int port, String folderName, String hostname, String sendtype){
		sendfolder(mCore, host, port, folderName, hostname, sendtype);
	}
	public void ReplyAccept(String strReply)
	{
		replyAccept(mCore, strReply);
	}
	public void StopTransfer(int nType)
	{
		stopTransfer(mCore, nType);
	}
	public void StopListen()
	{
		stopListen(mCore);
	}
	
	
	// 调试信息，回调
	public void onDebug(String strOutput)
	{
		System.out.println("------OnDebug:" + strOutput);
	}
	
	//接受 单个 文件传送请求时，回调
	public void onAccept(String host, String DeviceName, String SendType, String filename, int count)
	{
		System.out.println("------onAccept:" + host + ":" + filename);
		FileTransfer.this.hook_onAccept(host, DeviceName, SendType, filename, count);
	}
	void hook_onAccept(String host, String DeviceName, String SendType, String filename, int count)
	{
		if (mCallback != null) {
			mCallback.onAccept(host, DeviceName, SendType, filename, count);
		}
	}
	
    //接受 多个 文件传送请求时，回调
	public void onAcceptFolder(String host, String DeviceName, String SendType, String filename, int count)
	{
		System.out.println("------onAccept:" + host + ":" + filename);
		FileTransfer.this.hook_onAcceptFolder(host, DeviceName, SendType, filename, count);
    }
	void hook_onAcceptFolder(String host, String DeviceName, String SendType, String filename, int count)
	{
		if (mCallback != null) {
			mCallback.onAccept(host, DeviceName, SendType, filename, count);
		}
	}
	
    // 文件传送完毕时回调文件名称 
    public void onAcceptonFinish(String host, Object[] fileNameList)
    {
    	System.out.println("------onAccept:" + host + ":");
    	FileTransfer.this.hook_onAcceptonFinish(host, fileNameList);
    }
    void hook_onAcceptonFinish(String host, Object[] fileNameList)
	{

	}
    //传送结束时回调
    // msg: 失败-"FAIL", 成功-"SUCCESS", 停止-"STOP"
    public void onFinished(String msg)
    {
    	System.out.println("------onFinished:" + msg);
    	FileTransfer.this.hook_onFinished(msg);
    }
    void hook_onFinished(String msg)
    {
    	if (mCallback != null)
    	{
    		mCallback.onRecvFinished(msg);
    	}
    }
    
    //传送文件中回调，  1：停止接收  2：停止发送
    public void onTransfer(long sum,long current,String filename)
    {
    	System.out.println("------onTransfer:" + sum + ":" + current);
    	FileTransfer.this.hook_onTransfer(sum, current, filename);
    }
    void hook_onTransfer(long sum,long current,String filename)
    {
    	if (mCallback != null)
    	{
    		mCallback.onRecvTransfer(sum, current, filename);
    	}
    }
    
    //接受消息时回调
    public void onRecvMessage(String msg, String host,String hostname)
    {
    	FileTransfer.this.hook_onRecvMessage(host, hostname, msg);
    }
    void hook_onRecvMessage(String szIpAddr, String szHostName, String szMsg)
    {
    	System.out.println("------onRecvMessage:" + szIpAddr + ":" + szMsg);
		if (mCallback != null) {
			mCallback.onRecvMessage(szIpAddr, szHostName, szMsg);
		}
    }
    
    
    // 初始化
	private static native long init(Object delegate);
	// 结束
	private static native void destroy(long core);
    //开始接受文件传送
	private static native int listenFileSend(long core, int port);
    //发送消息
	private static native int sendMessage(long core, String host,int port,String message, String hostname, String sendtype);
    //发送文件
	private static native int sendFile(long core, String host,int port,String filename, String hostname, String sendtype);
    //发送多个文件 
	private static native int sendMultiFiles(long core, String host, int port, Object[] filelist, String hostname, String sendtype);
    //发送文件夹
	private static native int sendfolder(long core, String host, int port, String folderName, String hostname, String sendtype);
	// 是否接受文件  "REJECT", "REJECTBUSY", "/mnt/sdcard/";
	private static native int replyAccept(long core, String strReply);
    //停止文件传送、接收
	private static native int stopTransfer(long core, int type);
	// 停止接受文件传送
	private static native int stopListen(long core);
    
	Callback mCallback = null;
	boolean mDisposed = false;
	boolean mStarted = false;
	Handler mDisp = new Handler();
	long mCore = 0;
	private String receiveFolder;
	private String response;

    static {
        System.loadLibrary("udt");
    }
}
