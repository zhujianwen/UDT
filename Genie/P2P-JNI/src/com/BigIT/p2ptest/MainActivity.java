package com.BigIT.p2ptest;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.app.Activity;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.CheckBox;
import android.widget.TextView;
import android.widget.EditText;
import android.widget.Button;

import com.BigIT.p2p.p2p;
import com.BigIT.p2p.p2p.Callback;

public class MainActivity extends Activity implements Callback {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		
        Text_info = (TextView) this.findViewById(R.id.textView1);
        Text_info.setText("Press Init Button");
        Text_own = (EditText) this.findViewById(R.id.editText1);
        Text_own.setText("liuhong");
        Text_tar = (EditText) this.findViewById(R.id.editText2);
        Text_tar.setText("zhujianwen");
        Text_path = (EditText) this.findViewById(R.id.editText3);
        Text_path.setText("input message or file path");
        
        Btn_init = (CheckBox) this.findViewById(R.id.checkBox1);
        Btn_init.setOnClickListener(new OnClickListener(){
			@Override
			public void onClick(View arg0) {
				if (Btn_init.isChecked())
				{
					//mCore.init("54.254.169.186", 3456, "54.254.169.186", 3478,"zhujianwen","123456");
					mCore.init("54.254.169.186", 3456, "54.254.169.186", 3478,"liuhong","123456");
				}
				else
				{
					mCore.p2pclose();
				}
				
			}});
        
        Btn_conn = (CheckBox) this.findViewById(R.id.checkBox2);
        Btn_conn.setOnClickListener(new OnClickListener(){
			@Override
			public void onClick(View arg0) {				
				szTmp = Text_tar.getText().toString();
				if (Btn_init.isChecked())
				{
					mCore.p2pconnect("zhujianwen");
				}
				else
				{
					mCore.p2pdisconnect("zhujianwen");
				}
				
			}});
        
        Btn_sndm = (Button) this.findViewById(R.id.button3);
        Btn_sndm.setOnClickListener(new OnClickListener(){
			@Override
			public void onClick(View arg0) {
				szTmp = Text_path.getText().toString();
				byte[] data = szTmp.getBytes();
				
				if (mCore.connected)
					mCore.p2psend("zhujianwen", data, data.length);
				else
					udtsock = mCore.p2pconnect("zhujianwen");
			}});
        
        Btn_sndf = (Button) this.findViewById(R.id.button4);
        Btn_sndf.setOnClickListener(new OnClickListener(){
			@Override
			public void onClick(View arg0) {
				String msg = "android hello!";
				
				byte[] data = msg.getBytes();
				
				if (mCore.connected)
					mCore.p2psend("zhujianwen", data, data.length);
				else
					udtsock = mCore.p2pconnect("zhujianwen");
			}});
		
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}
	
    // callback
    public void onMessage(String szMsg, int Type)
	{
		szTmp = "onMessage:" + szMsg;
		handler.sendEmptyMessage(1);
	}
	public void onAccept(String szIpAddr, String szHostName, String szSendType, String szFileName, int nFileCount)
	{
		szTmp = "onAccept:" + szFileName;
		handler.sendEmptyMessage(1);
	}
	public void onTransfer(long nFileTotalSize, long nCurrent, String szFileName, int Type)
	{
		szTmp = "onTransfer total size:" + nFileTotalSize + ", nCurrent:" + nCurrent;
		handler.sendEmptyMessage(1);
	}
	
	private Handler handler = new Handler()
	{
		public void handleMessage(Message msg)
		{
			switch (msg.what)
			{
			case 1:
				Text_info.setText(szTmp);
				break;

			default:
				break;
			}
		};
	};
	
    p2p mCore = new p2p(this);
    int udtsock = 0;
    public String szTmp="";
    private TextView Text_info;
    private EditText Text_own;
	private EditText Text_tar;
	private EditText Text_path;
    private CheckBox Btn_init;
    private CheckBox Btn_conn;
    private Button Btn_sndm;
    private Button Btn_sndf;

}
