package org.unhack.project_am_tests_android;

import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.IBinder;
import android.support.v4.app.NotificationCompat;
import android.util.Log;
import android.widget.Toast;

import com.felhr.usbserial.UsbSerialDevice;
import com.felhr.usbserial.UsbSerialInterface;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;

import java.io.IOException;
import java.util.HashMap;
import java.util.Iterator;



public class SerialIOService extends Service {
    private int ONGOING_NOTIFICATION_ID = 1;
    public int ArduinoId = 9025;
    public int ProlificId = 1659;
    public UsbManager manager = null;
    public HashMap<String, UsbDevice> availableDrivers;
    public UsbDevice mUsbDevice;
    public UsbSerialDevice mOloloDevice;
    public UsbSerialDriver mUsbSerialDevice = null;
    //public UsbSerialPort port = null;
    public UsbSerialDevice port = null;
    public UsbDeviceConnection connection;
    private IOThread mThread;


    public UsbSerialDevice init_port(UsbDevice mUsbDevice){
        port = UsbSerialDevice.createUsbSerialDevice(mUsbDevice,connection);
        port.open();
        port.setBaudRate(115200);
        port.setDataBits(UsbSerialInterface.DATA_BITS_8);
        port.setParity(UsbSerialInterface.PARITY_NONE);
        port.setFlowControl(UsbSerialInterface.FLOW_CONTROL_OFF);
        port.setStopBits(1);
        return port;
    }

    public UsbSerialPort init_port_2 (UsbDevice mUsbDevice) {
        mUsbSerialDevice = UsbSerialProber.getDefaultProber().probeDevice(mUsbDevice);
        Log.d("AM PORTS!!!!",mUsbSerialDevice.getPorts().toString());
        UsbSerialPort port = mUsbSerialDevice.getPorts().get(0);
        connection = manager.openDevice(mUsbDevice);
        try {
            port.open(connection);
        } catch (IOException e) {
            Log.d("AMTESTS  OPEN PORT","EXCEPTION");
            e.printStackTrace();
        }
        try {
            port.setParameters(115200, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE);
            return port;
        } catch (IOException e) {
            Log.d("AMTESTS SET PORT","EXCEPTION");
            // Deal with error.
        }
        return null;
    }



    public void select_port(){
        manager = (UsbManager) getSystemService(Context.USB_SERVICE);
        availableDrivers = manager.getDeviceList();
        //pass throug all usb devices and search for arduino
        //a little bit narkomansky way to find correct device
        //but why not?
        Log.d("AM TESTST SERVICE","In SELECT PORT");
        if (availableDrivers != null) {
            Iterator<UsbDevice> deviceIterator = availableDrivers.values().iterator();
            while (deviceIterator.hasNext()) {
                UsbDevice device = deviceIterator.next();
                if (device.getVendorId() == ArduinoId || device.getVendorId()  == ProlificId || device.getVendorId() == 8963 ) {
                    mUsbDevice = device;
                    break;
                }
            }
        }

        if (mUsbDevice != null) {
            if (!manager.hasPermission(mUsbDevice)){
                Log.d("AMTESTS SERVICE","OPA OPA PERMISSIONS");
                Intent mReqPermIntent = new Intent(this, MainActivity.class);
                mReqPermIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                startActivity(mReqPermIntent);
            }
            else {
                connection = manager.openDevice(mUsbDevice);
                if (connection != null) {
                    port = init_port(mUsbDevice);
                }
            }

        }
        else {

            Log.d("AM TESTS PORT INIT","FAIL. Killing service");
            this.onDestroy();
            //we does not got a device
            //here will be self-destruction part
        }
    }

    public SerialIOService() {
    }

    @Override
    public void onCreate() {
        //register reciever
        //in onCreate we select device
        //and init port by calling select_port
        Log.d("AM TESTST SERVICE", "In onCreate");
        Toast.makeText(getApplicationContext(),"Starting service",Toast.LENGTH_SHORT).show();
        select_port();
        if (port != null) {
            Intent notificationIntent = new Intent(this, MainActivity.class);
            PendingIntent pendingIntent = PendingIntent.getActivity(this, 0,
                    notificationIntent, 0);

            NotificationCompat.Builder mBuilder = new NotificationCompat.Builder(this)
                    .setSmallIcon(R.drawable.service_icon)
                    .setContentTitle(getString(R.string.service_name))
                    .setContentText(mUsbDevice.getManufacturerName() + " " + mUsbDevice.getProductName() + "\n is running. Probably")
                    .setContentIntent(pendingIntent);

            Notification mNotification = mBuilder.build();
            startForeground(ONGOING_NOTIFICATION_ID, mNotification);
            Log.d("AM TESTST SERVICE", "After starting notification");

            mThread = new IOThread(port,getApplicationContext());
            Log.d("AM TESTS SERVICE","Created thread");
            mThread.start();

        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId){
        Log.d("AM TESTST SERVICE","In onStart");

        String inCmd = intent.getStringExtra("INCMD");
        if (inCmd != null && !inCmd.equals("")) {
            Log.d("AM TESTST SERVICE", inCmd);
        }
        else {
            Log.d("AM TESTST SERVICE", "NO BLOODY COMMAND!!!");
        }

        if (mThread != null) {
            mThread.setInCmd(inCmd);
        }

        return START_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        // TODO: Return the communication channel to the service.
        //throw new UnsupportedOperationException("Not yet implemented");
        return null;
    }

    @Override
    public void onDestroy() {
        //destroy something here later

        if (mThread != null) {
            mThread.stopthread();
        }

        Toast.makeText(getApplicationContext(),"Stoping service",Toast.LENGTH_SHORT).show();
    }

}
