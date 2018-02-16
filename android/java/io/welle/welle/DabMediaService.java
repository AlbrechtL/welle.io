package io.welle.welle;

import android.content.res.Resources;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.IBinder;
import android.support.annotation.NonNull;
import android.support.v4.media.MediaBrowserCompat;
import android.support.v4.media.MediaBrowserServiceCompat;
import android.support.v4.media.MediaDescriptionCompat;
import android.util.Log;

import io.welle.welle.DabService.DabBinder;

import java.util.Comparator;
import java.util.Iterator;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class DabMediaService extends MediaBrowserServiceCompat implements ServiceConnection {

    private static final String TAG = DabMediaService.class.getSimpleName();

    private static final String MEDIA_ID_ROOT = "__ROOT__";
    private static final String MEDIA_ID_DAB_CHANNELS = "__DAB_CHANNELS__";
    private static final String MEDIA_ID_FAVORITE_STATIONS = "__FAVORITE_STATIONS__";

    private static final String[] CHANNELS = {
            // Band III
            "5A",  "5B",  "5C",  "5D",  "6A",  "6B",  "6C",  "6D",  "7A",  "7B",  "7C",  "7D",
            "8A",  "8B",  "8C",  "8D",  "9A",  "9B",  "9C",  "9D", "10A", "10B", "10C", "10D",
            "11A", "11B", "11C", "11D", "12A", "12B", "12C", "12D", "13A", "13B", "13C", "13D",
            "13E", "13F",
            // Band L
            "LA",  "LB",  "LC",  "LD",  "LE",  "LF",  "LG",  "LH",  "LI",  "LJ",  "LK",  "LL",
            "LM",  "LN",  "LO",  "LP"
    };

    private static DabMediaService instance = null;
    private static List<MediaBrowserCompat.MediaItem> mFavoriteList = new ArrayList<>();
    private List<MediaBrowserCompat.MediaItem> mChannelList = new ArrayList<>();

    public static Bitmap drawableToBitmap(Drawable drawable) {
        Bitmap bitmap = null;

        if (drawable instanceof BitmapDrawable) {
            BitmapDrawable bitmapDrawable = (BitmapDrawable) drawable;
            if(bitmapDrawable.getBitmap() != null) {
                return bitmapDrawable.getBitmap();
            }
        }

        if(drawable.getIntrinsicWidth() <= 0 || drawable.getIntrinsicHeight() <= 0) {
            bitmap = Bitmap.createBitmap(1, 1, Bitmap.Config.ARGB_8888); // Single color bitmap will be created of 1x1 pixel
        } else {
            bitmap = Bitmap.createBitmap(drawable.getIntrinsicWidth(), drawable.getIntrinsicHeight(), Bitmap.Config.ARGB_8888);
        }

        Canvas canvas = new Canvas(bitmap);
        drawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
        drawable.draw(canvas);
        return bitmap;
    }

    public static void addFavoriteStation(String station, String channel) {
        Log.i(TAG, "Add favorite station: " + station + " channel: " + channel);
        boolean updateRoot = mFavoriteList.isEmpty();

        mFavoriteList.add(new MediaBrowserCompat.MediaItem(DabService.createStation(station, channel),
                MediaBrowserCompat.MediaItem.FLAG_PLAYABLE));
        Collections.sort(mFavoriteList, new Comparator<MediaBrowserCompat.MediaItem>() {
            @Override
            public int compare(MediaBrowserCompat.MediaItem lhs, MediaBrowserCompat.MediaItem rhs) {
                return DabService.compareStation(lhs.getDescription(), rhs.getDescription());
            }
        });

        if (instance != null) {
            instance.notifyChildrenChanged(updateRoot ? MEDIA_ID_ROOT : MEDIA_ID_FAVORITE_STATIONS);
        }
    }

    public static void removeFavoriteStation(String station, String channel) {
        Log.i(TAG, "Remove favorite station: " + station + " channel: " + channel);

        String mediaId = DabService.toMediaId(station, channel);
        Iterator<MediaBrowserCompat.MediaItem> it = mFavoriteList.iterator();
        while (it.hasNext()) {
            MediaBrowserCompat.MediaItem mediaItem = it.next();
            if (mediaId.equals(mediaItem.getMediaId())) {
                it.remove();
            }
        }

        if (instance != null) {
            instance.notifyChildrenChanged(mFavoriteList.isEmpty() ? MEDIA_ID_ROOT : MEDIA_ID_FAVORITE_STATIONS);
        }
    }

    public static void clearFavoriteStations() {
        Log.i(TAG, "Clear favorite stations");
        if (mFavoriteList.isEmpty())
            return;

        mFavoriteList.clear();

        if (instance != null) {
            instance.notifyChildrenChanged(MEDIA_ID_ROOT);
        }
    }

    /*
     * (non-Javadoc)
     * @see android.app.Service#onCreate()
     */
    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG, "onCreate");

        instance = this;

        bindService(new Intent(this, DabService.class), this, Context.BIND_AUTO_CREATE);

        // Init channel list
        for (String channel : CHANNELS) {
            Bundle extras = new Bundle();
            extras.putInt(DabService.BUNDLE_KEY_DAB_TYPE, DabService.TYPE_DAB_CHANNEL);
            extras.putString(DabService.BUNDLE_KEY_CHANNEL, channel);

            mChannelList.add(new MediaBrowserCompat.MediaItem(new MediaDescriptionCompat.Builder()
                    .setMediaId(DabService.toMediaId(null, channel))
                    .setTitle(channel)
                    .setExtras(extras)
                    .build(), MediaBrowserCompat.MediaItem.FLAG_PLAYABLE));
        }
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
        unbindService(this);
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder binder) {
        Log.d(TAG, "onServiceConnected");
        if (binder instanceof DabBinder) {
            Log.d(TAG, "DabService connected");
            DabBinder dabBinder = (DabBinder) binder;

            // Connect RTL-SDR if needed
            if (!dabBinder.isDeviceAvailable()) {
                Log.d(TAG, "DabService connect to RTL-SDR");
                Intent intent = new Intent(this, DabDelegate.class);
                intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                startActivity(intent);
            }

            // Set media session
            setSessionToken(dabBinder.getSessionToken());
        }
    }

    @Override
    public void onServiceDisconnected(ComponentName name) {
        Log.d(TAG, "onServiceDisconnected");
    }

    @Override
    public BrowserRoot onGetRoot(@NonNull String clientPackageName, int clientUid, Bundle rootHints) {
        return new BrowserRoot(MEDIA_ID_ROOT, null);
    }

    @Override
    public void onLoadChildren(@NonNull final String parentMediaId,
                               @NonNull final Result<List<MediaBrowserCompat.MediaItem>> result) {
        Log.d(TAG, "onLoadChildren: parentMediaId=" + parentMediaId);

        switch (parentMediaId) {
            case MEDIA_ID_ROOT:
                Resources resources = getResources();

                List<MediaBrowserCompat.MediaItem> mediaItems = new ArrayList<>();

                if (!mFavoriteList.isEmpty()) {
                    mediaItems.add(new MediaBrowserCompat.MediaItem(new MediaDescriptionCompat.Builder()
                            .setMediaId(MEDIA_ID_FAVORITE_STATIONS)
                            .setTitle(resources.getString(R.string.menu_favorites))
                            .setIconBitmap(drawableToBitmap(resources.getDrawable(R.drawable.ic_favorites)))
                            .build(), MediaBrowserCompat.MediaItem.FLAG_BROWSABLE));
                }

                mediaItems.add(new MediaBrowserCompat.MediaItem(new MediaDescriptionCompat.Builder()
                        .setMediaId(MEDIA_ID_DAB_CHANNELS)
                        .setTitle(resources.getString(R.string.menu_channels))
                        .setIconBitmap(drawableToBitmap(resources.getDrawable(R.drawable.ic_antenna)))
                        .build(), MediaBrowserCompat.MediaItem.FLAG_BROWSABLE));

                result.sendResult(mediaItems);
                break;

            case MEDIA_ID_DAB_CHANNELS:
                result.sendResult(mChannelList);
                break;

            case MEDIA_ID_FAVORITE_STATIONS:
                result.sendResult(mFavoriteList);
                break;

            default:
                Log.w(TAG, "Skipping unmatched parentMediaId: " + parentMediaId);
                result.sendResult(null);
                break;
        }
    }

    @Override
    public void onSearch(@NonNull String query, Bundle extras,
                         @NonNull Result<List<MediaBrowserCompat.MediaItem>> result) {
        Log.d(TAG, "onSearch: " + query);
        List<MediaBrowserCompat.MediaItem> mediaItems = new ArrayList<>();
        //TODO add search results
        result.sendResult(mediaItems);
    }
}
