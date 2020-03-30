# Fantom Ledger App

Fantom Ledger Nano S and/or Ledger Nano X hardware wallet and secure key storage application.

## Building the source

Please make sure you have the toolchain set and ready, 
see [Getting Started](https://ledger.readthedocs.io/en/latest/userspace/getting_started.html) documentation 
on Ledger Documentation Hub.

Before you try to load the app to your Ledger device, make sure you have the device updated
to the most recent firmware. If on Linux, you may need to setup `udev` rules for your device
to be available in the system.

Ledger provides automated udevsetup script. 
Enter the following command to automatically add the rules and reload udev:

```shell
wget -q -O - https://raw.githubusercontent.com/LedgerHQ/udev-rules/master/add_udev_rules.sh | sudo bash
```

Don't forget to reload udev rules in system's shell to load the changes.

```shell
udevadm control --reload
```

Following article can help you [fix your connection problems](https://support.ledger.com/hc/en-us/articles/115005165269-Fix-connection-issues).

### Load the Application

```shell
make load
```
