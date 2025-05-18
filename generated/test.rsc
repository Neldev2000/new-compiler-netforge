    # Section: device (Type: device)
    /system identity set name="mikrotik_test"
    # Section: interfaces (Type: interfaces)
    /interface-: add set type=""ethernet"" description=""Test interface""
        # Section: ip (Type: custom)
        /ip set address="192.168.1.1/24"
