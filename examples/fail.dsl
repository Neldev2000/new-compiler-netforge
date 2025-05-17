# Simple FAIL test file (it should be vendor = instead of vendor: )
device:
    vendor: "mikrotik"
    model = "test"

interfaces:
    ether1:
        type = "ethernet"
        description = "Test interface"
        ip:
            address = 192.168.1.1/24 