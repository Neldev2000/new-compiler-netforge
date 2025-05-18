device:
    vendor = "mikrotik"
    model="hAPac2"
    hostname = "Nelson-Johan"


interfaces:
    # LAN interface
    ether1:
       type = "ethernet"
       admin_state = "enabled"
    ether2:
        admin_state = "enabled"
        type = "ethernet"
        
        description = "WAN Connection"

ip:
    ether2:
        address = 192.168.1.1/24
    ether1:
        address = 10.0.0.1/24
