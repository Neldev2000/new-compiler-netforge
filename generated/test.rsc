# Device Configuration
/system identity set name="mikrotik_hAPac2"
# Interface Configuration
/interface set ethernet ether1 disabled=no
/interface set ethernet ether2 disabled=no comment="WAN Connection"
# IP Configuration: ip
/ip address add address=192.168.1.1/24 interface=ether2
/ip address add address=10.0.0.1/24 interface=ether1
