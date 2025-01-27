# How to Run Qtum-Qt.app with a Self-Signed Certificate on macOS

If you have downloaded `Qtum-Qt.app` and want to run it on macOS, you may encounter security restrictions. This guide provides specific instructions for using a self-signed certificate to allow `Qtum-Qt.app` to run.

## Steps

### 1. Create a Self-Signed Certificate (Skip if you already have a certificate for code signing)

1. Open **Keychain Access** (Applications > Utilities > Keychain Access).
2. In the **Keychain Access** menu, select **Certificate Assistant > Create a Certificate**.
3. Enter a name for your certificate (e.g., "Qtum Certificate").
4. Set **Identity Type** to **Self-Signed Root**.
5. Set **Certificate Type** to **Code Signing**.
6. Click **Create** to generate the certificate.

### 2. Sign the Qtum-Qt.app Application

1. Open **Terminal**.
2. Use the following `codesign` command to sign `Qtum-Qt.app` with the newly created certificate:
   
   ```
   codesign --force --deep --sign "Qtum Certificate" /path/to/Qtum-Qt.app
   ```
   
   Replace "Qtum Certificate" with the name of your certificate, and ensure /path/to/Qtum-Qt.app/ points to the actual location of the app on your system.
   Remove Qtum from MacOS Quarantine:
   `sudo xattr -rd com.apple.quarantine Qtum-Qt.app`
   
3. Open Qtum from the Applications menu
	

***Note:** The same procedure can be used to sign `qtumd`, `qtum-cli` and other command line binaries.
