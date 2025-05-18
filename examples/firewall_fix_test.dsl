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

firewall:
    # Testing both underscore and hyphen versions
    nat:
        # Using underscore version
        nat_rule1:
            chain = "srcnat"
            action = "masquerade"
            out_interface = "ether1"
            comment = "NAT using underscore syntax"
            
        # Using hyphen version
        nat_rule2:
            chain = "srcnat"
            action = "masquerade"
            out-interface = "ether1"
            comment = "NAT using hyphen syntax" 