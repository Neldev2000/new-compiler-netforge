# Device Configuration
/system identity set name="mikrotik_hAPac2"
# Interface Configuration
/interface set ethernet ether1 disabled=no
/interface set ethernet ether2 disabled=no comment="WAN Connection"
    # IP Configuration: ip
/ip address add address=192.168.1.1/24 interface=ether2
/ip address add address=10.0.0.1/24 interface=ether1
    # Routing Configuration: routing
/ip route add dst-address=0.0.0.0/0 gateway=192.168.1.254
/ip route add dst-address=172.16.0.0/24 gateway=10.0.0.254
