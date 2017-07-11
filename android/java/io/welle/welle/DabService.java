package io.welle.welle;

import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.IntentFilter;
import android.content.res.Resources;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.media.AudioManager;
import android.os.Binder;
import android.os.Bundle;
import android.os.IBinder;
import android.content.Intent;

import org.qtproject.qt5.android.bindings.QtService;

import android.os.SystemClock;
import android.support.annotation.NonNull;
import android.support.v4.app.NotificationManagerCompat;
import android.support.v4.media.MediaBrowserCompat;
import android.support.v4.media.MediaDescriptionCompat;
import android.support.v4.media.MediaMetadataCompat;
import android.support.v4.media.session.MediaSessionCompat;
import android.support.v4.media.session.MediaSessionCompat.Token;
import android.support.v4.media.session.PlaybackStateCompat;
import android.support.v7.app.NotificationCompat;
import android.util.Log;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

public class DabService extends QtService implements AudioManager.OnAudioFocusChangeListener {

    private static final String TAG = DabService.class.getSimpleName();

    // ID for our MediaNotification.
    public static final int NOTIFICATION_ID = 416;

    // DAB status
    private static final int DAB_STATUS_ERROR       = -1;
    private static final int DAB_STATUS_UNKNOWN     = 0;
    private static final int DAB_STATUS_INITIALISED = 1;
    private static final int DAB_STATUS_PLAYING     = 2;
    private static final int DAB_STATUS_PAUSED      = 3;
    private static final int DAB_STATUS_STOPPED     = 4;
    private static final int DAB_STATUS_SCANNING    = 5;

    // Custom actions
    private static final String CUSTOM_ACTION_PLAY = "io.welle.welle.PLAY";
    private static final String CUSTOM_ACTION_PAUSE = "io.welle.welle.PAUSE";
    private static final String CUSTOM_ACTION_SKIP_NEXT = "io.welle.welle.SKIP_NEXT";
    private static final String CUSTOM_ACTION_SKIP_PREV = "io.welle.welle.SKIP_PREV";
    private static final String CUSTOM_ACTION_SCAN_START = "io.welle.welle.SCAN_START";
    private static final String CUSTOM_ACTION_SCAN_STOP = "io.welle.welle.SCAN_STOP";
    private static final String CUSTOM_ACTION_FAVORITE = "io.welle.welle.FAVORITE";
    private static final String CUSTOM_ACTION_RECORD = "io.welle.welle.RECORD";
    private static final String CUSTOM_ACTION_NEXT_CHANNEL = "io.welle.welle.NEXT_CHANNEL";

    private static final int ACTION_OPEN = 1;
    private static final int ACTION_PLAY = 2;
    private static final int ACTION_PAUSE = 3;
    private static final int ACTION_SKIP_NEXT = 4;
    private static final int ACTION_SKIP_PREV = 5;
    private static final int ACTION_SCAN_START = 6;
    private static final int ACTION_SCAN_STOP = 7;
    private static final int ACTION_NEXT_CHANNEL = 8;

    // Bundle extras
    private static final String BUNDLE_KEY_DAB_TYPE = "DAB_TYPE";
    private static final String BUNDLE_KEY_STATION_ID = "STATION_ID";
    private static final String BUNDLE_KEY_STATION_NAME = "STATION_NAME";
    private static final String BUNDLE_KEY_CHANNEL = "CHANNEL";

    private static final String MEDIA_ID_CHANNEL_SCAN = "FFFFFF";
    private static final String MEDIA_ID_ERROR = "EEEEEE";

    private static final int TYPE_DAB_STATION = 1;
    private static final int TYPE_DAB_CHANNEL = 2;

    // Use these extras to reserve space for the corresponding actions, even when they are disabled
    // in the playbackstate, so the custom actions don't reflow.
    private static final String SLOT_RESERVATION_SKIP_TO_NEXT =
            "com.google.android.gms.car.media.ALWAYS_RESERVE_SPACE_FOR.ACTION_SKIP_TO_NEXT";
    private static final String SLOT_RESERVATION_SKIP_TO_PREV =
            "com.google.android.gms.car.media.ALWAYS_RESERVE_SPACE_FOR.ACTION_SKIP_TO_PREVIOUS";
    private static final String SLOT_RESERVATION_QUEUE =
            "com.google.android.gms.car.media.ALWAYS_RESERVE_SPACE_FOR.ACTION_QUEUE";
    private static final String ANDROID_AUTO_STATUS = "com.google.android.gms.car.media.STATUS";
    private static final String ANDROID_AUTO_MEDIA_CONNECTION_STATUS = "media_connection_status";
    private static final String ANDROID_AUTO_MEDIA_CONNECTED = "media_connected";

