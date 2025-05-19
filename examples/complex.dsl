# Complex Mikrotik Border Router Configuration
# Connecting two countries with redundant links

device:
    vendor = "MikroTik"
    model = "CCR1072-1G-8S+"
    hostname = "BorderRouter-CountryA-CountryB"

# Interface configuration
interfaces:
    # WAN interfaces - connections to ISPs
    wan1:
        type = "ethernet"
        description = "Primary ISP Connection - Country A"
        admin_state = "enabled"
        speed = 10000
        duplex = "full"
        mtu = 1500
    
    wan2:
        type = "ethernet"
        description = "Secondary ISP Connection - Country A"
        admin_state = "enabled"
        speed = 10000
        duplex = "full"
        mtu = 1500
    
    wan3:
        type = "ethernet"
        description = "Primary ISP Connection - Country B"
        admin_state = "enabled"
        speed = 10000
        duplex = "full"
        mtu = 1500
        
    wan4:
        type = "ethernet"
        description = "Secondary ISP Connection - Country B"
        admin_state = "enabled"
        speed = 10000
        duplex = "full"
        mtu = 1500
    
    # LAN interfaces
    lan1:
        type = "ethernet"
        description = "Internal Network - Country A"
        admin_state = "enabled"
        speed = 10000
        duplex = "full"
        mtu = 1500
    
    lan2:
        type = "ethernet"
        description = "Internal Network - Country B"
        admin_state = "enabled"
        speed = 10000
        duplex = "full"
        mtu = 1500
    
    # Create a bonding interface for redundancy in Country A
    bond0:
        type = "bonding"
        description = "Bonding for WAN redundancy - Country A"
        admin_state = "enabled"
        mode = "802.3ad"
        slaves = ["wan1", "wan2"]
    
    # Create a bonding interface for redundancy in Country B
    bond1:
        type = "bonding"
        description = "Bonding for WAN redundancy - Country B"
        admin_state = "enabled"
        mode = "802.3ad"
        slaves = ["wan3", "wan4"]
    
    # VLAN for management
    vlan100:
        type = "vlan"
        vlan_id = 100
        interface = "lan1"
        description = "Management VLAN"
        admin_state = "enabled"
    
    # VLAN for inter-country secure traffic
    vlan200:
        type = "vlan"
        vlan_id = 200
        interface = "lan1"
        description = "Secure Inter-Country Traffic"
        admin_state = "enabled"

# IP addressing configuration
ip:
    # IP configuration for interfaces
    bond0:
        address = "103.10.20.2/30"
    
    bond1:
        address = "185.45.67.2/30"
    
    lan1:
        address = "10.1.0.1/16"
    
    lan2:
        address = "10.2.0.1/16"
    
    vlan100:
        address = "172.16.100.1/24"
    
    vlan200:
        address = "172.16.200.1/24"
    
    # Default gateway configuration
    static_route_default_gw = "103.10.20.1"

# Routing configuration
routing:
    # Static routes for Country A networks
    static_route1:
        destination = "192.168.0.0/16"
        gateway = "10.1.0.2"
    
    # Static routes for Country B networks
    static_route2:
        destination = "172.20.0.0/16"
        gateway = "10.2.0.2"
    
    # Backup route to Country B via Country A ISP
    static_route3:
        destination = "172.20.0.0/16"
        gateway = "103.10.20.1"
        distance = 10
    
    # Policy-based routing rules
    rule1:
        src_address = "10.1.0.0/16"
        gateway = "103.10.20.1"
        destination = 173.2.0.0/16
    
    rule2:
        src_address = "10.2.0.0/16"
        gateway = "185.45.67.1"
        destination = 174.2.0.0/16

# Firewall configuration
firewall:
    # Define filter rules
    filter:
        # Allow established connections
        allow_established:
            chain = "input"
            connection_state = ["established", "related"]
            action = "accept"
        
        # Allow management access only from vlan100
        allow_management:
            chain = "input"
            src_address = "172.16.100.0/24"
            dst_port = 22
            protocol = "tcp"
            action = "accept"
        
        # Drop invalid connections
        drop_invalid:
            chain = "input"
            connection_state = "invalid"
            action = "drop"
        
        # Default drop policy for input
        drop_input:
            chain = "input"
            action = "drop"
        
        # Allow traffic between Country A and Country B
        allow_inter_country:
            chain = "forward"
            in_interface = "lan1"
            out_interface = "lan2"
            action = "accept"
        
        # Block malicious IP ranges
        block_malicious:
            chain = "forward"
            src_address = "192.168.99.0/24"
            action = "drop"
    
    # Define NAT rules
    nat:
        # Masquerade Country A traffic
        masq_country_a:
            chain = "srcnat"
            src_address = "10.1.0.0/16"
            out_interface = "bond0"
            action = "masquerade"
        
        # Masquerade Country B traffic
        masq_country_b:
            chain = "srcnat"
            src_address = "10.2.0.0/16"
            out_interface = "bond1"
            action = "masquerade"
        
        # Port forwarding for a specific service
        service_forward:
            chain = "dstnat"
            dst_address = "103.10.20.2"
            dst_port = 80
            protocol = "tcp"
            action = "dst-nat"
            to_addresses = "10.1.0.100"
            to_ports = 8080
