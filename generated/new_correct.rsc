# Traducción del "Simple test file" a comandos de RouterOS

# device:
#    vendor = "mikrotik"
#    model = "test"
# RouterOS no tiene comandos para establecer "vendor" o "model" directamente,
# ya que son atributos del hardware. Lo más cercano es configurar la identidad del sistema.
/system identity set name="Mikrotik_Test_Device"


# interfaces:
#    ether1:
#        type = "ethernet"
# El tipo "ethernet" es inherente a la interfaz ether1, no se configura explícitamente.

#        description = "Test interface"
/interface ethernet set ether1 comment="Test interface"

#        ip:
#            address = 192.168.1.1/24
/ip address add address=192.168.1.1/24 interface=ether1