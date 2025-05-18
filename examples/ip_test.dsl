device:
    vendor = "mikrotik"
    model = "hAP ac2"
    hostname = "test-router"

interfaces:
    ether1:
        type = "ethernet"
        admin_state = "enabled"
        description = "WAN Port"
    ether2:
        type = "ethernet"
        admin_state = "enabled"
        description = "LAN Port"
    bridge1:
        type = "bridge"
        admin_state = "enabled"
        ports = [ether2]

ip:
    # Interface IP addresses
    ether1:
        address = 192.168.88.2/24
    bridge1:
        address = 10.0.0.1/24
    
    # Routes configuration
    route:
        default = "192.168.88.1"
        10.1.0.0/24:
            gateway = "10.0.0.10"
            distance = 1
    
    # Firewall configuration
    firewall:
        filter:
            forward:
                action = "drop"
                protocol = "tcp"
                dst-port = 80
        nat:
            srcnat:
                action = "masquerade"
                out-interface = "ether1"
    
    # DHCP server configuration
    dhcp-server:
        pool1:
            interface = "bridge1"
            address-pool = "dhcp_pool"
            lease-time = "10m"
    
    # DHCP client configuration
    dhcp-client:
        ether1 = true
    
    # DNS configuration
    dns:
        servers = "8.8.8.8,8.8.4.4"
        allow-remote-requests = "yes"
    
    # ARP entries
    arp:
        192.168.88.10:
            mac-address = "00:11:22:33:44:55"
            interface = "ether1"
        10.0.0.10:
            mac-address = "AA:BB:CC:DD:EE:FF"
            interface = "bridge1" 