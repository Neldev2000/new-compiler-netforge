device:
    vendor   = "mikrotik"
    model    = "HAP ac2"
    hostname = "Test-nvivas-jparedes"

interfaces:

    bridge:
        description="This is a bridge for eth1 and eth2"
        admin_state="enabled"

        eth3:
            type = "ethernet"
            description= "Salida a Internet WAN"

ip:
    bridge:
        address = 10.100.100.1/23
    eth3:
        address = 192.168.1.1/30