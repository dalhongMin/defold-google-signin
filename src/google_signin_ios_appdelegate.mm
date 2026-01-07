// iOS AppDelegate extension for Google Sign-In URL handling

#if defined(DM_PLATFORM_IOS)

#import <Foundation/Foundation.h>
#import <GoogleSignIn/GoogleSignIn.h>

// This function is called by Defold's app delegate
extern "C" bool GoogleSignIn_HandleOpenURL(id url, id options)
{
    return [GIDSignIn.sharedInstance handleURL:(NSURL*)url];
}

#endif
