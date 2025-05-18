device:
    vendor = "mikrotik"
    model = "CCR1036"
    hostname = "core-router"

interfaces:
    ether1:
        type = "ethernet"
        admin_state = "enabled"
        description = "ISP1"
    ether2:
        type = "ethernet"
        admin_state = "enabled"
        description = "ISP2"
    ether3:
        type = "ethernet"
        admin_state = "enabled"
        description = "LAN"

routing:
    # Default route through ISP1 with failover
    static_route_default_gw = "192.168.1.1"
    
    # Regular static route
    static_route1:
        destination = "172.16.0.0/24"
        gateway = "10.0.0.254"
        distance = "1"
    
    # Route with check-gateway for automatic failover
    static_route2:
        destination = "10.10.0.0/24"
        gateway = "192.168.1.1"
        check-gateway = "ping"
        distance = "1"
    
    # Route to a specific custom routing table
    static_route3:
        destination = "8.8.8.8/32"
        gateway = "192.168.2.1"
        routing-table = "ISP2"
    
    # Custom routing tables definition
    tables:
        ISP1:
            fib = "yes"
        ISP2:
            fib = "yes"
    
    # Routing rules (policy-based routing)
    rules:
        rule1:
            src-address = "192.168.10.0/24"
            action = "lookup-only-in-table"
            table = "ISP2"
        rule2:
            dst-address = "8.8.8.8/32"
            action = "lookup-only-in-table"
            table = "ISP2"
    
    # Routing filters for dynamic routing
    filter:
        ospf_in:
            rule = "if (dst==0.0.0.0/0 && protocol static) { accept }" 