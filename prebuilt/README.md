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
