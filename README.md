# Defold Google Sign-In Extension

Google Sign-In native extension for Defold game engine (Android/iOS).

## Installation

Add to your `game.project` dependencies:

```ini
[project]
dependencies#N = https://github.com/dalhongMin/defold-google-signin/archive/refs/tags/1.0.0.zip
```

## Setup

### Android

1. Add your `google-services.json` to `bundle/android/`
2. Get your Web Client ID from Google Cloud Console

### iOS

1. Add your `GoogleService-Info.plist` to your project
2. Configure URL schemes in Info.plist

## Usage

```lua
local google_signin = require("google_signin")

-- Initialize with Web Client ID
google_signin.init("YOUR_WEB_CLIENT_ID.apps.googleusercontent.com")

-- Start sign-in
google_signin.sign_in()

-- Poll for result (call in update)
local result = google_signin.poll_result()
if result then
    if result.success then
        print("ID Token: " .. result.id_token)
        print("Email: " .. result.email)
        print("Display Name: " .. result.display_name)
    else
        print("Error: " .. result.error)
    end
end

-- Sign out
google_signin.sign_out()
```

## API

### `google_signin.init(web_client_id)`
Initialize the extension with your Web Client ID.

### `google_signin.sign_in()`
Start the Google Sign-In flow. Opens Google's sign-in UI.

### `google_signin.poll_result()`
Poll for sign-in result. Returns `nil` if no result yet, or a table:
- `success` (boolean): Whether sign-in succeeded
- `id_token` (string): Firebase-compatible ID token
- `email` (string): User's email
- `display_name` (string): User's display name
- `error` (string): Error message if failed

### `google_signin.sign_out()`
Sign out the current user.

## Notes

- Uses polling pattern instead of callbacks to avoid JNI threading issues
- Compatible with Firebase Authentication

## License

MIT License
