# Device Configuration
/system identity set name="mikrotik_CCR1036"
# Interface Configuration
/interface set ethernet ether1 disabled=no comment="ISP1"
/interface set ethernet ether2 disabled=no comment="ISP2"
/interface set ethernet ether3 disabled=no comment="LAN"
    # Routing Configuration: routing
/ip route add dst-address=0.0.0.0/0 gateway=192.168.1.1
/ip route add dst-address=172.16.0.0/24 gateway=10.0.0.254
/ip route add dst-address=10.10.0.0/24 gateway=192.168.1.1
/ip route add dst-address=8.8.8.8/32 gateway=192.168.2.1
