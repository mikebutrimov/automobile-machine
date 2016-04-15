package org.unhack.project_am_tests_android;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;
import android.widget.Toast;

import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.StringTokenizer;


public class MainActivity extends AppCompatActivity implements AdapterView.OnItemSelectedListener {
    private static final String ACTION_USB_PERMISSION = "com.android.example.USB_PERMISSION";
    public PendingIntent mPermissionIntent;
    public HashMap<String, UsbDevice> availableDrivers;
    public HashMap<String,UsbDevice> devices = new HashMap<>();
    public UsbManager manager = null;
    public UsbDevice mUsbDevice;
    public UsbSerialDriver mUsbSerialDevice = null;
    public CmdStorage mCmdStorage = new CmdStorage();
    public UsbSerialPort port;
    public UsbDeviceConnection connection;

    private final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {

        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (ACTION_USB_PERMISSION.equals(action)) {
                synchronized (this) {
                    UsbDevice device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);

                    if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                        if(device != null){
                            port = init_port(device);
                            //call method to set up device communication
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





        //register receiver
        mPermissionIntent = PendingIntent.getBroadcast(this, 0, new Intent(ACTION_USB_PERMISSION), 0);
        IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
        registerReceiver(mUsbReceiver, filter);

        //probe for devices
        //if one then select as default and make toast
        fillSpinner(getCurrentFocus());
        if (devices.size() == 1){
            mUsbDevice = availableDrivers.get(0);
            selected_dev_toast(this.getCurrentFocus(), mUsbDevice);
        }
    }


    //onClick listener
    private View.OnClickListener mOnClickListener = new View.OnClickListener() {
        public void onClick(View v) {
            switch (v.getId()) {
                case R.id.button_up:
                    push_command(mCmdStorage.getCommand("UpButton"),port);
                    break;
                case R.id.button_down:
                    push_command(mCmdStorage.getCommand("DownButton"),port);
                    break;
                case R.id.button_left:
                    push_command(mCmdStorage.getCommand("LeftButton"),port);
                    break;
                case R.id.button_right:
                    for (byte b: mCmdStorage.getCommand("RightButton")){
                        Log.d("COMMAND BYTES", String.valueOf(b));
                    }
                    push_command(mCmdStorage.getCommand("RightButton"),port);
                    break;
                case R.id.button_menu:
                    push_command(mCmdStorage.getCommand("Menu"),port);
                    break;
                case R.id.button_exit:
                    push_command(mCmdStorage.getCommand("Esc"),port);
                    break;
                case R.id.button_ok:
                    push_command(mCmdStorage.getCommand("Ok"),port);
                    break;
                case R.id.button_hon:
                    byte[] ololo = new byte[]{(byte)113,(byte)1,(byte)0,(byte)0,(byte)0,(byte)0,(byte)0,(byte)0,(byte)0,(byte)0,(byte)0,(byte)0,(byte)0,(byte)0,(byte)114};

                    push_command(mCmdStorage.getCommand("HeadUnitOn"),port);
                    break;
                case R.id.button_clear:
                    push_command(mCmdStorage.getCommand("Clear"),port);
                    break;
            }
        }
    };

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();
        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }
        return super.onOptionsItemSelected(item);
    }


    public void fillSpinner(View v) {
        devices.clear();
        manager = (UsbManager) getSystemService(Context.USB_SERVICE);
        availableDrivers = manager.getDeviceList();
        if (availableDrivers != null) {
            Iterator<UsbDevice> deviceIterator = availableDrivers.values().iterator();
            while(deviceIterator.hasNext()){
                UsbDevice device = deviceIterator.next();
                devices.put(device.getDeviceName(), device);
            }
        }
        List<String> spinnerArray =  new ArrayList<String>();
        for (String device : devices.keySet()) {
            spinnerArray.add(device);
        }
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(
                this, android.R.layout.simple_spinner_item, spinnerArray);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        Spinner sDevices = (Spinner) findViewById(R.id.spinner_devices);
        sDevices.setOnItemSelectedListener(this);
        sDevices.setAdapter(adapter);

    }

    public UsbSerialPort init_port (UsbDevice mUsbDevice) {
        mUsbSerialDevice = UsbSerialProber.getDefaultProber().probeDevice(mUsbDevice);
        UsbSerialPort port = mUsbSerialDevice.getPorts().get(0);
        connection = manager.openDevice(mUsbDevice);
        try {
                port.open(connection);
            } catch (IOException e) {
                e.printStackTrace();
            }
        try {
                port.setParameters(115200, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE);
                //byte buffer[] = new byte[16];
                //int numBytesRead = port.read(buffer, 1000);
                //Toast.makeText(this,"read from buffer: " +buffer.toString(),Toast.LENGTH_LONG).show();
                return port;
            } catch (IOException e) {
                // Deal with error.
            }
        return null;
    }


    public void selected_dev_toast(View v,UsbDevice mUsbDevice ){
        String dev_string = mUsbDevice.getDeviceName() + " " + mUsbDevice.getSerialNumber() + " is selected";
        Toast.makeText(this,dev_string,Toast.LENGTH_LONG).show();
    }

    //implementen fucking shit for spinners
    public void onItemSelected(AdapterView<?> sDevices, View v, int pos, long id){
        String device_position = sDevices.getSelectedItem().toString();
        mUsbDevice = devices.get(device_position);
        selected_dev_toast(v, mUsbDevice);
        connection = manager.openDevice(mUsbDevice);
        if (connection == null) {
            manager.requestPermission(mUsbDevice, mPermissionIntent);
        }
        else {
            port = init_port(mUsbDevice);
        }
    }
    public void onNothingSelected(AdapterView<?> parent) {
        // Do nothing here, why not
    }




    public void push_command(byte[] cmd, UsbSerialPort port){
        if (port != null){
            //
            try {
                port.write(cmd,10000);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

}
