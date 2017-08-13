
Debian
====================
This directory contains files used to package qtumd/qtum-qt
for Debian-based Linux systems. If you compile qtumd/qtum-qt yourself, there are some useful files here.

## qtum: URI support ##


qtum-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install qtum-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your qtum-qt binary to `/usr/bin`
and the `../../share/pixmaps/bitcoin128.png` to `/usr/share/pixmaps`

qtum-qt.protocol (KDE)

