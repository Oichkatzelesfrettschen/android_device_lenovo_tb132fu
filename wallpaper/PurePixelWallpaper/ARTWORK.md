# Pure Pixel default wallpaper artwork

The two approved 1586 x 992 PNG masters were generated with OpenAI's built-in
image generation tool on 2026-08-31. They are kept byte-for-byte in
`res/drawable-nodpi`; Android center-crops them at runtime without modifying the
source artwork.

- `pure_pixel_wallpaper_dark.png`
  - SHA-256: `122240e152d21c74810a1adab5d4eec22ad614b009b19442a78be24166dbd0f2`
- `pure_pixel_wallpaper_light.png`
  - SHA-256: `fb60a996207361713d2d70b4662ad7ddbf809474011337aa35db19be6ba4edc6`

The original prompt requested an original premium Material You/Pixel-like
tablet wallpaper with OLED navy negative space, ceramic/frosted-glass ribbons,
blue and indigo accents, a restrained coral accent, safe portrait/landscape
cropping, and no text, logo, or trademark. The light variant retained the exact
forms and composition while changing only lighting and color to pearl and pale
blue.

The wallpaper service selects the light or dark master from the system theme.
It is deliberately event-driven: there is no animation, timer, network access,
or wake lock.
