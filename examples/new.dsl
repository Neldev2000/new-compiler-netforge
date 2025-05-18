# Simple test file
device:
    vendor = "mikrotik"
    model = "test"

interfaces:
    # LAN interface
    ether1:
       type = "ethernet"
       admin_state = "enabled"

    # WAN interface
    ether2:
        type = "ethernet"
        admin_state = "enabled"
        description = "WAN Connection"
