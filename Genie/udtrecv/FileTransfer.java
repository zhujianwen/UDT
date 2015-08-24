package com.dragonflow;

public class FileTransfer {

	private String receiveFolder;
	private String response;
	
	public FileTransfer(){
	}
	
	public void OnDebug(String strOutput)
	{
		System.out.println("------OnDebug:" + strOutput);
	}
	
	//接受 单个 文件传送请求时，回调
    public void onAccept(String host, String DeviceName, String SendType, String filename, int count){
    	System.out.println("------onAccept:" + host + ":" + filename);
    }
    
    //接受 多个 文件传送请求时，回调
    public void onAcceptFolder(String host, String DeviceName, String SendType, String filename, int count){
    	
    }
    
    // 文件传送完毕时回调文件名称 
    public void onAcceptonFinish(String host, Object[] fileNameList){
    	System.out.println("------onAccept:" + host + ":");
    	
    }
    
    //传送结束时回调
    // msg: 失败-"FAIL", 成功-"SUCCESS", 停止-"STOP"
    public void onFinished(String msg){
    	System.out.println("------onFinished:" + msg);
    }
    
    //传送文件中回调，  1：停止接收  2：停止发送
    public void onTransfer(long sum,long current,String filename){
    	System.out.println("------onTransfer:" + sum + ":" + current);
    }
    
    //接受消息时回调
    public void onRecvMessage(String msg,String host,String hostname){
    	System.out.println("------onRecvMessage:" + host + ":" + msg);
    	response = msg;
    }
    
    public String getText(){
    	return response;
    }
    
    //开始接受文件传送
    public native int listenFileSend(int port);
    //停止接受文件传送
    public native int stopListen();
    //发送消息
    public native int sendMessage(String host,int port,String message, String hostname, String sendtype);
    //发送文件
    public native int sendFile(String host,int port,String filename, String hostname, String sendtype);
    //发送多个文件 
    public native int sendMultiFiles(String host, int port, Object[] filelist, String hostname, String sendtype);
    //发送文件夹
    public native int sendfolder(String host, int port, String folderName, String hostname, String sendtype);
    //停止文件传送、接收
    public native int stopTransfer(int type);
    //是否接受文件  "REJECT", "REJECTBUSY", "/mnt/sdcard/";
    public native int replyAccept(String strReply);

    static {
        System.loadLibrary("udt");
    }
}
