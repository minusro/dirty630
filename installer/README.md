# Installing DIRTY630

Builds come from CI: open the
[Actions](https://github.com/minusro/dirty630/actions) tab, pick the newest
green run, and download the artifact for your platform. Each one is named
with its version, e.g. `dirty630-0.6.0-macos-installer`. The `-installer`
artifacts put the plugin in the right folder for you; the `-vst3` ones are
the bare bundle if you would rather place it yourself.

**The builds are not code-signed.** Both operating systems will warn you
about that, and both let you proceed. If you would rather not click through
a security warning, build from source instead — see `docs/BUILD.md` — as a
locally built plugin raises no warnings.

## macOS

Run `dirty630-<version>-macos.pkg`. Gatekeeper refuses it on first
double-click, because it is unsigned and un-notarised:

1. Double-click the pkg. macOS says it "could not verify that this
   software is free from malware". Click **Done**.
2. Open **System Settings → Privacy & Security**, scroll to the Security
   section, and click **Open Anyway** next to the message about the pkg.
3. Confirm, then let the installer run. It needs an admin password, because
   it writes to `/Library/Audio/Plug-Ins/VST3`.

To install by hand instead, unzip the `-vst3` artifact, strip the download
flag, and drop the bundle in place:

```sh
xattr -dr com.apple.quarantine DIRTY630.vst3
cp -R DIRTY630.vst3 ~/Library/Audio/Plug-Ins/VST3/
```

The `xattr` line matters: a quarantined bundle is silently skipped by some
hosts, which looks like the plugin simply not appearing.

## Windows

Run `dirty630-<version>-windows.exe`. SmartScreen shows a blue
"Windows protected your PC" panel because the installer has no signature
and no reputation yet:

1. Click **More info**.
2. Click **Run anyway**.
3. Accept the UAC prompt — the installer writes to
   `C:\Program Files\Common Files\VST3`, which needs administrator rights.

To install by hand, copy the `DIRTY630.vst3` folder from the `-vst3`
artifact into `C:\Program Files\Common Files\VST3\`.

## After installing

Rescan plugins in your DAW. On macOS the AU is only built locally, not
shipped in the installer; if you build from source it lands in
`~/Library/Audio/Plug-Ins/Components` and Logic needs
`killall -9 AudioComponentRegistrar` plus a restart to notice a rebuild.

## Uninstalling

Delete the bundle the installer placed:

- macOS: `/Library/Audio/Plug-Ins/VST3/DIRTY630.vst3`
- Windows: `C:\Program Files\Common Files\VST3\DIRTY630.vst3`

Both need administrator rights. Builds before 0.5.2 installed a lower-case
`dirty630.vst3`; on macOS that sits alongside the current bundle rather than
being replaced, so delete it too or the host lists the plugin twice.
