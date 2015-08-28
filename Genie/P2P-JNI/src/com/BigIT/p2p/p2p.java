package com.BigIT.p2p;


public class p2p
{
	public interface Callback
	{		
		public abstract void onMessage(String szMsg, int Type);
		public abstract void onAccept(String szIpAddr, String szHostName, String szSendType, String szFileName, int nFileCount);
		public abstract void onTransfer(long nFileTotalSize, long nCurrent, String szFileName, int Type);
	}
	
	public p2p(Callback callback)
	{
		mCallback = callback;
	}
	
	public void init(String ctlserv, int ctlport, String stunsrv,int stunport,String name, String passwd)
	{
		p2pinit(this, ctlserv, ctlport, stunsrv, stunport, name, passwd);
	}
	
	public  void on_init(int err)
	{
		if (mCallback != null){
			mCallback.onMessage("onInit ok", 0);
		}
		System.out.println("oninit ok");
	}
	
	public  void on_request(String peername)
	{
		p2paccept(peername);
		if (mCallback != null){
			String szTmp = "p2p request from: "+peername;
			mCallback.onMessage(szTmp, 0);
		}
		System.out.println("p2p request from:" + peername);
	}
	
	public  void on_connect(int err, String peername){
		String szTmp = "";
		if (err == 0){
			connected = true;
			szTmp = "connect to "+peername+" ok!";
		}else
			szTmp = "connect to "+peername+" failed!";
		
		System.out.println(szTmp);
		if (mCallback != null){
			mCallback.onMessage(szTmp, err);
		}
	}
	
	public  void on_receive(String peername, byte[] data, int len){
		String str= new String(data);
		str = "p2p receive data:"+str;
		if (mCallback != null){
			mCallback.onMessage(str, len);
		}
		System.out.println(str+str);
	}
	
	public  void on_close(String peername){
		String str = "p2p peername close:"+peername;
		if (mCallback != null){
			mCallback.onMessage(str, 0);
		}
		System.out.println("p2p receive:" +str);
	}
	
	public native  int p2pinit(Object delegate, String ctlserv, int ctlport,String stunsrv,int stunport,String name, String passwd);
	public native  int p2pconnect(String peername);
	public native  int p2paccept(String peername);
	public native  int p2psend(String peername, byte[] data, int len);
	public native  int p2pdisconnect(String peername);
	public native  int p2pclose();
	
	Callback mCallback = null;
	long mCore = 0;
	public static boolean connected;

	static {
		System.loadLibrary("p2p");
	}
}