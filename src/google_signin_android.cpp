// Android-specific implementation for Google Sign-In

#if defined(DM_PLATFORM_ANDROID)

#include <dmsdk/sdk.h>
#include <dmsdk/dlib/log.h>
#include <dmsdk/graphics/graphics.h>
#include <jni.h>

// Global references
static jobject g_Activity = nullptr;
static jclass g_GoogleSignInClass = nullptr;
static jmethodID g_InitMethod = nullptr;
static jmethodID g_LoginMethod = nullptr;
static jmethodID g_LogoutMethod = nullptr;
static jmethodID g_IsSignedInMethod = nullptr;
static bool g_Initialized = false;

// Forward declaration of callback
extern "C" void GoogleSignIn_OnResult(bool success, const char* id_token, const char* user_id, const char* email, const char* display_name, const char* error);

// Get JavaVM from Defold
static JavaVM* GetJavaVM()
{
    return (JavaVM*)dmGraphics::GetNativeAndroidJavaVM();
}

// RAII class for JNI thread attachment
struct AttachScope
{
    JNIEnv* m_Env;
    bool m_Attached;
    JavaVM* m_VM;

    AttachScope() : m_Env(nullptr), m_Attached(false), m_VM(nullptr)
    {
        m_VM = GetJavaVM();
        if (m_VM == nullptr) return;

        int status = m_VM->GetEnv((void**)&m_Env, JNI_VERSION_1_6);
        if (status == JNI_EDETACHED)
        {
            if (m_VM->AttachCurrentThread(&m_Env, nullptr) == 0)
            {
                m_Attached = true;
            }
        }
        else if (status == JNI_OK)
        {
            m_Attached = false; // Already attached
        }
    }

    ~AttachScope()
    {
        if (m_Attached && m_VM)
        {
            m_VM->DetachCurrentThread();
        }
    }

    JNIEnv* GetEnv() { return m_Env; }
};

// JNI callback from Java - receives sign-in result
extern "C" JNIEXPORT void JNICALL Java_com_defold_google_1signin_GoogleSignInExtension_onSignInResult(
    JNIEnv* env, jclass cls,
    jboolean success,
    jstring idToken,
    jstring userId,
    jstring email,
    jstring displayName,
    jstring error)
{
    const char* idTokenStr = idToken ? env->GetStringUTFChars(idToken, nullptr) : nullptr;
    const char* userIdStr = userId ? env->GetStringUTFChars(userId, nullptr) : nullptr;
    const char* emailStr = email ? env->GetStringUTFChars(email, nullptr) : nullptr;
    const char* displayNameStr = displayName ? env->GetStringUTFChars(displayName, nullptr) : nullptr;
    const char* errorStr = error ? env->GetStringUTFChars(error, nullptr) : nullptr;

    GoogleSignIn_OnResult(success, idTokenStr, userIdStr, emailStr, displayNameStr, errorStr);

    if (idTokenStr) env->ReleaseStringUTFChars(idToken, idTokenStr);
    if (userIdStr) env->ReleaseStringUTFChars(userId, userIdStr);
    if (emailStr) env->ReleaseStringUTFChars(email, emailStr);
    if (displayNameStr) env->ReleaseStringUTFChars(displayName, displayNameStr);
    if (errorStr) env->ReleaseStringUTFChars(error, errorStr);
}

