# fisop-fs

## Documentación de diseño

Como primera idea, quisimos hacer un Very Simple FileSystem (VSFS), ya que vimos la explicación clarísima en el Three Easy Pieces. No nos terminó de convencer por el hecho del delete_time principalmente, porque si bien veiamos la ventaja de no tener que hacer el swap final que hacemos en `filesystem_unlink()`, los ordenes de complejidad una vez hallado un directorio seguían siendo iguales, siendo todos `O(n)` siendo `n` la cantidad de inodos que contiene el directorio actual. También la comprobación de si el delete_time era o no nulo, complejizaba más el código. Por lo cual, misma complejidad temporal y encimá menos legibilidad -> optamos por la practicidad.

La estructura de nuestro FileSystem es:

Lo primero que caracteriza a la estructura del FS es, valga la redundancia, una estructura llamada `filesystem` el cual tiene un puntero a inodo, que es el inodo `root`, el cual es de tipo directorio y el cual se inicializa cuando se inicia y monta nuestro FileSystem. Luego la estructura general es:

- inodo directorio -> contiene un array de punteros a dentries y la longitud de este array
- inodo file -> contiene únicamente el contenido del file
- inodo -> es un directorio o un file
- dentry -> el nombre del directorio/archivo y su respectivo inodo

Pero al ir avanzando en el tp nos dimos cuenta que era necesario la metadata respectiva del inodo para poder mostrar la información de esta y darle permisos o no al usuario tanto de escritura como de lectura, entre otros.
Por lo que luego a nuestra estructura inicial la fuimos modificando para agregarle al inodo los siguientes datos:

inodo -> ahora ademas de ser un file o un dir, tambien contenia:
            modo -> los permisos de este inodo
            numero de links -> la cantidad de elementos que apuntan a este inodo
            uid -> a que usuario le pertenece este inodo
            gid -> a que grupo le pertenece este inodo
            ultimo acceso al dir/file -> fecha y hora del último acceso al inodo
            ultima modificacion -> fecha y hora de la última modificación al inodo
            hora de creacion -> fecha y hora de la última creación al inodo
            tamaño -> el tamaño en bytes/chars que ocupa el dir/file

Búsqueda de archivos según un path:

La búsqueda según el path se basa en el spliteo del mismo path segun `/` e ir recorriendo a través de los dentries que apuntan a sus propios inodos, para ir leyendo lo que serían sus nombres y encontrar los nombres solicitados dentro del path. En caso de no matchear algún nombre en algún directorio, automáticamente detecta que la operación es inválida porque no existe el archivo o el directorio y devuelve que no se lo encontró.

Por ejemplo en el caso de: `/prueba1/prueba2/file`

El recorrido que haría el sistema, suponiendo que el path es correcto, sería:
- Obtengo el inodo root de mi filesystem
- Busco en el array de dentries del inodo root, el nombre `prueba1`
- Obtengo el inodo `prueba1`
- Como es de tipo directorio busco en el array de dentries el que tenga el nombre `prueba2`
- Obtengo el inodo `prueba2`
- Como es de tipo directorio, busco en el array de dentries el que tenga el nombre `file`
- Obtengo el inodo `file`
- Devuelvo el inodo encontrado

Como detalle que nos gustaría agregar para comentar es el update de modify_time. En los casos de que se realicen algunas de las siguientes operaciones, además de updatear los tiempos correspondientes del propio file, también actualizamos el modify time del directorio padre:

- create
- mkdir
- rmdir
- unlink

[Fuente1](https://stackoverflow.com/questions/61570808/what-operations-should-change-the-modification-date-of-a-directory#:~:text=It%20is%20apparent%20when%20you,directory%20will%20update%20its%20mtime.)

[Fuente2](https://serverfault.com/questions/388050/does-directory-mtime-always-change-when-a-new-file-is-created-inside)

## Tests

Aclaración importante: los tests pueden llegar a diferir según el idioma en el que se tenga el sistema operativo, pero en caso de que suceda, de ver la salida esperada contra la salida obtenida va ser notorio que es solo una cuestión idiomática.

### Explicación breve de caso probado y salida esperada

#### Test 1

Prueba que al crear un directorio se cree correctamente

```
testdir
```

#### Test 2

Prueba que al crear un directorio dentro de otro se cree correctamente.

```
testdir
nested_dir
```

#### Test 3

Prueba que al querer borrar un directorio vacío se borre correctamente

```
testdir
```

#### Test 4

Prueba que al crear un archivo se cree correctamente.

```
testfile
```

#### Test 5

Prueba que al querer borrar un directorio no vacío no se borre

```
rmdir: failed to remove 'tests/mount/testdir': Directory not empty
testdir
testfile
```

#### Test 6

Prueba que al borrar un archivo efectivamente se borre

```
testfile
```

#### Test 7

Prueba que al querer borrar varios archivos de un mismo directorio se borren

```
testfile1
testfile2
```

#### Test 8

Prueba que la creación de un archivo nuevo con redirección utilizando el comando echo funcione

```
testfile
```

#### Test 9

Prueba que al crear un archivo con echo el contenido sea el esperado

```
hello world
```

#### Test 10

Prueba que al redirigir texto a un archivo ya creado se pise el contenido previo

```
goodbye
```

#### Test 11

Prueba que la redirección de un archivo hacia un cat funcione correctamente

```
hello world
```

#### Test 12

Prueba que la modificación de un archivo utilizando un >> (append) funcione correctamente

```
first
second
```

#### Test 13

Prueba que al querer mirar las estadisticas de una archivo funcione correctamente

```
File
Size
Device
Access
Access
Modify
Change
Birth
```
