package com.defold.google_signin;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

/**
 * Transparent helper activity to handle Google Sign-In ActivityResult.
 * This activity launches the sign-in intent and forwards the result back.
 */
public class GoogleSignInHelperActivity extends Activity {
    private static final String TAG = "GoogleSignInHelper";
    private static final int RC_SIGN_IN = 9001;
    private static final String EXTRA_SIGN_IN_INTENT = "sign_in_intent";

    private static ResultCallback sCallback;

    public interface ResultCallback {
        void onResult(Intent data);
        void onCancelled();
    }

    public static void launch(Activity parentActivity, Intent signInIntent, ResultCallback callback) {
        sCallback = callback;

        Intent intent = new Intent(parentActivity, GoogleSignInHelperActivity.class);
        intent.putExtra(EXTRA_SIGN_IN_INTENT, signInIntent);
        intent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
        parentActivity.startActivity(intent);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Log.d(TAG, "GoogleSignInHelperActivity created");

        Intent signInIntent = getIntent().getParcelableExtra(EXTRA_SIGN_IN_INTENT);
        if (signInIntent != null) {
            Log.d(TAG, "Starting sign-in activity for result");
            startActivityForResult(signInIntent, RC_SIGN_IN);
        } else {
            Log.e(TAG, "No sign-in intent provided");
            if (sCallback != null) {
                sCallback.onCancelled();
                sCallback = null;
            }
            finish();
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        Log.d(TAG, "onActivityResult: requestCode=" + requestCode + ", resultCode=" + resultCode);

        if (requestCode == RC_SIGN_IN) {
            if (sCallback != null) {
                if (resultCode == RESULT_OK || data != null) {
                    Log.d(TAG, "Sign-in result received, forwarding to callback");
                    sCallback.onResult(data);
                } else {
                    Log.d(TAG, "Sign-in cancelled or failed");
                    sCallback.onCancelled();
                }
                sCallback = null;
            }
        }

        finish();
        overridePendingTransition(0, 0); // No animation
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "GoogleSignInHelperActivity destroyed");
    }
}