static jclass FindClass(JNIEnv* env, jobject activity, const char* name)
{
    jclass activityClass = env->GetObjectClass(activity);
    jmethodID getClassLoader = env->GetMethodID(activityClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject classLoader = env->CallObjectMethod(activity, getClassLoader);

    jclass classLoaderClass = env->FindClass("java/lang/ClassLoader");
    jmethodID loadClass = env->GetMethodID(classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

    jstring className = env->NewStringUTF(name);
    jclass cls = (jclass)env->CallObjectMethod(classLoader, loadClass, className);

    env->DeleteLocalRef(className);
    env->DeleteLocalRef(activityClass);
    env->DeleteLocalRef(classLoaderClass);
    env->DeleteLocalRef(classLoader);

    return cls;
}

static bool InitializeJNI()
{
    if (g_Initialized) return true;

    JavaVM* vm = GetJavaVM();
    if (vm == nullptr)
    {
        dmLogError("Google Sign-In: JavaVM not available");
        return false;
    }

    AttachScope attach;
    JNIEnv* env = attach.GetEnv();
    if (!env)
    {
        dmLogError("Google Sign-In: Failed to get JNI environment");
        return false;
    }

    // Get activity from Defold
    jobject activity = (jobject)dmGraphics::GetNativeAndroidActivity();
    if (!activity)
    {
        dmLogError("Google Sign-In: Failed to get Android activity");
        return false;
    }

    g_Activity = env->NewGlobalRef(activity);

    // Find class using app class loader
    jclass cls = FindClass(env, activity, "com.defold.google_signin.GoogleSignInExtension");
    if (!cls)
    {
        dmLogError("Google Sign-In: Failed to find GoogleSignInExtension class");
        return false;
    }

    g_GoogleSignInClass = (jclass)env->NewGlobalRef(cls);
    env->DeleteLocalRef(cls);

    g_InitMethod = env->GetStaticMethodID(g_GoogleSignInClass, "init", "(Landroid/app/Activity;Ljava/lang/String;)V");
    g_LoginMethod = env->GetStaticMethodID(g_GoogleSignInClass, "login", "()V");
    g_LogoutMethod = env->GetStaticMethodID(g_GoogleSignInClass, "logout", "()V");
    g_IsSignedInMethod = env->GetStaticMethodID(g_GoogleSignInClass, "isSignedIn", "()Z");

    if (!g_InitMethod || !g_LoginMethod || !g_LogoutMethod || !g_IsSignedInMethod)
    {
        dmLogError("Google Sign-In: Failed to find required Java methods");
        return false;
    }

    g_Initialized = true;
    dmLogInfo("Google Sign-In: JNI initialized successfully");
    return true;
}

void GoogleSignIn_Init(const char* client_id)
{
    if (!InitializeJNI())
    {
        return;
    }

    AttachScope attach;
    JNIEnv* env = attach.GetEnv();
    if (!env)
    {
        dmLogError("Google Sign-In: Failed to attach JNI for init");
        return;
    }

    jstring jClientId = env->NewStringUTF(client_id);
    env->CallStaticVoidMethod(g_GoogleSignInClass, g_InitMethod, g_Activity, jClientId);
    env->DeleteLocalRef(jClientId);

    if (env->ExceptionCheck())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
        dmLogError("Google Sign-In: Exception during init");
        return;
    }

    dmLogInfo("Google Sign-In: Initialized with client_id");
}

void GoogleSignIn_Login()
{
    if (!g_Initialized)
    {
        GoogleSignIn_OnResult(false, nullptr, nullptr, nullptr, nullptr, "Not initialized");
        return;
    }

    AttachScope attach;
    JNIEnv* env = attach.GetEnv();
    if (!env)
    {
        GoogleSignIn_OnResult(false, nullptr, nullptr, nullptr, nullptr, "JNI attach failed");
        return;
    }

    env->CallStaticVoidMethod(g_GoogleSignInClass, g_LoginMethod);

    if (env->ExceptionCheck())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
        GoogleSignIn_OnResult(false, nullptr, nullptr, nullptr, nullptr, "Exception during login");
    }
}

void GoogleSignIn_Logout()
{
    if (!g_Initialized) return;

    AttachScope attach;
    JNIEnv* env = attach.GetEnv();
    if (!env) return;

    env->CallStaticVoidMethod(g_GoogleSignInClass, g_LogoutMethod);

    if (env->ExceptionCheck())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}

bool GoogleSignIn_IsSignedIn()
{
    if (!g_Initialized) return false;

    AttachScope attach;
    JNIEnv* env = attach.GetEnv();
    if (!env) return false;

    jboolean result = env->CallStaticBooleanMethod(g_GoogleSignInClass, g_IsSignedInMethod);

    if (env->ExceptionCheck())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return false;
    }

    return result;
}

#endif // DM_PLATFORM_ANDROID
