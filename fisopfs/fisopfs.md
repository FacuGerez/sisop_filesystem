# fisop-fs

## Documentación de diseño

Como primera idea, quisimos hacer un Very Simple FileSystem (VSFS), ya que vimos la explicación clarísima en el Three
Easy Pieces. No nos terminó de convencer, principalmente por el hecho del `delete_time`. Si bien veiamos la
ventaja de no tener que hacer el swap final que hacemos en `filesystem_unlink()`, los órdenes de complejidad una vez
hallado un directorio seguían siendo iguales, siendo todos `O(n)` siendo `n` la cantidad de inodos que contiene el
directorio actual. También, la comprobación de si `delete_time` era o no nulo agregaba complejidad al código.
Por lo cual, ante la misma complejidad temporal, pero menor legibilidad, optamos por la practicidad.

La estructura de nuestro FileSystem es:

Lo primero que caracteriza a la estructura del FS es, valga la redundancia, una estructura llamada `filesystem`. Esta
estructura tiene un puntero a inodo `root`, el cual es de tipo directorio y se inicializa cuando se inicia
y monta nuestro FileSystem. Luego, la estructura general es:

- inodo directorio -> contiene un array de punteros a dentries y la longitud de este array
- inodo file -> contiene únicamente el contenido del file
- inodo -> es un directorio o un file
- dentry -> el nombre del directorio/archivo y su respectivo inodo

Al ir avanzando en el TP, nos dimos cuenta de que era necesario la metadata respectiva del inodo para poder mostrar su
información y darle permisos o no al usuario (escritura, lectura, entre otros).
Por lo tanto, la estructura inicial la modificamos, agregandole al inodo los siguientes datos:

inodo -> ahora, además de ser un file o un dir, también contendrá:
modo -> los permisos de este inodo
número de links -> la cantidad de elementos que apuntan a este inodo
uid -> a qué usuario le pertenece este inodo
gid -> a qué grupo le pertenece este inodo
último acceso al dir/file -> fecha y hora del último acceso al inodo
última modificacion -> fecha y hora de la última modificación al inodo
hora de creacion -> fecha y hora de la última creación al inodo
tamaño -> el tamaño en bytes/chars que ocupa el dir/file

Búsqueda de archivos según un path:

La búsqueda según el path se basa en separar del path según `/` e ir recorriendo a través de los dentries que
apuntan a sus propios inodos, para ir leyendo lo que serían sus nombres y encontrar los nombres solicitados dentro del
path. En caso de no matchear algún nombre en algún directorio, automáticamente detecta que la operación es inválida
porque no existe el archivo o el directorio y devuelve que no se lo encontró.

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

Como detalle que nos gustaría agregar para comentar es el update de modify_time. En los casos de que se realicen algunas
de las siguientes operaciones, además de updatear los tiempos correspondientes del propio file, también actualizamos el
modify time del directorio padre:

- create
- mkdir
- rmdir
- unlink

