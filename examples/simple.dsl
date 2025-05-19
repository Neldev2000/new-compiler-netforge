# This is a sample Mikrotik configuration in our DSL language

device:
    vendor = "mikrotik"
    model = "CCR2004-1G-12S+2XS"
    hostname = "core-router"

# Interface configuration
interfaces:
    ether1:
        type = "ethernet"
        admin_state = "enabled"
        description = "WAN Connection"

    ether2:
        type = "ethernet"
        admin_state = "enabled"
        description = "LAN Connection"
   
    vlan100:
        type = "vlan"
        vlan_id = 100
        interface = "ether2"
        admin_state = "enabled"
        description = "Management VLAN"

# IP configuration
ip:
    vlan100:
        address = 10.100.0.1/24
    ether1:
        address = 103.10.20.2/30
    ether2:
        address = 10.0.0.1/24

# Routing configuration
routing:
    static_route_default_gw = 192.168.1.254
    static_route1:
        destination = 172.16.0.0/24
        gateway = 10.0.0.254

# Firewall configuration
firewall:
    filter:
        input_accept_established:
            chain = input
            connection_state = ["established", "related"]
            action = accept
            
        input_drop_all:
            chain = input
            action = drop
            
    nat:
        srcnat_masquerade:
            chain = srcnat
            action = masquerade
            out_interface = "ether1"

