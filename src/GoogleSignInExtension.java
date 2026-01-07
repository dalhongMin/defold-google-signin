package com.defold.google_signin;

import android.app.Activity;
import android.content.Intent;
import android.util.Log;

import com.google.android.gms.auth.api.signin.GoogleSignIn;
import com.google.android.gms.auth.api.signin.GoogleSignInAccount;
import com.google.android.gms.auth.api.signin.GoogleSignInClient;
import com.google.android.gms.auth.api.signin.GoogleSignInOptions;
import com.google.android.gms.common.api.ApiException;
import com.google.android.gms.tasks.Task;
import com.google.android.gms.tasks.OnCompleteListener;

public class GoogleSignInExtension {
    private static final String TAG = "GoogleSignIn";

    private static Activity sActivity;
    private static GoogleSignInClient sGoogleSignInClient;
    private static String sClientId;
    private static boolean sIsSignedIn = false;

    // Native callback - will be called from C++
    public static native void onSignInResult(boolean success, String idToken, String userId, String email, String displayName, String error);

    public static void init(final Activity activity, final String clientId) {
        sActivity = activity;
        sClientId = clientId;

        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    GoogleSignInOptions gso = new GoogleSignInOptions.Builder(GoogleSignInOptions.DEFAULT_SIGN_IN)
                            .requestIdToken(clientId)
                            .requestEmail()
                            .requestProfile()
                            .build();

                    sGoogleSignInClient = GoogleSignIn.getClient(activity, gso);

                    // Check if already signed in
                    GoogleSignInAccount account = GoogleSignIn.getLastSignedInAccount(activity);
                    sIsSignedIn = (account != null);

                    Log.d(TAG, "Google Sign-In initialized, already signed in: " + sIsSignedIn);
                } catch (Exception e) {
                    Log.e(TAG, "Failed to initialize Google Sign-In", e);
                }
            }
        });
    }

    public static void login() {
        if (sActivity == null || sGoogleSignInClient == null) {
            Log.e(TAG, "Google Sign-In not initialized");
            sendResult(false, null, null, null, null, "Not initialized");
            return;
        }

        sActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    Log.d(TAG, "Starting Google Sign-In flow...");

                    // First try silent sign-in
                    sGoogleSignInClient.silentSignIn().addOnCompleteListener(sActivity, new OnCompleteListener<GoogleSignInAccount>() {
                        @Override
                        public void onComplete(Task<GoogleSignInAccount> task) {
                            if (task.isSuccessful()) {
                                // Silent sign-in succeeded
                                Log.d(TAG, "Silent sign-in successful");
                                handleSignInResult(task);
                            } else {
                                // Silent sign-in failed, need interactive sign-in
                                Log.d(TAG, "Silent sign-in failed, starting interactive sign-in...");
                                startInteractiveSignIn();
                            }
                        }
                    });
                } catch (Exception e) {
                    Log.e(TAG, "Failed to start sign-in", e);
                    sendResult(false, null, null, null, null, "Failed to start sign-in: " + e.getMessage());
                }
            }
        });
    }

    private static void startInteractiveSignIn() {
        try {
            Intent signInIntent = sGoogleSignInClient.getSignInIntent();

            // Use a helper activity to handle the result
            GoogleSignInHelperActivity.launch(sActivity, signInIntent, new GoogleSignInHelperActivity.ResultCallback() {
                @Override
                public void onResult(Intent data) {
                    Task<GoogleSignInAccount> task = GoogleSignIn.getSignedInAccountFromIntent(data);
                    handleSignInResult(task);
                }

                @Override
                public void onCancelled() {
                    sendResult(false, null, null, null, null, "Sign-in cancelled");
                }
            });
        } catch (Exception e) {
            Log.e(TAG, "Failed to start interactive sign-in", e);
            sendResult(false, null, null, null, null, "Failed to start sign-in: " + e.getMessage());
        }
    }

    public static void logout() {
        if (sGoogleSignInClient != null && sActivity != null) {
            sActivity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    sGoogleSignInClient.signOut();
                    sIsSignedIn = false;
                    Log.d(TAG, "Signed out");
                }
            });
        }
    }

    public static boolean isSignedIn() {
        return sIsSignedIn;
    }

    private static void handleSignInResult(Task<GoogleSignInAccount> completedTask) {
        try {
            GoogleSignInAccount account = completedTask.getResult(ApiException.class);
            sIsSignedIn = true;

            String idToken = account.getIdToken();
            String userId = account.getId();
            String email = account.getEmail();
            String displayName = account.getDisplayName();

            Log.d(TAG, "Sign-in successful: " + email);
            sendResult(true, idToken, userId, email, displayName, null);

        } catch (ApiException e) {
            sIsSignedIn = false;
            String errorMessage = "Sign-in failed (code " + e.getStatusCode() + "): " + getStatusCodeMessage(e.getStatusCode());
            Log.e(TAG, errorMessage, e);
            sendResult(false, null, null, null, null, errorMessage);
        } catch (Exception e) {
            sIsSignedIn = false;
            String errorMessage = "Sign-in error: " + e.getMessage();
            Log.e(TAG, errorMessage, e);
            sendResult(false, null, null, null, null, errorMessage);
        }
    }

    private static String getStatusCodeMessage(int statusCode) {
        switch (statusCode) {
            case 7: return "Network error";
            case 10: return "Developer error (check SHA-1 and package name)";
            case 12500: return "Sign-in failed";
            case 12501: return "Sign-in cancelled by user";
            case 12502: return "Sign-in currently in progress";
            default: return "Unknown error";
        }
    }

    private static void sendResult(final boolean success, final String idToken, final String userId,
                                   final String email, final String displayName, final String error) {
        // Call native callback
        try {
            onSignInResult(success, idToken, userId, email, displayName, error);
        } catch (Exception e) {
            Log.e(TAG, "Failed to call native callback", e);
        }
    }

    // Silent sign-in for auto-login
    public static void silentSignIn() {
        if (sGoogleSignInClient == null || sActivity == null) {
            return;
        }

        sActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                sGoogleSignInClient.silentSignIn().addOnCompleteListener(new OnCompleteListener<GoogleSignInAccount>() {
                    @Override
                    public void onComplete(Task<GoogleSignInAccount> task) {
                        handleSignInResult(task);
                    }
                });
            }
        });
    }
}