    public static final String[] CHANNELS = {
            // Band III
            "5A",  "5B",  "5C",  "5D",  "6A",  "6B",  "6C",  "6D",  "7A",  "7B",  "7C",  "7D",
            "8A",  "8B",  "8C",  "8D",  "9A",  "9B",  "9C",  "9D", "10A", "10B", "10C", "10D",
            "11A", "11B", "11C", "11D", "12A", "12B", "12C", "12D", "13A", "13B", "13C", "13D",
            "13E", "13F",
            // Band L
            "LA",  "LB",  "LC",  "LD",  "LE",  "LF",  "LG",  "LH",  "LI",  "LJ",  "LK",  "LL",
            "LM",  "LN",  "LO",  "LP"
    };

    private static final String CHANNEL_ID_PREFIX = "____";

    private static List<MediaBrowserCompat.MediaItem> channelList = null;

    public static List<MediaBrowserCompat.MediaItem> getChannelList() {
        if (channelList == null) {
            channelList = new ArrayList<>();
            for (String channel : DabService.CHANNELS) {
                Bundle extras = new Bundle();
                extras.putInt(BUNDLE_KEY_DAB_TYPE, TYPE_DAB_CHANNEL);
                extras.putString(BUNDLE_KEY_CHANNEL, channel);

                channelList.add(new MediaBrowserCompat.MediaItem(new MediaDescriptionCompat.Builder()
                        .setMediaId(CHANNEL_ID_PREFIX + channel)
                        .setTitle(channel)
                        .setExtras(extras)
                        .build(), MediaBrowserCompat.MediaItem.FLAG_PLAYABLE));
            }
        }
        return channelList;
    }

    // Native

    private static DabService instance = null;

    public static native void openUsbDevice(int fd, String path);

    public static native boolean isFavoriteStation(String stationId);
    public static native void addFavoriteStation(String stationId, String stationName);
    public static native void removeFavoriteStation(String stationId);

    public static native void playStationById(String stationId);
    public static native String lastStation();

    public static native void duckPlayback(boolean duck);
    public static native void pausePlayback();
    public static native void stopPlayback();

    public static native void nextChannel();
    public static native void setManualChannel(String channelName);

    public static native void startChannelScan();
    public static native void stopChannelScan();

    public static native void saveStations();

    public static void serviceReady() {
        Log.i(TAG, "Service ready");
        if (instance == null)
            return;

        instance.handleServiceReady();
    }

    public static void deviceReady() {
        Log.i(TAG, "Device ready");
        if (instance == null)
            return;

        instance.handleDeviceReady();
    }

    public static void addStation(String id, String station, String channel) {
        Log.i(TAG, "Add station: " + station + " (" + id + ") channel: " + channel);
        if (instance == null)
            return;

        Bundle extras = new Bundle();
        extras.putInt(BUNDLE_KEY_DAB_TYPE, TYPE_DAB_STATION);
        extras.putString(BUNDLE_KEY_STATION_ID, id);
        extras.putString(BUNDLE_KEY_STATION_NAME, station);
        extras.putString(BUNDLE_KEY_CHANNEL, channel);

        instance.mStationList.add(new MediaBrowserCompat.MediaItem(new MediaDescriptionCompat.Builder()
                .setMediaId(id)
                .setTitle(station.trim() + " (" + channel + ")")
                .setExtras(extras)
                .build(), MediaBrowserCompat.MediaItem.FLAG_PLAYABLE));

        Collections.sort(instance.mStationList, new Comparator<MediaBrowserCompat.MediaItem>() {
            @Override
            public int compare(MediaBrowserCompat.MediaItem lhs, MediaBrowserCompat.MediaItem rhs) {
                String lStation = lhs.getDescription().getTitle().toString();
                String rStation = rhs.getDescription().getTitle().toString();

                int comp = lStation.compareToIgnoreCase(rStation);
                //if (comp == 0) {
                //    String lChannel = lhs.getDescription().getExtras().getString(BUNDLE_KEY_CHANNEL);
                //    String rChannel = rhs.getDescription().getExtras().getString(BUNDLE_KEY_CHANNEL);
                //    comp = lChannel.compareToIgnoreCase(rChannel);
                //}
                return comp;
            }
        });

        if (instance.mDabCallback != null) {
            instance.mDabCallback.updateStationList(instance.mStationList);
        }
    }

    public static void clearStations() {
        Log.i(TAG, "Clear stations");
        if (instance == null)
            return;

        instance.mStationList.clear();
    }

    public static void channelScanStopped() {
        Log.i(TAG, "Channel scan stopped");
        if (instance == null)
            return;

        instance.mChannelScanProgress = -1;
    }

    public static void channelScanProgress(int progress) {
        Log.i(TAG, "Channel scan progress: " + progress);
        if (instance == null)
            return;

        instance.mChannelScanProgress = progress;
    }

