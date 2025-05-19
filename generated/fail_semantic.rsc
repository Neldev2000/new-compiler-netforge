# Device Configuration
/system identity set name="mikrotik_Test-nvivas-jparedes_HAP ac2"
# Interface Configuration
/interface set eth1 disabled=no comment="This is a bridge for eth1 and eth2"
    # IP Configuration: ip
/ip address add address=10.100.100.1/23 interface=bridge
