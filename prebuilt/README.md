# Lenovo pen prebuilt

`PenService.apk` is a proprietary Lenovo stock component and is intentionally
not committed. Extract it from the matching TB132FU stock image and verify:

```text
SHA-256  7d43d5a67de8934bd96dfa4119caf145937f78353d25f493fcc812a661f967ef
package  com.lenovo.penservice
version  1.3.7.021620241106 (13070216)
```

The build re-signs this package with the ROM platform certificate because its
manifest uses `android.uid.system`.

## Pen notification audio

Pure Pixel 17 R1 also uses two small pen notification audio payloads. Their
redistribution license has not been established, so they are intentionally
excluded from this repository and ignored by Git. Supply legally obtained
copies at these paths and verify them before building:

```text
prebuilt/pen_mono.wav
SHA-256  25fe878cb7ebc5c5f41a1a611490a1a0b20b84e812753b435fea5643d6f0e128

prebuilt/pen_sound.ogg
SHA-256  04708555177533bb864b32a8657ae7fd2c1eaa448ed5d29cd9077dd805fa1c5a
```

These media files, `PenService.apk`, ROM signing material, Google packages,
and generated partition images are not part of the public device source.
