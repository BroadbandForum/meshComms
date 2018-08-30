# What is this folder?

In this folder you will find auxiliary scripts to start/manage/stop/... the
IEEE1905 binaries in different 'targets'.

The folder structure is organized like this:

```
  .
  |-<platform_1>                 'platform' and 'flavour' take the same
  | |-<flavour_1>                values they receive in file 'Makefile'
  | |-<flavour_2>                on the project root folder.
  | ...                          
  | `-<flavour_n>                Examples of 'platform' values: 'linux', ...
  ...
  `-<platform_k>                 Examples of 'flavour' values: 'x86_generic',
    |-<flavour_1>                'arm_wrt1900acx', 'x86_windows_mingw', ...
    |-<flavour_2>
    ...
    `-<flavour_q>
```

...where each leaf directory contains self-documented scripts that in some cases
you will be able to use directly while in others you will first need to modify
them for your particular setup/use case.

> Note that not all supported platforms/flavours necessarily have an associated
> folder (maybe because no scripts have been developed yet or maybe because
> they are not needed at all).

For example, the 'linux/arm_wrt1900acx' folder contains scripts to start the AL
binary and manage its 'external triggers' using the OpenWRT's configuration
system available in that target.

