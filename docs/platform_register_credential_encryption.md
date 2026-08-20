# MQTT Device Register Credential Encryption

## Device-side contract

The device publishes `NET_DEVICE_REGISTER` to `device/<SN>/register` after the MQTT connection is established. A wired uplink selects `StreamMode = "rtsp"`; only then are the RTSP URL and credentials sent in encrypted form. A wireless or unresolved uplink selects `StreamMode = "rtmp"`, sends `Data.Rtmp.Url`, and does not send RTSP credentials.

The encrypted credential object is located at `Data.Credential`:

```json
{
  "Version": 1,
  "Algorithm": "RSA-OAEP-SHA256+A256GCM",
  "KeyId": "platform-rsa-2026-08",
  "EncryptedKey": "Base64(RSA-OAEP-SHA256(AES-256 key))",
  "Nonce": "Base64(12 bytes)",
  "Ciphertext": "Base64(AES-256-GCM ciphertext)",
  "Tag": "Base64(16 bytes)"
}
```

The plaintext after successful decryption is UTF-8 JSON:

```json
{
  "RtspUrl": "rtsp://172.16.25.229:554/Streaming/Channels/101",
  "Username": "admin",
  "Password": "example-password"
}
```

`Data.Rtsp` is intentionally absent. If the device cannot read the public key or encrypt the RTSP credential, it sends `Data.CredentialState = "unavailable"` and never falls back to plaintext RTSP credentials. RTMP registrations send `Data.CredentialState = "not_required"` instead.

## AAD

AES-GCM uses the following UTF-8 AAD string. The platform must recreate it byte-for-byte before decrypting:

```text
NET_DEVICE_REGISTER|1|<SN>|<RequestId>|<Timestamp>|<UplinkType>|<StreamMode>
```

`SN`, `RequestId`, `Timestamp`, `UplinkType`, and `StreamMode` are the values in the outer MQTT JSON. A GCM tag validation failure means that either the ciphertext or one of these outer fields has been changed.

## Key deployment

The device reads the PEM-encoded RSA public key from:

```text
/opt/cam/cert/trust/platform_register_public.pem
```

The platform generates and protects the matching private key. For example, on a secure platform administration host:

```sh
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:3072 -out platform_register_private.pem
openssl pkey -in platform_register_private.pem -pubout -out platform_register_public.pem
```

Only `platform_register_public.pem` is copied into the device firmware or installed at the path above. The private key must remain in the platform key store, must not be committed to source control, and must not be sent through MQTT.

The platform must decrypt `EncryptedKey` with RSA-OAEP using SHA-256 for both OAEP and MGF1, verify that the result is exactly 32 bytes, then decrypt `Ciphertext` with AES-256-GCM using `Nonce`, `Tag`, and the AAD above. The PHP implementation must use a crypto API that permits explicit OAEP SHA-256 selection; do not use an API that silently defaults OAEP to SHA-1.

## Platform validation

Before using the decrypted RTSP data, the platform must:

1. Require `Algorithm` to equal `RSA-OAEP-SHA256+A256GCM` and accept only configured `KeyId` values.
2. Reject timestamps outside the configured acceptance window, recommended at five minutes.
3. Deduplicate `SN + RequestId` for at least ten minutes to prevent replay.
4. Reject malformed Base64, a non-32-byte unwrapped AES key, and every GCM tag failure.
5. Keep decrypted credentials in memory only. Do not store or log the password or a credential-bearing RTSP URL.

MQTT TLS is still recommended. This message-level encryption protects the RTSP credential fields, but it does not hide MQTT topics, device metadata, broker credentials, or the later RTSP session.
