// iOS-specific implementation for Google Sign-In

#if defined(DM_PLATFORM_IOS)

#include <dmsdk/sdk.h>
#include <dmsdk/dlib/log.h>

#import <GoogleSignIn/GoogleSignIn.h>
#import <UIKit/UIKit.h>

static NSString* g_ClientId = nil;
static BOOL g_IsSignedIn = NO;

// Forward declaration of callback
extern "C" void GoogleSignIn_OnResult(bool success, const char* id_token, const char* user_id, const char* email, const char* display_name, const char* error);

@interface GoogleSignInDelegate : NSObject
+ (instancetype)sharedInstance;
- (void)signIn;
- (void)signOut;
- (BOOL)isSignedIn;
@end

@implementation GoogleSignInDelegate

+ (instancetype)sharedInstance {
    static GoogleSignInDelegate* instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[GoogleSignInDelegate alloc] init];
    });
    return instance;
}

- (void)signIn {
    UIViewController* rootViewController = [UIApplication sharedApplication].keyWindow.rootViewController;

    GIDConfiguration* config = [[GIDConfiguration alloc] initWithClientID:g_ClientId];

    [GIDSignIn.sharedInstance signInWithConfiguration:config
                             presentingViewController:rootViewController
                                             callback:^(GIDGoogleUser* user, NSError* error) {
        if (error) {
            g_IsSignedIn = NO;
            NSString* errorMessage = [NSString stringWithFormat:@"Sign-in failed: %@", error.localizedDescription];
            GoogleSignIn_OnResult(false, nullptr, nullptr, nullptr, nullptr, [errorMessage UTF8String]);
            return;
        }

        g_IsSignedIn = YES;

        NSString* idToken = user.authentication.idToken;
        NSString* userId = user.userID;
        NSString* email = user.profile.email;
        NSString* displayName = user.profile.name;

        GoogleSignIn_OnResult(true,
                            idToken ? [idToken UTF8String] : nullptr,
                            userId ? [userId UTF8String] : nullptr,
                            email ? [email UTF8String] : nullptr,
                            displayName ? [displayName UTF8String] : nullptr,
                            nullptr);
    }];
}

- (void)signOut {
    [GIDSignIn.sharedInstance signOut];
    g_IsSignedIn = NO;
}

- (BOOL)isSignedIn {
    return g_IsSignedIn || (GIDSignIn.sharedInstance.currentUser != nil);
}

@end

// C++ interface
void GoogleSignIn_Init(const char* client_id)
{
    g_ClientId = [NSString stringWithUTF8String:client_id];

    // Check if already signed in
    if (GIDSignIn.sharedInstance.currentUser) {
        g_IsSignedIn = YES;
    }

    dmLogInfo("Google Sign-In initialized on iOS");
}

void GoogleSignIn_Login()
{
    if (!g_ClientId) {
        GoogleSignIn_OnResult(false, nullptr, nullptr, nullptr, nullptr, "Not initialized");
        return;
    }

    dispatch_async(dispatch_get_main_queue(), ^{
        [[GoogleSignInDelegate sharedInstance] signIn];
    });
}

void GoogleSignIn_Logout()
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [[GoogleSignInDelegate sharedInstance] signOut];
    });
}

bool GoogleSignIn_IsSignedIn()
{
    return [[GoogleSignInDelegate sharedInstance] isSignedIn];
}

// Handle URL for OAuth callback
extern "C" bool GoogleSignIn_HandleURL(NSURL* url)
{
    return [GIDSignIn.sharedInstance handleURL:url];
}

#endif // DM_PLATFORM_IOS
