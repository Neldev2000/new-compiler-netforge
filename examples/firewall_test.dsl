device:
    vendor = "mikrotik"
    model = "hAP ac2"
    hostname = "office-router"

interfaces:
    ether1:
        type = "ethernet"
        admin_state = "enabled"
        description = "WAN"
    ether2:
        type = "ethernet"
        admin_state = "enabled"
        description = "LAN"
    bridge1:
        type = "bridge"
        admin_state = "enabled"
        ports = [ether2]

firewall:
    # Filter rules
    filter:
        # Allow established connections
        input_accept_established:
            chain = "input"
            connection_state = ["established", "related"]
            action = "accept"
            comment = "Allow established connections"
        
        # Drop all other input traffic
        input_drop_all:
            chain = "input"
            action = "drop"
            comment = "Drop all other input traffic"
        
        # Allow specific protocol
        forward_allow_dns:
            chain = "forward"
            protocol = "udp"
            dst_port = "53"
            action = "accept"
            comment = "Allow DNS traffic"
        
        # Block specific website access
        forward_block_social:
            chain = "forward"
            dst_address = "192.0.2.1"
            action = "drop"
            comment = "Block access to social media"
    
    # NAT rules
    nat:
        # Source NAT for outgoing traffic
        srcnat_masquerade:
            chain = "srcnat"
            action = "masquerade"
            out_interface = "ether1"
            comment = "NAT for Internet access"
        
        # Destination NAT for incoming traffic (port forwarding)
        dstnat_web_server:
            chain = "dstnat"
            action = "dst-nat"
            protocol = "tcp"
            dst_port = "80"
            to_addresses = "10.0.0.10"
            to_ports = "80"
            comment = "Forward HTTP to web server"
    
    # Address lists for common uses
    address-list:
        blocked:
            203.0.113.50 = "Malicious IP"
            198.51.100.0/24 = "Known spam network"
        
        local-networks:
            10.0.0.0/8 = "RFC1918"
            172.16.0.0/12 = "RFC1918"
            192.168.0.0/16 = "RFC1918"
    
    # Raw table for advanced rules
    raw:
        block_invalid:
            chain = "prerouting"
            action = "drop"
            connection-state = "invalid"
            comment = "Drop invalid connections"
    
    # Service ports configuration
    service-port:
        ftp = "yes"
        h323 = "no"
        sip = "yes" 