    public static void updateDisplayData(int status, String channel, String station, String label, String type) {
        Log.i(TAG, "Update display data status" + status + " channel: " + channel + ", station=" + station
                + ", label=" + label + ", type=" + type);
        if (instance == null)
            return;

        String displayTitle = (channel == null) ? station : station + " (" + channel +")";

        instance.mDabStatus = status;
        instance.mTrack = new MediaMetadataCompat.Builder()
                .putString(MediaMetadataCompat.METADATA_KEY_TITLE, station)
                .putString(MediaMetadataCompat.METADATA_KEY_DISPLAY_TITLE, displayTitle)
                .putString(MediaMetadataCompat.METADATA_KEY_DISPLAY_SUBTITLE, label)
                .putString(MediaMetadataCompat.METADATA_KEY_GENRE, type)
                .build();
        instance.updatePlaybackState();
    }

    public static void showErrorMessage(String text) {
        Log.i(TAG, "Show error message: " + text);
        if (instance == null)
            return;

        instance.mError = text;
        instance.mDabStatus = DAB_STATUS_ERROR;
        instance.updatePlaybackState();
    }

    public static void showInfoMessage(String text) {
        Log.i(TAG, "Show info message: " + text);
    }

    public interface DabCallback {
        public void updateStationList(List<MediaBrowserCompat.MediaItem> list);
        public void updateFavoriteList(List<MediaBrowserCompat.MediaItem> list);
        public void updateDabStatus(int playbackState);
        public void updateScanProgress(int scanProgress);
        public void showError(String error);
    }

    public class DabBinder extends Binder {

        Token getSessionToken() {
            return mSession.getSessionToken();
        }

        List<MediaBrowserCompat.MediaItem> getStationList() {
            return mStationList;
        }

        List<MediaBrowserCompat.MediaItem> getFavoriteList() {
            return mFavoriteList;
        }

        void setDabCallback(DabCallback callback) {
            Log.i(TAG, "DabCallback set");
            mDabCallback = callback;
            initDabCallback();
        }
    }

    private UsbDevice mUsbDevice = null;
    private DabCallback mDabCallback = null;
    private MediaSessionCompat mSession = null;
    private MediaMetadataCompat mTrack = null;
    private NotificationManagerCompat mNotificationManager = null;
    private BroadcastReceiver mDabReceiver = null;

    private int mChannelScanProgress = -1;
    private int mDabStatus = DAB_STATUS_UNKNOWN;
    private MediaBrowserCompat.MediaItem mCurrentMedia = null;
    private String mCurrentChannel = null;
    private String mError = null;

    private boolean mUsbConnected = false;
    private boolean mServiceReady = false;
    private boolean mAudioFocus = false;

    List<MediaBrowserCompat.MediaItem> mStationList = new ArrayList<>();
    List<MediaBrowserCompat.MediaItem> mFavoriteList = new ArrayList<>();

    private IBinder mBinder = new DabBinder();

    private void openUsbDevice() {
        if (mUsbDevice == null || mUsbConnected || !mServiceReady)
            return;

        // Get device path
        String path = mUsbDevice.getDeviceName();

        // Open device
        UsbManager usbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
        UsbDeviceConnection deviceConnection = usbManager.openDevice(mUsbDevice);
        int fd = deviceConnection.getFileDescriptor();
        openUsbDevice(fd, path);
    }

    private void handleServiceReady() {
        Log.d(TAG, "Handle service ready");
        mServiceReady = true;
        openUsbDevice();
    }

