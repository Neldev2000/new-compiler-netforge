# Device Configuration
/system identity set name="mikrotik_core-router_CCR2004-1G-12S+2XS"
# Interface Configuration
/interface set ethernet ether1 disabled=no comment="WAN Connection"
/interface set ethernet ether2 disabled=no comment="LAN Connection"
/interface vlan add name=vlan100 vlan-id=100 interface=ether2 disabled=no comment="Management VLAN"
    # IP Configuration: ip
/ip address add address=10.100.0.1/24 interface=vlan100
/ip address add address=103.10.20.2/30 interface=ether1
/ip address add address=10.0.0.1/24 interface=ether2
    # Routing Configuration: routing
/ip route add dst-address=0.0.0.0/0 gateway=192.168.1.254
/ip route add dst-address=172.16.0.0/24 gateway=10.0.0.254
    # Firewall Configuration: firewall
/ip firewall filter add chain=input action=accept connection-state={"established","related"} comment="input_accept_established"
/ip firewall filter add chain=input action=drop comment="input_drop_all"
/ip firewall nat add chain=srcnat action=masquerade out-interface=ether1 comment="srcnat_masquerade"
