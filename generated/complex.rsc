# Device Configuration
/system identity set name="MikroTik_BorderRouter-CountryA-CountryB_CCR1072-1G-8S+"
# Interface Configuration
/interface set ethernet wan1 mtu=1500 disabled=no comment="Primary ISP Connection - Country A"
/interface set ethernet wan2 mtu=1500 disabled=no comment="Secondary ISP Connection - Country A"
/interface set ethernet wan3 mtu=1500 disabled=no comment="Primary ISP Connection - Country B"
/interface set ethernet wan4 mtu=1500 disabled=no comment="Secondary ISP Connection - Country B"
/interface set ethernet lan1 mtu=1500 disabled=no comment="Internal Network - Country A"
/interface set ethernet lan2 mtu=1500 disabled=no comment="Internal Network - Country B"
/interface bonding add name=bond0 disabled=no comment="Bonding for WAN redundancy - Country A" mode=802.3ad slaves=
/interface bonding add name=bond1 disabled=no comment="Bonding for WAN redundancy - Country B" mode=802.3ad slaves=
/interface vlan add name=vlan100 vlan-id=100 interface=lan1 disabled=no comment="Management VLAN"
/interface vlan add name=vlan200 vlan-id=200 interface=lan1 disabled=no comment="Secure Inter-Country Traffic"
    # IP Configuration: ip
/ip address add address=103.10.20.2/30 interface=bond0
/ip address add address=185.45.67.2/30 interface=bond1
/ip address add address=10.1.0.1/16 interface=lan1
/ip address add address=10.2.0.1/16 interface=lan2
/ip address add address=172.16.100.1/24 interface=vlan100
/ip address add address=172.16.200.1/24 interface=vlan200
    # Routing Configuration: routing
/ip route add dst-address=192.168.0.0/16 gateway=10.1.0.2
/ip route add dst-address=172.20.0.0/16 gateway=10.2.0.2
/ip route add dst-address=172.20.0.0/16 gateway=103.10.20.1 distance=10
/ip route add dst-address=173.2.0.0/16 gateway=103.10.20.1
/ip route add dst-address=174.2.0.0/16 gateway=185.45.67.1
    # Firewall Configuration: firewall
/ip firewall filter add chain=input action=accept connection-state={"established","related"} comment="allow_established"
/ip firewall filter add chain=input action=accept protocol=tcp src-address=172.16.100.0/24 dst-port=22 comment="allow_management"
/ip firewall filter add chain=input action=drop connection-state=invalid comment="drop_invalid"
/ip firewall filter add chain=input action=drop comment="drop_input"
/ip firewall filter add chain=forward action=accept in-interface=lan1 out-interface=lan2 comment="allow_inter_country"
/ip firewall filter add chain=forward action=drop src-address=192.168.99.0/24 comment="block_malicious"
/ip firewall nat add chain=srcnat action=masquerade src-address=10.1.0.0/16 out-interface=bond0 comment="masq_country_a"
/ip firewall nat add chain=srcnat action=masquerade src-address=10.2.0.0/16 out-interface=bond1 comment="masq_country_b"
/ip firewall nat add chain=dstnat action=dst-nat protocol=tcp dst-address=103.10.20.2 dst-port=80 to-addresses=10.1.0.100 to-ports=8080 comment="service_forward"
