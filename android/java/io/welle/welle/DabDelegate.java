package io.welle.welle;

import android.app.Activity;
import android.content.Intent;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.util.Log;

public class DabDelegate extends Activity {
    private static final String TAG = DabMediaService.class.getSimpleName();

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        Log.i(TAG, "DabDelegate onCreate");

        Intent intent = getIntent();
        Log.i(TAG, "DabDelegate onCreate"
                + " action: " + intent.getAction()
                + " component: " + intent.getComponent().toString()
                + " dataStr: " + intent.getDataString());

        if (intent.getAction().equals(UsbManager.ACTION_USB_DEVICE_ATTACHED)) {
            //if (intent.hasExtra(UsbManager.EXTRA_DEVICE)) {
            Log.i(TAG, "USB attached");
            UsbDevice usbDevice = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);

            Intent serviceIntent = new Intent(this, DabService.class);
            serviceIntent.setAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
            serviceIntent.putExtra(UsbManager.EXTRA_DEVICE, usbDevice);
            startService(serviceIntent);
        }

        finish();
    }

}
