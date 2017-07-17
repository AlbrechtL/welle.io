package io.welle.welle;

import android.content.res.Resources;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.support.annotation.NonNull;
import android.support.v4.media.MediaBrowserCompat;
import android.support.v4.media.MediaBrowserServiceCompat;
import android.support.v4.media.MediaDescriptionCompat;
import android.util.Log;

import io.welle.welle.DabService.DabBinder;
import io.welle.welle.DabService.DabCallback;

import java.util.ArrayList;
import java.util.List;

public class DabMediaService extends MediaBrowserServiceCompat implements ServiceConnection, DabCallback {

    private static final String TAG = DabMediaService.class.getSimpleName();

    private static final String MEDIA_ID_ROOT = "__ROOT__";
    private static final String MEDIA_ID_DAB_CHANNELS = "__DAB_CHANNELS__";
    private static final String MEDIA_ID_DAB_STATIONS = "__DAB_STATIONS__";
    private static final String MEDIA_ID_FAVORITE_STATIONS = "__FAVORITE_STATIONS__";

    private DabBinder mDabBinder = null;

    /*
     * (non-Javadoc)
     * @see android.app.Service#onCreate()
     */
    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG, "onCreate");
        bindService(new Intent(this, DabService.class), this, Context.BIND_AUTO_CREATE);
    }

    /**
     * (non-Javadoc)
     *
     * @see android.app.Service#onStartCommand(android.content.Intent, int, int)
     */
    @Override
    public int onStartCommand(Intent startIntent, int flags, int startId) {
        return super.onStartCommand(startIntent, flags, startId);
    }

    /**
     * (non-Javadoc)
     *
     * @see android.app.Service#onDestroy()
     */
    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "onDestroy");
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder binder) {
        Log.d(TAG, "onServiceConnected");
        if (binder instanceof DabBinder) {
            Log.i(TAG, "DabService connected");
            mDabBinder = (DabBinder) binder;
            mDabBinder.setDabCallback(this);
            setSessionToken(mDabBinder.getSessionToken());
        }
    }

    @Override
    public void onServiceDisconnected(ComponentName name) {
        Log.d(TAG, "onServiceDisconnected");
        mDabBinder = null;
    }

    @Override
    public BrowserRoot onGetRoot(@NonNull String clientPackageName, int clientUid, Bundle rootHints) {
        return new BrowserRoot(MEDIA_ID_ROOT, null);
    }

    @Override
    public void onLoadChildren(@NonNull final String parentMediaId,
                               @NonNull final Result<List<MediaBrowserCompat.MediaItem>> result) {
        Log.d(TAG, "OnLoadChildren: parentMediaId=" + parentMediaId);

        List<MediaBrowserCompat.MediaItem> mediaItems = new ArrayList<>();

        switch (parentMediaId) {
            case MEDIA_ID_ROOT:
                Resources resources = getResources();
//TODO channels                mediaItems.add(new MediaBrowserCompat.MediaItem(new MediaDescriptionCompat.Builder()
//                        .setMediaId(MEDIA_ID_DAB_CHANNELS)
//                        .setTitle(resources.getString(R.string.menu_channels))
//                        .build(), MediaBrowserCompat.MediaItem.FLAG_BROWSABLE));

                mediaItems.add(new MediaBrowserCompat.MediaItem(new MediaDescriptionCompat.Builder()
                        .setMediaId(MEDIA_ID_DAB_STATIONS)
                        .setTitle(resources.getString(R.string.menu_stations))
                        .build(), MediaBrowserCompat.MediaItem.FLAG_BROWSABLE));

//TODO fav                mediaItems.add(new MediaBrowserCompat.MediaItem(new MediaDescriptionCompat.Builder()
//                        .setMediaId(MEDIA_ID_FAVORITE_STATIONS)
//                        .setTitle(resources.getString(R.string.menu_favorites))
//                        .build(), MediaBrowserCompat.MediaItem.FLAG_BROWSABLE));

                break;

            case MEDIA_ID_DAB_CHANNELS:
                mediaItems = DabService.getChannelList();
                break;

            case MEDIA_ID_DAB_STATIONS:
                if (mDabBinder != null)
                    mediaItems = mDabBinder.getStationList();
                break;

            case MEDIA_ID_FAVORITE_STATIONS:
                if (mDabBinder != null)
                    mediaItems = mDabBinder.getFavoriteList();
                break;

            default:
                Log.w(TAG, "Skipping unmatched parentMediaId: " + parentMediaId);
                break;
        }
        result.sendResult(mediaItems);
    }

    @Override
    public void onSearch(String query, Bundle extras, Result<List<MediaBrowserCompat.MediaItem>> result) {
        List<MediaBrowserCompat.MediaItem> mediaItems = new ArrayList<>();
        //TODO add search results
        result.sendResult(mediaItems);
    }

    @Override
    public void updateStationList(List<MediaBrowserCompat.MediaItem> list) {
        notifyChildrenChanged(MEDIA_ID_DAB_STATIONS);
    }

    @Override
    public void updateFavoriteList(List<MediaBrowserCompat.MediaItem> list) {
//TODO fav        notifyChildrenChanged(MEDIA_ID_FAVORITE_STATIONS);
    }

    @Override
    public void updateDabStatus(int playbackState) {}

    @Override
    public void updateScanProgress(int scanProgress) {}

    @Override
    public void showError(String error) {}
}
