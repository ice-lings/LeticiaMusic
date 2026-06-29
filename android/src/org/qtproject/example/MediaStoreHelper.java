package org.qtproject.example;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.provider.MediaStore;
import java.util.ArrayList;

/* 通过 MediaStore 查询设备上所有音频文件 */
public class MediaStoreHelper {

    /* 查询所有音频文件，返回文件路径数组 */
    public static String[] queryAllAudio(Context context) {
        ArrayList<String> paths = new ArrayList<>();

        Uri uri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;

        String[] projection = {
            MediaStore.Audio.Media._ID,
            MediaStore.Audio.Media.DATA,
            MediaStore.Audio.Media.TITLE,
            MediaStore.Audio.Media.ARTIST,
            MediaStore.Audio.Media.ALBUM,
            MediaStore.Audio.Media.DURATION,
            MediaStore.Audio.Media.RELATIVE_PATH,
            MediaStore.Audio.Media.DISPLAY_NAME
        };

        String selection = MediaStore.Audio.Media.IS_MUSIC + " != 0";
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            selection += " AND " + MediaStore.Audio.Media.IS_PENDING + " != 1";
        }

        ContentResolver resolver = context.getContentResolver();
        Cursor cursor = null;

        try {
            cursor = resolver.query(uri, projection, selection, null, null);
            if (cursor != null) {
                int dataColumn = cursor.getColumnIndex(MediaStore.Audio.Media.DATA);
                int relativePathColumn = cursor.getColumnIndex(MediaStore.Audio.Media.RELATIVE_PATH);
                int displayNameColumn = cursor.getColumnIndex(MediaStore.Audio.Media.DISPLAY_NAME);

                while (cursor.moveToNext()) {
                    String path = dataColumn >= 0 ? cursor.getString(dataColumn) : null;

                    // Android 11+ scoped storage: DATA 可能为空，手动拼接路径
                    if (path == null || path.isEmpty()) {
                        String relativePath = relativePathColumn >= 0
                            ? cursor.getString(relativePathColumn) : "";
                        String displayName = displayNameColumn >= 0
                            ? cursor.getString(displayNameColumn) : "";
                        if (displayName != null && !displayName.isEmpty()) {
                            path = "/storage/emulated/0/" + relativePath + displayName;
                        }
                    }

                    if (path != null && !path.isEmpty()) {
                        paths.add(path);
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }

        return paths.toArray(new String[0]);
    }
}
