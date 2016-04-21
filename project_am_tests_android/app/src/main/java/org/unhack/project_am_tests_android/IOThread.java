package org.unhack.project_am_tests_android;

import android.content.Context;
import android.content.Intent;
import android.util.Log;

import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.maxmpz.poweramp.player.PowerampAPI;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Created by unhack on 4/19/16.
 */
public class IOThread extends Thread {
    private CmdStorage mCmdStorage = new CmdStorage();
    private String inCmd = null;
    private final UsbSerialPort mPort;
    private final Context mContext;
    private boolean running = true;
    public IOThread(UsbSerialPort port, Context context){
        mPort = port;
        mContext = context;
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

            byte buffer[] = new byte[64];
            int numBytesRead = 0;
            try {
                numBytesRead = mPort.read(buffer, 100);
                Log.d("EBA!!!!","READING BUFFER");
            } catch (IOException e) {
                Log.d("EBA!!!!","HER VAM");
                e.printStackTrace();
            }
            for (int  i = 0; i < numBytesRead; i++){
                Log.d("EBA!!!!","PARSING BUFFER");
                if (buffer[i] == 113){
                    Log.d("EBA!!!!","MAGIC BYTE");
                    //short chk = (short)(buffer[i+2] << 8 | buffer[i+3]);
                    ByteBuffer chk_buf = ByteBuffer.allocate(2);
                    chk_buf.order(ByteOrder.LITTLE_ENDIAN);
                    chk_buf.put(buffer[i+2],buffer[i+3]);
                    short chk = chk_buf.getShort();
                    Log.d("EBA!!!!","CRC " + String.valueOf(chk));
                    if (chk == buffer[i+1]+113){
                        if (buffer[i+1] == 104){
                            mContext.startService(mCmdStorage.getAndroidIntent(buffer[i+1]));
                        }
                    }
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
