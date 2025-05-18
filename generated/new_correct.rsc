/system identity set name="mikrotik_test"

/interface set ethernet ether1 disabled=no
/interface set ethernet ether2 disabled=no comment="WAN CONNECTION"

/ip address add address=192.168.1.1/24 interface=ether2
/ip address add address=10.0.0.1/24 interface=ether1

/ip route add dst-address=0.0.0.0/0 gateway=192.168.1.254
/ip route add dst-address=172.16.0.0/24 gateway=10.0.0.254

# Configuración del Firewall

# firewall:
#    filter:

#        input_accept_established:
#            chain = input
#            connection_state = ["established", "related"]
#            action = accept
# Esta regla permite el tráfico de respuesta y el tráfico relacionado (como FTP activo)
# que entra al router y que forma parte de una conexión ya iniciada por el router o por la LAN.
/ip firewall filter add chain=input action=accept connection-state=established,related comment="Permitir conexiones establecidas/relacionadas en input"

#        input_drop_all:
#            chain = input
#            action = drop
# ¡IMPORTANTE! Esta regla debe ir DESPUÉS de cualquier regla que permita tráfico específico
# en la cadena 'input', incluyendo la regla de 'established,related' anterior.
# Bloquea todo el tráfico restante dirigido al router (chain=input) que no fue aceptado por reglas anteriores.
/ip firewall filter add chain=input action=drop comment="Denegar todo el tráfico restante en input"

#    nat:

#        srcnat_masquerade:
#            chain = srcnat
#            action = masquerade
#            out_interface = "ether1"
# Esta regla NAT (Network Address Translation) es crucial para permitir que los dispositivos de la LAN
# accedan a internet a través de la interfaz WAN (ether1).
# Cambia la dirección IP de origen de los paquetes salientes por la IP de la interfaz WAN del router.
/ip firewall nat add chain=srcnat action=masquerade out-interface=ether1 comment="NAT Masquerade para acceso a Internet"