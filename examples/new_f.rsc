    # Section: device (Type: device)
    /system identity set vendor=""mikrotik"" ==""test""
    # Section: interfaces (Type: interfaces)
    # Sub-section: : (Full path: /interface-:)
    /interface-: set type=""ethernet"" description=""Test interface""
        # Section: ip (Type: custom)
        /ip set address="192.168.1.1/24"
