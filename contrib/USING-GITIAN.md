Using Gitian
====================
### Setting up GPG keys
Generate GPG key on your computer:
1. ```gpg --gen-key ```(choose RSA and RSA, size 4096 byte, name and email should be the same like on Github)
2. ```gpg --list-secret-keys --keyid-format LONG ```(We are interested in the first line after 4096R/)
3. ```gpg --armor --export 3AA5C34371567BD2```(Enter the hash from previous command)
Copy got gpg key into the /contrib/gitian-keys/ folder with .pgp format.
### Setting up Gitian
1. Replace .yml files in qtum-bitcore/contrib/gitian-descriptors folder. Replace gitian-build.sh in qtum/contrib folder. Add windeploy/ folder into the qtum-bitcore/contrib. Push these changes to remote repository https://github.com/qtumproject/qtum-bitcore/. Also very important, windeploy/ folder should be The same version as you want to build. You will couldn't build win binaries without this folder in version which you want to build.
2. gitian-build.sh script should be started from directory where qtum places(like in instruction).
##### First time / New Gitian builders
These actions are executed once when first using gitian-builder. If you have used gitian-builder for qtum skip these steps.
1. ```qtum-bitcore/contrib/gitian-build.sh --setup``` This command create and setup virtual machines to build your binaries files. This command may take a while (about 40 minutes). If you want to use KVM as build VM , run script with ```--kvm```.
    ```qtum-bitcore/contrib/gitian-build.sh --setup --kvm```

2. Create the OS X SDK tarball( https://github.com/qtumproject/qtum/blob/master/doc/README_osx.md), create inputs/ folder in gitian-builder/ . Copy MacOSX10.11.sdk.tar.gz into the inputs/ directory.
##### Not first time
Ensure that the ./gitian.sigs directory is up to date for signs verifying.

    pushd ./gitian.sigs
    git pull
    popd

Ensure that the ./gitian-builder directory is up to date.

    pushd ./gitian-builder
    git pull
    popd

### Build and sign Qtum for Linux, Windows, and OS X:

  ```qtum-bitcore/contrib/gitian-build.sh --build --signer signer version``` or 
  ```qtum-bitcore/contrib/gitian-build.sh --build --kvm --signer signer version```

signer — GPG Signer sign assert files for builds (name you entered with GPG key creation). When script is running you must specify passphrase. Use passphrase you entered with the GPG key creation. 

version - tag or branch of Github.

You can set false for a certain os in gitian-build.sh if you aren't interested in it building.
Script will be running about 1h or more.

When script is running you may check state of installation and build progress with:

    tail -f var/install.log
    tail -f var/build.log
    
Output will look something like:
    
    Initialized empty Git repository in /home/gitianuser/gitian-builder/inputs/qtum-bitcore/.git/
    remote: Counting objects: 57959, done.
    remote: Total 57959 (delta 0), reused 0 (delta 0), pack-reused 57958
    Receiving objects: 100% (57959/57959), 53.76 MiB | 484.00 KiB/s, done.
    Resolving deltas: 100% (41590/41590), done.
    From https://github.com/qtumproject/qtum-bitcore
    ... (new tags, new branch etc)
    --- Building for trusty amd64 ---
    Stopping target if it is up
    Making a new image copy
    stdin: is not a tty
    Starting target
    Checking if target is up
    Preparing build environment
    Updating apt-get repository (log in var/install.log)
    Installing additional packages (log in var/install.log)
    Grabbing package manifest
    stdin: is not a tty
    Creating build script (var/build-script)
    lxc-start: Connection refused - inotify event with no name (mask 32768)
    Running build script (log in var/build.log)


Binaries will be in qtum-binaries/ . Signatures will appear in gitian.sigs/ . Signatures will be committed and you should push it manually. Sigs haven't committed with ```--no-commit```.

Build output expected:

  1. source tarball (`qtum-${VERSION}.tar.gz`)
  2. linux 32-bit and 64-bit dist tarballs (`qtum-${VERSION}-linux[32|64].tar.gz`)
  3. windows 32-bit and 64-bit unsigned installers and dist zips (`qtum-${VERSION}-win[32|64]-setup-unsigned.exe`, `qtum-${VERSION}-win[32|64].zip`)
  4. OS X unsigned installer and dist tarball (`qtum-${VERSION}-osx-unsigned.dmg`, `qtum-${VERSION}-osx64.tar.gz`)
  5. Gitian signatures (in `gitian.sigs/${VERSION}-<linux|{win,osx}-unsigned>/(your Gitian key)/`)

### Verify other gitian builders signatures to your own. (Optional)

Add other gitian builders keys to your gpg keyring

    gpg --import bitcoin/contrib/gitian-keys/*.pgp
    gpg --refresh-keys

Verify the signatures

```qtum-bitcore/contrib/gitian-build.sh --verify version```