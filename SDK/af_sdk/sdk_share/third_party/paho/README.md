# Eclipse Paho MQTT C headers

These headers are vendored from Eclipse Paho MQTT C and retain their upstream
license notices. The SDK platform runtime loads `paho-mqtt3a` dynamically at
runtime, so this directory contributes compile-time declarations only and does
not embed or redistribute a Paho binary.

The target device must provide a compatible `libpaho-mqtt3a.so.1`, or the host
must set `NET_PlatformConfig_S::strMqttRuntimeLibrary` to an explicit compatible
runtime library path.
