"""TB132FU-specific classic recovery OTA assertions."""


_DEFAULT_ASSERTION_FRAGMENTS = (
    'getprop("ro.product.device") == "tb132fu"',
    'getprop("ro.product.device") == "TB132FU"',
)


def _replace_device_assertion(info):
  """Accept only the AOSP, Lenovo PRC, and Lenovo ROW recovery identities."""
  if not info.script.script:
    raise RuntimeError("TB132FU OTA is missing its generated device assertion")

  generated = info.script.script[-1]
  if not any(fragment in generated for fragment in _DEFAULT_ASSERTION_FRAGMENTS):
    raise RuntimeError(
        "Refusing to replace an unexpected TB132FU device assertion: " + generated)

  info.script.script[-1] = (
      '(getprop("ro.product.device") == "tb132fu" ||\n'
      ' getprop("ro.product.device") == "TB132FU" ||\n'
      ' getprop("ro.product.device") == "TB132FU_PRC" ||\n'
      ' getprop("ro.product.device") == "TB132FU_ROW") ||\n'
      ' abort("E3004: This package is for TB132FU devices; this is a \\"" + '
      'getprop("ro.product.device") + "\\\".");')


def FullOTA_Assertions(info):
  _replace_device_assertion(info)


def IncrementalOTA_Assertions(info):
  _replace_device_assertion(info)
