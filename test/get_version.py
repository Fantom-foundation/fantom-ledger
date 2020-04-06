#!/usr/bin/env python
#
# This will test Get Address instruction on Fantom Ledger App
#
from ledgerblue.comm import getDongle
import binascii

# inform what we do
print("~~ Fantom None Ledger Test ~~")
print("Requesting application version: INS 0x01")

# Create APDU message.
# CLA 0xE0
# INS 0x01  GET VERSION
# P1 0x00   NO DATA
# P2 0x00   NO DATA
# No confirmation
apduMessage = "E001000000"
apdu = bytearray.fromhex(apduMessage)

# do the request
dongle = getDongle(True)
result = dongle.exchange(apdu)

# print the result
print("Version Received: v" +
      binascii.hexlify(result[0:1]).decode() + "." +
      binascii.hexlify(result[1:2]).decode() + "." +
      binascii.hexlify(result[2:3]).decode())
