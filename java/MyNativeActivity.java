
import android.app.NativeActivity;

public class MyNativeActivity extends NativeActivity {
    static {
        System.loadLibrary("nativedroidblaster");
    }
}
