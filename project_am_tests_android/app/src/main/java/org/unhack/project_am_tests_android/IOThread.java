package org.unhack.project_am_tests_android;

import android.util.Log;

import com.hoho.android.usbserial.driver.UsbSerialPort;

import java.io.IOException;

/**
 * Created by unhack on 4/19/16.
 */
public class IOThread extends Thread {
    private CmdStorage mCmdStorage = new CmdStorage();
    private String inCmd = null;
    private final UsbSerialPort mPort;
    private boolean running = true;
    public IOThread(UsbSerialPort port){
        mPort = port;
    }


    @Override
    public void run(){
        while (running){
            if (inCmd != null){
                byte[] data = mCmdStorage.getCommand(inCmd);
                try {
                    mPort.write(data, 50);
                    inCmd = null;
                } catch (IOException e) {
                    Log.d("AM TESTS THREAD","No PORT");
                    e.printStackTrace();
                }

            }

        }

    }

    public void setInCmd(String cmd){
        this.inCmd = cmd;
    }

    public void stopthread(){
        this.running = false;
    }
}
