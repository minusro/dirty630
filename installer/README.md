# Installing DIRTY630

One installer per platform. Neither is code-signed, so each operating system
blocks it the first time and has to be told to go ahead — the steps below
include doing that. Both write to the system-wide VST3 folder, so both ask
for an administrator password.

## macOS — `dirty630-<version>-macos.pkg`

1. Double-click the pkg. macOS refuses it, saying it "could not verify that
   this software is free from malware". Click **Done**.
2. Open **System Settings → Privacy & Security** and scroll to the Security
   section. Click **Open Anyway** on the message naming the pkg.
3. Confirm, then enter your password when the installer asks.

Installs `DIRTY630.vst3` into `/Library/Audio/Plug-Ins/VST3`.

## Windows — `dirty630-<version>-windows.exe`

1. Run the exe. SmartScreen shows a blue **Windows protected your PC**
   panel. Click **More info**.
2. Click **Run anyway**.
3. Accept the User Account Control prompt.

Installs `DIRTY630.vst3` into `C:\Program Files\Common Files\VST3`.

---

Then rescan plugins in your DAW. To remove the plugin later, delete the
`DIRTY630.vst3` folder from the path above.