[Fuente1](https://stackoverflow.com/questions/61570808/what-operations-should-change-the-modification-date-of-a-directory#:~:text=It%20is%20apparent%20when%20you,directory%20will%20update%20its%20mtime.)

[Fuente2](https://serverfault.com/questions/388050/does-directory-mtime-always-change-when-a-new-file-is-created-inside)

## Tests

Aclaración importante: los tests pueden llegar a diferir según el idioma en el que se tenga el sistema operativo, pero
en caso de que suceda, de ver la salida esperada contra la salida obtenida va a ser notorio que es solo una cuestión
idiomática.

Los tests se pueden ejecutar con:

```
make tests
```

Y, si se desea una salida más completa, con:

```
make tests-verbose
```

En el repositorio están subidas las salidas por pantalla de cada test
específico, en `tests/output`.

Salida de la suite de pruebas con `make tests`:

```
bash tests/run.sh
test_01_create_directory PASSED
test_02_nested_create_directory PASSED
test_03_remove_dir PASSED
test_04_create_file PASSED
test_05_not_empty_dir_fails_rm PASSED
test_06_unlink_file PASSED
test_07_remove_files PASSED
test_08_out_redirection_creates_file PASSED
test_09_cat_and_out_redirection_works PASSED
test_10_out_redirection_overwrites PASSED
test_11_in_redirection_works PASSED
test_12_append_redirection_works PASSED
test_13_stat PASSED
```

Salida de la suite de pruebas con `make tests-verbose`:

```
bash tests/run.sh -v
[verbose] Mounting filesystem...
[verbose] Running test_01_create_directory
[verbose] Comparing tests/output/test_01_create_directory_out.txt to tests/expected/test_01_create_directory_expected.txt
test_01_create_directory PASSED
[verbose] 
[verbose] Cleaning test directory...
[verbose] Running test_02_nested_create_directory
[verbose] Comparing tests/output/test_02_nested_create_directory_out.txt to tests/expected/test_02_nested_create_directory_expected.txt
test_02_nested_create_directory PASSED
[verbose] 
[verbose] Cleaning test directory...
[verbose] Running test_03_remove_dir
[verbose] Comparing tests/output/test_03_remove_dir_out.txt to tests/expected/test_03_remove_dir_expected.txt
test_03_remove_dir PASSED
[verbose] 
[verbose] Cleaning test directory...
[verbose] Running test_04_create_file
[verbose] Comparing tests/output/test_04_create_file_out.txt to tests/expected/test_04_create_file_expected.txt
test_04_create_file PASSED
[verbose] 
[verbose] Cleaning test directory...
[verbose] Running test_05_not_empty_dir_fails_rm
[verbose] Comparing tests/output/test_05_not_empty_dir_fails_rm_out.txt to tests/expected/test_05_not_empty_dir_fails_rm_expected.txt
test_05_not_empty_dir_fails_rm PASSED
[verbose] 
[verbose] Cleaning test directory...
[verbose] Running test_06_unlink_file
[verbose] Comparing tests/output/test_06_unlink_file_out.txt to tests/expected/test_06_unlink_file_expected.txt
test_06_unlink_file PASSED
[verbose] 
[verbose] Cleaning test directory...
[verbose] Running test_07_remove_files
[verbose] Comparing tests/output/test_07_remove_files_out.txt to tests/expected/test_07_remove_files_expected.txt
test_07_remove_files PASSED
[verbose] 
[verbose] Cleaning test directory...
[verbose] Running test_08_out_redirection_creates_file
[verbose] Comparing tests/output/test_08_out_redirection_creates_file_out.txt to tests/expected/test_08_out_redirection_creates_file_expected.txt
test_08_out_redirection_creates_file PASSED
[verbose] 
[verbose] Cleaning test directory...
[verbose] Running test_09_cat_and_out_redirection_works
[verbose] Comparing tests/output/test_09_cat_and_out_redirection_works_out.txt to tests/expected/test_09_cat_and_out_redirection_works_expected.txt
test_09_cat_and_out_redirection_works PASSED
[verbose] 
[verbose] Cleaning test directory...
[verbose] Running test_10_out_redirection_overwrites
[verbose] Comparing tests/output/test_10_out_redirection_overwrites_out.txt to tests/expected/test_10_out_redirection_overwrites_expected.txt
test_10_out_redirection_overwrites PASSED
[verbose] 
[verbose] Cleaning test directory...
[verbose] Running test_11_in_redirection_works
[verbose] Comparing tests/output/test_11_in_redirection_works_out.txt to tests/expected/test_11_in_redirection_works_expected.txt
test_11_in_redirection_works PASSED
[verbose] 
[verbose] Cleaning test directory...
[verbose] Running test_12_append_redirection_works
[verbose] Comparing tests/output/test_12_append_redirection_works_out.txt to tests/expected/test_12_append_redirection_works_expected.txt
test_12_append_redirection_works PASSED
[verbose] 
[verbose] Cleaning test directory...
[verbose] Running test_13_stat
[verbose] Comparing tests/output/test_13_stat_out.txt to tests/expected/test_13_stat_expected.txt
test_13_stat PASSED
[verbose] 
[verbose] Cleaning test directory...
[verbose] Unmounting tests/mount...
[verbose] Killing FS process 10641
```

### Explicación breve de caso probado y salida esperada

#### Test 1

Prueba que al crear un directorio se cree correctamente.

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

Prueba que al querer borrar un directorio vacío se borre correctamente.

```
testdir
```

#### Test 4

Prueba que al crear un archivo se cree correctamente.

```
testfile
```

#### Test 5

Prueba que al querer borrar un directorio no vacío no se borre.

```
rmdir: failed to remove 'tests/mount/testdir': Directory not empty
testdir
testfile
```

#### Test 6

Prueba que al borrar un archivo efectivamente se borre.

```
testfile
```

#### Test 7

Prueba que al querer borrar varios archivos de un mismo directorio se borren.

```
testfile1
testfile2
```

#### Test 8

Prueba que la creación de un archivo nuevo con redirección utilizando el comando `echo` funcione.

```
testfile
```

#### Test 9

Prueba que al crear un archivo con `echo` el contenido sea el esperado.

```
hello world
```

#### Test 10

Prueba que al redirigir texto a un archivo ya creado se sobrescriba el contenido previo.

```
goodbye
```

#### Test 11

Prueba que la redirección de un archivo hacia un `cat` funcione correctamente.

```
hello world
```

#### Test 12

Prueba que la modificación de un archivo utilizando un `>>` (append) funcione correctamente.

```
first
second
```

#### Test 13

Prueba que al querer mirar las estadisticas de un archivo funcione correctamente.
NOTA: No se incluyen los valores, solo los campos esperados, porque cambian constantemente.

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
