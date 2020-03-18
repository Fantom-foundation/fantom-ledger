# Fantom LEdger App

Fantom Ledger Nano S hardware wallet and secure key storage application.

## Building the source

Please make sure you have the toolchain set and ready, 
see [Getting Started](https://ledger.readthedocs.io/en/latest/userspace/getting_started.html) documentation 
on Ledger Documentation Hub.

Before you try to load the app to your Ledger device, make sure you have the device updated
to the most recent firmware. If on Linux, you may need to setup `udev` rules for your device
to be available in the system.

On Ubuntu, create a file under `/etc/udev/rules.d` called `01-ledger.rules` and paste this content inside:

```
SUBSYSTEMS=="usb", ATTRS{idVendor}=="2c97", ATTRS{idProduct}=="0000", MODE="0660", TAG+="uaccess", TAG+="udev-acl" OWNER="__user__"
SUBSYSTEMS=="usb", ATTRS{idVendor}=="2c97", ATTRS{idProduct}=="0001", MODE="0660", TAG+="uaccess", TAG+="udev-acl" OWNER="__user__"
```

Replace the __user__ with your system's user name. 
Run `udevadm control --reload` in system's shell to load the changes.

### Load the Application

```shell
make load
```
