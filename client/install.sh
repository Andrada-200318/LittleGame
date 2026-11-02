#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

STUDIO_JDK="$HOME/.local/share/JetBrains/Toolbox/apps/android-studio/jbr"
if [ -d "$STUDIO_JDK" ]; then
    export JAVA_HOME="$STUDIO_JDK"
fi

SDK_DIR=$(sed -n 's/^sdk\.dir=//p' local.properties)
ADB="$SDK_DIR/platform-tools/adb"
APK="app/build/outputs/apk/debug/app-debug.apk"

mapfile -t USB_PHONES < <("$ADB" devices -l | awk '$2 == "device" && / usb:/ { print $1 }')

if [ "${#USB_PHONES[@]}" -eq 0 ]; then
    echo "No phone connected by USB."
    if "$ADB" devices | awk 'NR > 1 { print $2 }' | grep -q unauthorized; then
        echo "A phone is connected but not allowed yet: unlock it and tap Allow on the 'Allow USB debugging?' popup."
    else
        echo "Plug the phone in with a USB cable and turn on USB debugging (Settings > Developer options)."
    fi
    exit 1
fi

echo "Building..."
./gradlew assembleDebug --quiet

for phone in "${USB_PHONES[@]}"; do
    model=$("$ADB" -s "$phone" shell getprop ro.product.model | tr -d '\r')
    echo "Installing on $model ($phone)..."
    "$ADB" -s "$phone" install -r "$APK"
    "$ADB" -s "$phone" shell am start -n com.littleimpostorgame/.MainActivity > /dev/null
done

echo "Done: installed and opened on ${#USB_PHONES[@]} phone(s)."
