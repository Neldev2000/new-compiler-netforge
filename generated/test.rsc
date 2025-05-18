    # Device Configuration
    /system identity set name="mikrotik_test"
    # Interface Configuration: interfaces
    /interface
      # Custom Configuration: :
type="ethernet"comment="Test interface"      # Custom Configuration: ip
address="192.168.1.1/24"