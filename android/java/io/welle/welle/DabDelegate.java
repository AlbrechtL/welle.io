package io.welle.welle;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;

public class DabDelegate extends Activity {
    private static final String TAG = DabDelegate.class.getSimpleName();

    public static void startDab(Context context) {
        context.startActivity(new Intent(context, DabDelegate.class));
    }

    private ServiceConnection mConnection = null;
    private DabService.DabBinder mDabBinder = null;

    private boolean startSdrActivity(UsbDevice usbDevice) {
        try {
            Intent intent = new Intent(Intent.ACTION_VIEW).setData(
                    Uri.parse("iqsrc://-a " + DabService.SDR_ADDRESS
                            + " -p " + DabService.SDR_PORT
                            + " -s " + DabService.SDR_SAMPLERATE));
            if (usbDevice != null) {
                intent.putExtra(UsbManager.EXTRA_DEVICE, usbDevice);
            }
            startActivityForResult(intent, 1234);
            return true;
        } catch (ActivityNotFoundException e) {
            Log.e(TAG, "RTL-SDR error: " + "Android RTL-SDR driver is not installed");
            if (mDabBinder != null)
                mDabBinder.setError(getString(R.string.error_driver_not_installed));
            return false;
        }
    }

    private void showAndroidInstallDialog() throws Exception {
        AlertDialog.Builder builder = new AlertDialog.Builder(DabDelegate.this);
        builder.setTitle(R.string.dialog_driver_title);
        builder.setMessage(R.string.dialog_driver_msg);

        builder.setPositiveButton(R.string.dialog_driver_btn_ok, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                try {
                    startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("market://details?id=marto.rtl_tcp_andro")));
                } catch (android.content.ActivityNotFoundException anfe) {
                    startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("https://play.google.com/store/apps/details?id=marto.rtl_tcp_andro")));
                }

                dialog.dismiss();
            }
        });

        builder.setNegativeButton(R.string.dialog_driver_btn_cancel, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
            }
        });

        builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                finish();
            }
        });

        builder.show();
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate");

        Intent intent = getIntent();
        if (intent != null && intent.hasExtra(UsbManager.EXTRA_DEVICE)) {
            UsbDevice usbDevice = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
            Log.i(TAG, "USB attached: " + usbDevice.getDeviceName());
            startSdrActivity(usbDevice);
        } else {
            Log.i(TAG, "DAB started");
            mConnection = new ServiceConnection() {
                @Override
                public void onServiceConnected(ComponentName name, IBinder binder) {
                    mDabBinder = (DabService.DabBinder) binder;
                    if (mDabBinder.isDeviceAvailable()) {
                        finish();
                    } else if (!startSdrActivity(null)) {
                        // Show Android Install Dialog & Error
                        try {
                            showAndroidInstallDialog();
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                }

                @Override
                public void onServiceDisconnected(ComponentName name) {
                    finish();
                }
            };
            startService(new Intent(this, DabService.class));
            bindService(new Intent(this, DabService.class), mConnection, Context.BIND_AUTO_CREATE);
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode != 1234) return; // This is the requestCode that was used with startActivityForResult
        if (resultCode == RESULT_OK) {
            Log.d(TAG, "onActivityResult: RESULT_OK");
            // Connection with device has been opened and the rtl-tcp server is running. You are now responsible for connecting.
            Intent serviceIntent = new Intent(this, DabService.class);
            serviceIntent.setAction(DabService.ACTION_SDR_DEVICE_ATTACHED);
            serviceIntent.putExtras(data.getExtras());
            startService(serviceIntent);
            finish();
        } else if (data != null) {
            // something went wrong, and the driver failed to start
            String errmsg = data.getStringExtra("detailed_exception_message");
            Log.e(TAG, "RTL-SDR error: " + errmsg);
            if (mDabBinder != null)
                mDabBinder.setError(errmsg);
            finish();
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "onDestroy");

        if (mConnection != null) {
            unbindService(mConnection);
        }
    }
}
