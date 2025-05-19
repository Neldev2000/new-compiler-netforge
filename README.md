
# NetForge Compiler

Estructura del Proyecto
Todos los archivos fuente del compilador, incluyendo el Makefile, se encuentran en la carpeta src/. Esta estructura organiza de forma clara los diferentes módulos y componentes del proyecto.
`
.
├── src/
│   ├── scanner.flex
│   ├── parser.bison
│   ├── main.c
│   ├── Makefile
│   └── ... (otros archivos fuente)
└── README.md
`
Compilación
Para compilar el proyecto NetForge, primero navega a la carpeta src/ y luego ejecuta el siguiente comando:
`
cd src/
make all
`
Este comando se encargará de compilar todos los archivos fuente necesarios, incluyendo scanner.flex y parser.bison, y de generar el ejecutable del compilador.
Uso
Una vez compilado, el ejecutable del compilador se encontrará en la ruta `../bin/mikrotik_compiler (relativa a la carpeta src/).`
Puedes utilizar el compilador de la siguiente manera:
`
../bin/mikrotik_compiler [path_archivo_input] [path_archivo_output]
`

 * [path_archivo_input]: La ruta al archivo de código fuente que deseas compilar.
 * [path_archivo_output]: La ruta donde se guardará el archivo de salida generado por el compilador.
Ejemplo:
Si tienes un archivo de entrada llamado mi_programa.nf en la carpeta ejemplos/ y quieres que el resultado se guarde como salida.txt en la misma carpeta, y estás en el directorio src/:
`
../bin/mikrotik_compiler ../ejemplos/mi_programa.nf ../ejemplos/salida.txt
`