    private void handleDeviceReady() {
        Log.d(TAG, "Handle device ready");
        mUsbConnected = true;

        // Inform others
        initDabCallback();

        // Play last station
        playLastStation();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////

    private final class MediaSessionCallback extends MediaSessionCompat.Callback {

        @Override
        public void onPlayFromMediaId(String mediaId, Bundle extras) {
            Log.d(TAG, "playFromMediaId mediaId:" + mediaId + "  extras=" + extras);
            // Check stations
            for (MediaBrowserCompat.MediaItem mediaItem: mStationList) {
                if (mediaItem.getMediaId().equals(mediaId)) {
                    Log.d(TAG, "playFromMediaId: station " + mediaId + " found");
                    mCurrentMedia = mediaItem;

                    // play the music
                    handlePlayRequest();
                    break;
                }
            }
            // Check channels
            for (MediaBrowserCompat.MediaItem mediaItem: getChannelList()) {
                if (mediaItem.getMediaId().equals(mediaId)) {
                    Log.d(TAG, "playFromMediaId: channel " + mediaId + " found");
                    mCurrentMedia = mediaItem;

                    // play the music
                    handlePlayRequest();
                    break;
                }
            }
        }

        @Override
        public void onPlay() {
            Log.d(TAG, "play");

            handlePlayRequest();
        }

        @Override
        public void onPause() {
            Log.d(TAG, "pause");
            handlePauseRequest();
        }

        @Override
        public void onStop() {
            Log.d(TAG, "stop");
            handleStopRequest();
        }

        @Override
        public void onSkipToNext() {
            Log.d(TAG, "skipToNext");
            handleSkipRequest(1);
        }

        @Override
        public void onSkipToPrevious() {
            Log.d(TAG, "skipToPrevious");
            handleSkipRequest(-1);
        }

        @Override
        public void onSkipToQueueItem(long queueId) {
            Log.d(TAG, "OnSkipToQueueItem:" + queueId);
        }

        @Override
        public void onSeekTo(long position) {
            Log.d(TAG, "onSeekTo:" + position);
        }

        @Override
        public void onCustomAction(@NonNull String action, Bundle extras) {
            if (!handleCustomAction(action, extras))
                Log.d(TAG, "onCustomAction unsupported action: " + action);
        }

        @Override
        public void onPlayFromSearch(final String query, final Bundle extras) {
            Log.d(TAG, "playFromSearch query=" + query + " extras=" + extras);
        }
    }

    private void handlePlayRequest() {
        Log.d(TAG, "handlePlayRequest");

        if (!requestAudioFocus()) {
            Log.d(TAG, "request audio focus failed!");
            return;
        }

        if (!mSession.isActive()) {
            mSession.setActive(true);
        }

        if (mCurrentMedia == null) {
            playLastStation();
        } else {
            int type = mCurrentMedia.getDescription().getExtras().getInt(BUNDLE_KEY_DAB_TYPE);
            if (type == TYPE_DAB_STATION)
                playStationById(mCurrentMedia.getMediaId());
            else
                setManualChannel(mCurrentMedia.getMediaId());
        }
    }

    private void handlePauseRequest() {
        Log.d(TAG, "handlePauseRequest");
        if (0 <= mChannelScanProgress)
            stopChannelScan();
        else
            pausePlayback();

        releaseAudioFocus();
    }

    private void handleStopRequest() {
        Log.d(TAG, "handleStopRequest");
        if (0 <= mChannelScanProgress)
            stopChannelScan();
        else
            stopPlayback();

        releaseAudioFocus();
    }

    private void handleSkipRequest(int i) {
        Log.d(TAG, "handleSkipRequest: " + i);
        if (mCurrentMedia == null || mStationList.isEmpty())
            return;
        int index = mStationList.indexOf(mCurrentMedia);
        MediaBrowserCompat.MediaItem mediaItem = null;
        if (index == -1) {
            mediaItem = mStationList.get(0);
        } else {
            index += i;
            if (0 <= index && index < mStationList.size()) {
                mediaItem = mStationList.get(index);
            }
        }

        if (mediaItem != null) {
            mCurrentMedia = mediaItem;
            handlePlayRequest();
        }
    }

    private void handleScanStartRequest() {
        Log.d(TAG, "handleScanStartRequest");
        startChannelScan();
    }

    private void handleScanStopRequest() {
        Log.d(TAG, "handleScanStopRequest");
        stopChannelScan();
    }

    private boolean handleCustomAction(String action, Bundle extras) {
        if (CUSTOM_ACTION_PLAY.equals(action)) {
            Log.i(TAG, "handleCustomAction: play");
            handlePlayRequest();
        } else if (CUSTOM_ACTION_PAUSE.equals(action)) {
            Log.i(TAG, "handleCustomAction: pause");
            handlePauseRequest();
        } else if (CUSTOM_ACTION_SKIP_NEXT.equals(action)) {
            Log.i(TAG, "handleCustomAction: skip next");
            handleSkipRequest(1);
        } else if (CUSTOM_ACTION_SKIP_PREV.equals(action)) {
            Log.i(TAG, "handleCustomAction: skip prev");
            handleSkipRequest(-1);
        } else if (CUSTOM_ACTION_SCAN_START.equals(action)) {
            Log.i(TAG, "handleCustomAction: scan start");
            handleScanStartRequest();
        } else if (CUSTOM_ACTION_SCAN_STOP.equals(action)) {
            Log.i(TAG, "handleCustomAction: scan stop");
            handleScanStopRequest();
        } else if (CUSTOM_ACTION_FAVORITE.equals(action)) {
            Log.i(TAG, "handleCustomAction: favorite");

            if (mCurrentMedia != null) {
                String mediaId = mCurrentMedia.getMediaId();
                if (isFavoriteStation(mediaId))
                    removeFavoriteStation(mediaId);
                else
                    addFavoriteStation(mediaId, mCurrentMedia.getDescription().getTitle().toString());
            }

            updatePlaybackState();
        } else if (CUSTOM_ACTION_NEXT_CHANNEL.equals(action)) {
            Log.i(TAG, "handleCustomAction: nex channel");
            nextChannel();
        } else if (CUSTOM_ACTION_RECORD.equals(action)) {
            Log.i(TAG, "handleCustomAction: record");
        } else {
            return false;
        }
        return true;
    }

    private void playLastStation() {
        Log.d(TAG, "playLastStation");
        String id = lastStation();

        if (id.isEmpty()) {
            if (mStationList.isEmpty())
                return;
            mCurrentMedia = mStationList.get(0);
            handlePlayRequest();
            return;
        }

        for (MediaBrowserCompat.MediaItem mediaItem: mStationList) {
            if (mediaItem.getMediaId().equals(id)) {
                Log.d(TAG, "playLastStation: station " + id + " found");
                mCurrentMedia = mediaItem;
                handlePlayRequest();
                return;
            }
        }

        String station = id.substring(0, 4);
        String channel = id.substring(4);

        Bundle extras = new Bundle();
        extras.putInt(BUNDLE_KEY_DAB_TYPE, TYPE_DAB_STATION);
        extras.putString(BUNDLE_KEY_STATION_ID, id);
        extras.putString(BUNDLE_KEY_STATION_NAME, station);
        extras.putString(BUNDLE_KEY_CHANNEL, channel);

        mCurrentMedia = new MediaBrowserCompat.MediaItem(new MediaDescriptionCompat.Builder()
                .setMediaId(id)
                .setTitle(station)
                .setExtras(extras)
                .build(), MediaBrowserCompat.MediaItem.FLAG_PLAYABLE);

        handlePlayRequest();
    }

    private void updatePlaybackState() {
        Log.d(TAG, "updateDabStatus");

        Resources resources = getResources();
        PlaybackStateCompat.Builder stateBuilder = new PlaybackStateCompat.Builder();

        long playbackActions = 0;

        // Update playback state
        if (DAB_STATUS_ERROR == mDabStatus && mError != null) {
            stateBuilder.setErrorMessage(-1, mError);
        } else {
            playbackActions |= PlaybackStateCompat.ACTION_PLAY_FROM_MEDIA_ID;
            //TODO playbackActions |= PlaybackStateCompat.ACTION_PLAY_FROM_SEARCH;

            if (0 > mChannelScanProgress) {
                // Playback
                playbackActions |= PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS |
                        PlaybackStateCompat.ACTION_SKIP_TO_NEXT;

                if (mDabStatus == DAB_STATUS_PLAYING) {
                    playbackActions |= PlaybackStateCompat.ACTION_PAUSE;
                } else {
                    playbackActions |= PlaybackStateCompat.ACTION_PLAY;
                }

                stateBuilder.addCustomAction(CUSTOM_ACTION_SCAN_START, resources.getString(R.string.action_scan),
                        android.R.drawable.ic_menu_search);

                stateBuilder.addCustomAction(CUSTOM_ACTION_NEXT_CHANNEL, resources.getString(R.string.action_next_channel),
                        android.R.drawable.ic_menu_upload); //ic_menu_rotate

//TODO record                stateBuilder.addCustomAction(CUSTOM_ACTION_RECORD,
//                        resources.getString(R.string.record), android.R.drawable.ic_menu_add);

                // Set the activeQueueItemId if the current index is valid.
                if (mCurrentMedia != null) {
                    //stateBuilder.setActiveQueueItemId(mCurrentMedia.getQueueId());
//TODO fav                    stateBuilder.addCustomAction(CUSTOM_ACTION_FAVORITE,
//                            resources.getString(R.string.action_favorite),
//                            (isFavoriteStation(mCurrentMedia.getDescription().getMediaId())
//                                    ? android.R.drawable.star_on
//                                    : android.R.drawable.star_off));
                }
            } else {
                // Scanning
                playbackActions |= PlaybackStateCompat.ACTION_PAUSE;
                stateBuilder.addCustomAction(CUSTOM_ACTION_SCAN_STOP, resources.getString(R.string.action_scan),
                        android.R.drawable.ic_menu_close_clear_cancel);
            }

            int playbackState;
            switch (mDabStatus) {
                case DAB_STATUS_ERROR: playbackState = PlaybackStateCompat.STATE_ERROR; break;
                case DAB_STATUS_INITIALISED: playbackState = PlaybackStateCompat.STATE_NONE; break;
                case DAB_STATUS_PLAYING: playbackState = PlaybackStateCompat.STATE_PLAYING; break;
                case DAB_STATUS_PAUSED: playbackState = PlaybackStateCompat.STATE_PAUSED; break;
                case DAB_STATUS_STOPPED: playbackState = PlaybackStateCompat.STATE_STOPPED; break;
                case DAB_STATUS_SCANNING: playbackState = PlaybackStateCompat.STATE_PLAYING; break;
                default: playbackState = PlaybackStateCompat.STATE_NONE;
            }

            stateBuilder.setActions(playbackActions);
            stateBuilder.setState(playbackState, PlaybackStateCompat.PLAYBACK_POSITION_UNKNOWN, 1.0f,
                    SystemClock.elapsedRealtime());
        }

        mSession.setPlaybackState(stateBuilder.build());

        // Update meta data
        updateMetadata();
    }

    private void updateMetadata() {
        if (DAB_STATUS_ERROR == mDabStatus && mError != null) {
            MediaMetadataCompat track = new MediaMetadataCompat.Builder()
                    .putString(MediaMetadataCompat.METADATA_KEY_MEDIA_ID, MEDIA_ID_ERROR)
                    .putString(MediaMetadataCompat.METADATA_KEY_TITLE, mError)
                    .putString(MediaMetadataCompat.METADATA_KEY_DISPLAY_TITLE, mError)
                    .build();

            mSession.setMetadata(track);
        } else if (mTrack != null) {
            mSession.setMetadata(mTrack);
        } else if (0 <= mChannelScanProgress) {
            Resources resources = getResources();
            String title = resources.getString(R.string.label_scanning) + " " + mChannelScanProgress + "%";
            String subTitle = (mCurrentChannel == null) ? "" : mCurrentChannel;

            MediaMetadataCompat track = new MediaMetadataCompat.Builder()
                    .putString(MediaMetadataCompat.METADATA_KEY_MEDIA_ID, MEDIA_ID_CHANNEL_SCAN)
                    .putString(MediaMetadataCompat.METADATA_KEY_TITLE, title)
                    .putString(MediaMetadataCompat.METADATA_KEY_DISPLAY_TITLE, title)
                    .putString(MediaMetadataCompat.METADATA_KEY_DISPLAY_SUBTITLE, subTitle)
                    .build();

            mSession.setMetadata(track);
        } else if (mCurrentMedia != null) {
            String title = mCurrentMedia.getDescription().getTitle().toString();
            Bundle extras = mCurrentMedia.getDescription().getExtras();
            String channel = extras.getString(BUNDLE_KEY_CHANNEL);
            String displayTitle = (channel == null) ? title : title + " (" + channel +")";

            MediaMetadataCompat track = new MediaMetadataCompat.Builder()
                    .putString(MediaMetadataCompat.METADATA_KEY_MEDIA_ID, mCurrentMedia.getMediaId())
                    .putString(MediaMetadataCompat.METADATA_KEY_TITLE, title)
                    .putString(MediaMetadataCompat.METADATA_KEY_DISPLAY_TITLE, displayTitle)
                    .build();

            mSession.setMetadata(track);
        }

        // Update notification
        mNotificationManager.notify(NOTIFICATION_ID, createNotification());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////

    private Notification createNotification() {
        Resources resources = getResources();

        // Update Notification
        NotificationCompat.Builder notificationBuilder = new NotificationCompat.Builder(this);

        if (DAB_STATUS_ERROR == mDabStatus && mError != null) {
            // Error
            notificationBuilder.setStyle(new NotificationCompat.MediaStyle());
            notificationBuilder.setContentTitle(mError);
        } else if (0 <= mChannelScanProgress) {
            // Scanning
            notificationBuilder.setStyle(new NotificationCompat.MediaStyle().setShowActionsInCompactView(new int[]{0}));

            // Stop scan button
            Intent intent = new Intent(CUSTOM_ACTION_SCAN_STOP);
            notificationBuilder.addAction(new NotificationCompat.Action(android.R.drawable.ic_menu_close_clear_cancel,
                    resources.getString(R.string.action_scan),
                    PendingIntent.getBroadcast(this, ACTION_SCAN_STOP, intent, PendingIntent.FLAG_UPDATE_CURRENT)
            ));

            // Title & Text
            if (mTrack != null) {
                notificationBuilder.setContentTitle(mTrack.getText(MediaMetadataCompat.METADATA_KEY_DISPLAY_TITLE));
                notificationBuilder.setContentText(mTrack.getText(MediaMetadataCompat.METADATA_KEY_DISPLAY_SUBTITLE));
            } else {
                notificationBuilder.setContentTitle(resources.getString(R.string.label_scanning) + " " + mChannelScanProgress + "%");
                notificationBuilder.setContentText((mCurrentChannel == null) ? "" : mCurrentChannel);
            }
        } else {
            // Playback
            notificationBuilder.setStyle(new NotificationCompat.MediaStyle().setShowActionsInCompactView(new int[]{0,1,2}));
            Intent intent;

            // Play/Pause toggle button
            if (DAB_STATUS_PLAYING == mDabStatus) {
                intent = new Intent(CUSTOM_ACTION_PAUSE);
                notificationBuilder.addAction(new NotificationCompat.Action(android.R.drawable.ic_media_pause,
                        resources.getString(R.string.action_pause),
                        PendingIntent.getBroadcast(this, ACTION_PLAY, intent, PendingIntent.FLAG_UPDATE_CURRENT)
                ));
            } else {
                intent = new Intent(CUSTOM_ACTION_PLAY);
                notificationBuilder.addAction(new NotificationCompat.Action(android.R.drawable.ic_media_play,
                        resources.getString(R.string.action_play),
                        PendingIntent.getBroadcast(this, ACTION_PAUSE, intent, PendingIntent.FLAG_UPDATE_CURRENT)
                ));
            }


            // Skip button
            intent = new Intent(CUSTOM_ACTION_SKIP_NEXT);
            notificationBuilder.addAction(new NotificationCompat.Action(android.R.drawable.ic_media_next,
                    resources.getString(R.string.action_skip),
                    PendingIntent.getBroadcast(this, ACTION_SKIP_NEXT, intent, PendingIntent.FLAG_UPDATE_CURRENT)
            ));

            // Start scan button
            intent = new Intent(CUSTOM_ACTION_SCAN_START);
            notificationBuilder.addAction(new NotificationCompat.Action(android.R.drawable.ic_menu_search,
                    resources.getString(R.string.action_scan),
                    PendingIntent.getBroadcast(this, ACTION_SCAN_START, intent, PendingIntent.FLAG_UPDATE_CURRENT)
            ));

            // Next channel button
//TODO next chan            intent = new Intent(CUSTOM_ACTION_NEXT_CHANNEL);
//            notificationBuilder.addAction(new NotificationCompat.Action(android.R.drawable.ic_menu_rotate,
//                    resources.getString(R.string.action_next_channel),
//                    PendingIntent.getBroadcast(this, ACTION_NEXT_CHANNEL, intent, PendingIntent.FLAG_UPDATE_CURRENT)
//            ));

            // Title & Text
            if (mTrack != null) {
                notificationBuilder.setContentTitle(mTrack.getText(MediaMetadataCompat.METADATA_KEY_DISPLAY_TITLE));
                notificationBuilder.setContentText(mTrack.getText(MediaMetadataCompat.METADATA_KEY_DISPLAY_SUBTITLE));
            } else if (mCurrentMedia != null) {
                notificationBuilder.setContentTitle(mCurrentMedia.getDescription().getTitle());
                notificationBuilder.setContentText(mCurrentMedia.getDescription().getSubtitle());
            } else {
                notificationBuilder.setContentTitle("");
                notificationBuilder.setContentText("");
            }
        }

        notificationBuilder.setSmallIcon(R.drawable.icon);
        //notificationBuilder.setLargeIcon(aBitmap);
        notificationBuilder.setShowWhen(false);
        notificationBuilder.setVisibility(NotificationCompat.VISIBILITY_PUBLIC);

        return notificationBuilder.build();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////

    private boolean requestAudioFocus() {
        if (mAudioFocus)
            return true;
        Log.d(TAG, "requestAudioFocus");
        AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        int result = audioManager.requestAudioFocus(this, AudioManager.STREAM_MUSIC,
                AudioManager.AUDIOFOCUS_GAIN);
        mAudioFocus = (result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED);
        return mAudioFocus;
    }

    private void releaseAudioFocus() {
        if (!mAudioFocus)
            return;
        Log.d(TAG, "releaseAudioFocus");
        AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        if (audioManager.abandonAudioFocus(this) == AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
            mAudioFocus = false;
        }
    }

    /**
     * Called by AudioManager on audio focus changes.
     * Implementation of {@link android.media.AudioManager.OnAudioFocusChangeListener}.
     */
    @Override
    public void onAudioFocusChange(int focusChange) {
        if (focusChange == AudioManager.AUDIOFOCUS_LOSS) {
            // Permanent loss of audio focus
            // Pause playback immediately
            mAudioFocus = false;
            duckPlayback(false);
            handlePauseRequest();
            //TODO Wait 30 seconds before stopping playback
        } else if (focusChange == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT) {
            // Pause playback
            mAudioFocus = false;
            duckPlayback(false);
            handlePauseRequest();
        } else if (focusChange == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK) {
            // Lower the volume, keep playing
            mAudioFocus = true;
            duckPlayback(true);
        } else if (focusChange == AudioManager.AUDIOFOCUS_GAIN) {
            // Your app has been granted audio focus again
            // Raise volume to normal, restart playback if necessary
            mAudioFocus = true;
            if (mDabStatus != DAB_STATUS_PLAYING)
                handlePlayRequest();
            duckPlayback(false);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////

    public final class DabReceiver extends BroadcastReceiver {
        public DabReceiver() {
            super();
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.i(TAG, "onReceive global: " + action);

            if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
                UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                Log.i(TAG, "USB detached: " + device.getDeviceName());
                if (mUsbDevice != null && mUsbDevice.getDeviceName().equals(device.getDeviceName())) {
                    Log.i(TAG, "USB close");
                    mUsbDevice = null;
                    mUsbConnected = false;
                    mChannelScanProgress = -1;
                    mTrack = null;
                    mCurrentMedia = null;
                    mCurrentChannel = null;
                    mDabStatus = DAB_STATUS_ERROR;
                    mError = getResources().getString(R.string.msg_rtl_sdr_unplugged);
                    updatePlaybackState();
                }
            } else if (ANDROID_AUTO_STATUS.equals(action)) {
                String status = intent.getStringExtra(ANDROID_AUTO_MEDIA_CONNECTION_STATUS);
                boolean isConnectedToCar = ANDROID_AUTO_MEDIA_CONNECTED.equals(status);
                Log.i(TAG, "AA is connected: " + isConnectedToCar);
                Bundle aa_extras = intent.getExtras();
                for (String key : aa_extras.keySet())
                    Log.i(TAG, "AA key:" + key + " connected: " + aa_extras.getString(key));
            } else {
                handleCustomAction(action, intent.getExtras());
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////

    /** Called when the service is being created. */
    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "Service created");

        instance = this;

        // Start a new MediaSession.
        mSession = new MediaSessionCompat(this, TAG);
        mSession.setCallback(new MediaSessionCallback());
        mSession.setFlags(MediaSessionCompat.FLAG_HANDLES_MEDIA_BUTTONS | MediaSessionCompat.FLAG_HANDLES_TRANSPORT_CONTROLS);

        Context context = getApplicationContext();

        // This is an Intent to launch the app's UI, used primarily by the ongoing notification.
//TODO        Intent intent = new Intent(context, DabDelegate.class);
//        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
//        PendingIntent pi = PendingIntent.getActivity(context, ACTION_OPEN, intent,
//                PendingIntent.FLAG_UPDATE_CURRENT);
//        mSession.setSessionActivity(pi);

        Bundle sessionExtras = new Bundle();
        sessionExtras.putBoolean(SLOT_RESERVATION_QUEUE, true);
        sessionExtras.putBoolean(SLOT_RESERVATION_SKIP_TO_PREV, true);
        sessionExtras.putBoolean(SLOT_RESERVATION_SKIP_TO_NEXT, true);
        mSession.setExtras(sessionExtras);

        if (!mSession.isActive()) {
            mSession.setActive(true);
            Log.d(TAG, "media session is active");
        }

        // Register receiver
        mDabReceiver = new DabReceiver();
        IntentFilter filter = new IntentFilter();
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        filter.addAction(Intent.ACTION_MEDIA_BUTTON);
        filter.addAction(CUSTOM_ACTION_PLAY);
        filter.addAction(CUSTOM_ACTION_PAUSE);
        filter.addAction(CUSTOM_ACTION_SKIP_NEXT);
        filter.addAction(CUSTOM_ACTION_SKIP_PREV);
        filter.addAction(CUSTOM_ACTION_SCAN_START);
        filter.addAction(CUSTOM_ACTION_SCAN_STOP);
        filter.addAction(CUSTOM_ACTION_FAVORITE);
        filter.addAction(CUSTOM_ACTION_RECORD);
        filter.addAction(CUSTOM_ACTION_NEXT_CHANNEL);
        registerReceiver(mDabReceiver,filter);

        // Notification
        mNotificationManager = NotificationManagerCompat.from(this);
        startForeground(NOTIFICATION_ID, createNotification());

        // Playback state
        updatePlaybackState();
    }

    /** The service is starting, due to a call to startService() */
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        super.onStartCommand(intent, flags, startId);
        Log.i(TAG, "Service started, startId: " + startId);
        if (intent == null)
            return START_STICKY;

        if (intent.getAction().equals(UsbManager.ACTION_USB_DEVICE_ATTACHED)) {
            UsbDevice usbDevice = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
            Log.d(TAG, "USB attached: " + usbDevice.getDeviceName());
            if (mUsbDevice == null) {
                mUsbDevice = usbDevice;
                openUsbDevice();
            }
        } else {
            Log.d(TAG, "onStartCommand action: " + intent.getAction());
        }
        return Service.START_STICKY;
    }

    /** A client is binding to the service with bindService() */
    @Override
    public IBinder onBind(Intent intent) {
        super.onBind(intent);
        Log.i(TAG, "Service bound");
        return mBinder;
    }


    /** Called when all clients have unbound with unbindService() */
    @Override
    public boolean onUnbind(Intent intent) {
        return super.onUnbind(intent);
    }

    /** Called when a client is binding to the service with bindService()*/
    @Override
    public void onRebind(Intent intent) {
        super.onRebind(intent);
    }

    /** Called when The service is no longer used and is being destroyed */
    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.i(TAG, "Service destroyed");
        stopPlayback();
        //TODO close USB device

        unregisterReceiver(mDabReceiver);
        mNotificationManager.cancel(NOTIFICATION_ID);
        stopForeground(true);
    }

    private void initDabCallback() {
        if (mDabCallback == null)
            return;
        mDabCallback.updateStationList(mStationList);
        mDabCallback.updateFavoriteList(mFavoriteList);
        mDabCallback.updateDabStatus(mDabStatus);
        mDabCallback.updateScanProgress(mChannelScanProgress);
        if (DAB_STATUS_ERROR == mDabStatus)
            mDabCallback.showError(mError);
    }
}
