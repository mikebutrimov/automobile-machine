package org.unhack.project_am_tests_android;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import com.maxmpz.poweramp.player.PowerampAPI;


import java.util.HashMap;
import java.util.Iterator;


public class MainActivity extends AppCompatActivity {
    private static final String ACTION_USB_PERMISSION = "com.android.example.USB_PERMISSION";
    public PendingIntent mPermissionIntent;
    public HashMap<String, UsbDevice> availableDrivers;
    public UsbManager manager = null;
    public UsbDevice mUsbDevice;
    public int ArduinoId = 9025;

    private final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (ACTION_USB_PERMISSION.equals(action)) {
                synchronized (this) {
                    UsbDevice device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);

                    if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                        if(device != null){
                            Intent startServiceIntent = new Intent(getApplicationContext(),SerialIOService.class);
                            stopService(startServiceIntent);
                            startService(startServiceIntent);
                            finish();
                        }
                    }
                    else {
                        Log.d("OLOLO BLAD", "permission denied for device " + device);
                    }
                }
            }
        }
    };




    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        //set onClickListener for some buttons
        Button button_up = (Button) findViewById(R.id.button_up);
        button_up.setOnClickListener(mOnClickListener);
        Button button_down = (Button) findViewById(R.id.button_down);
        button_down.setOnClickListener(mOnClickListener);
        Button button_left = (Button) findViewById(R.id.button_left);
        button_left.setOnClickListener(mOnClickListener);
        Button button_right = (Button) findViewById(R.id.button_right);
        button_right.setOnClickListener(mOnClickListener);
        Button button_menu = (Button) findViewById(R.id.button_menu);
        button_menu.setOnClickListener(mOnClickListener);
        Button button_exit = (Button) findViewById(R.id.button_exit);
        button_exit.setOnClickListener(mOnClickListener);
        Button button_ok = (Button) findViewById(R.id.button_ok);
        button_ok.setOnClickListener(mOnClickListener);
        Button button_hon = (Button) findViewById(R.id.button_hon);
        button_hon.setOnClickListener(mOnClickListener);
        Button button_clear = (Button) findViewById(R.id.button_clear);
        button_clear.setOnClickListener(mOnClickListener);
        Button button_pamp_1 = (Button) findViewById(R.id.button_pamp_1);
        button_pamp_1.setOnClickListener(mOnClickListener);
        Button button_pamp_2 = (Button) findViewById(R.id.button_pamp_2);
        button_pamp_2.setOnClickListener(mOnClickListener);
        Button button_pamp_3 = (Button) findViewById(R.id.button_pamp_3);
        button_pamp_3.setOnClickListener(mOnClickListener);


        mPermissionIntent = PendingIntent.getBroadcast(this, 0, new Intent(ACTION_USB_PERMISSION), 0);
        IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
        registerReceiver(mUsbReceiver, filter);


        manager = (UsbManager) getSystemService(Context.USB_SERVICE);
        availableDrivers = manager.getDeviceList();
        //pass throug all usb devices and search for arduino
        //a little bit narkomansky way to find correct device
        //but why not?

        if (availableDrivers != null) {
            Iterator<UsbDevice> deviceIterator = availableDrivers.values().iterator();
            while (deviceIterator.hasNext()) {
                UsbDevice device = deviceIterator.next();
                if (device.getVendorId() == ArduinoId && !manager.hasPermission(device)) {
                    mUsbDevice = device;
                    manager.requestPermission(mUsbDevice, mPermissionIntent);
                    break;

                }
            }
        }

        Intent startServiceIntent = new Intent(getApplicationContext(),SerialIOService.class);
        PendingIntent pStartServiceIntent = PendingIntent.getService(this, 0, startServiceIntent, 0);
        AlarmManager alarm = (AlarmManager) getSystemService(Context.ALARM_SERVICE);
        long timeToExec = System.currentTimeMillis()+6000;
        alarm.setExact(AlarmManager.RTC_WAKEUP,timeToExec,pStartServiceIntent);
        Toast.makeText(this,"Alarm was set on +6000ms",Toast.LENGTH_SHORT).show();

    }


    //onClick listener
    private View.OnClickListener mOnClickListener = new View.OnClickListener() {
        public void onClick(View v) {
            switch (v.getId()) {
                case R.id.button_up:
                    push_command("UpButton");
                    break;
                case R.id.button_down:
                    push_command("DownButton");
                    break;
                case R.id.button_left:
                    push_command("LeftButton");
                    break;
                case R.id.button_right:
                    push_command("RightButton");
                    break;
                case R.id.button_menu:
                    push_command("Menu");
                    break;
                case R.id.button_exit:
                    push_command("Esc");
                    break;
                case R.id.button_ok:
                    push_command("Ok");
                    break;
                case R.id.button_hon:
                    push_command("HeadUnitOn");
                    break;
                case R.id.button_clear:
                    push_command("Clear");
                    break;
                case R.id.button_pamp_1:
                    startService(new Intent(PowerampAPI.ACTION_API_COMMAND).putExtra(PowerampAPI.COMMAND, PowerampAPI.Commands.TOGGLE_PLAY_PAUSE).setPackage(PowerampAPI.PACKAGE_NAME));
                    break;
                case R.id.button_pamp_2:
                    startService(new Intent(PowerampAPI.ACTION_API_COMMAND).putExtra(PowerampAPI.COMMAND, PowerampAPI.Commands.PREVIOUS).setPackage(PowerampAPI.PACKAGE_NAME));
                    break;
                case R.id.button_pamp_3:
                    startService(new Intent(PowerampAPI.ACTION_API_COMMAND).putExtra(PowerampAPI.COMMAND, PowerampAPI.Commands.NEXT).setPackage(PowerampAPI.PACKAGE_NAME));
                    break;

            }
        }
    };



    public void push_command(String cmd){
        Intent cmdIntent = new Intent(this,SerialIOService.class);
        cmdIntent.putExtra("INCMD",cmd);
        startService(cmdIntent);
    }

    public void StopService(View v){
        Intent intent = new Intent(this,SerialIOService.class);
        stopService(intent);
    }


    @Override
    public void onDestroy() {
        unregisterReceiver(mUsbReceiver);
        super.onDestroy();
    }

}
