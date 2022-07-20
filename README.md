# 3ds_dl
This library exposes a dl-like API for loading and executing ELF files on a 3DS.

## Limitations:
- `RTLD_LAZY`, `RTLD_DEEPBIND`, and `RTLD_NODELETE` are not supported;
- NULL pseudo path for main process is not supported;
- `dladdr()` never fills `dli_sname` or `dli_saddr`;
- Only a handful relocations are implemented.