# TP1-SO
Trabajo Practico 1 de Sistemas Operativos.
Solucion implementada por el Grupo 3:
- Brandy, Tobias
- Pannunzio, Faustino
- Sagues, Ignacio

__Objetivo:__ Implementar un programa que permita resolver multiples problemas de SATsolving utilizando minisat y distribuyendo las tareas en multiples hijos para tener un mejor desempeño.
Los resultados de este proceso son impresos en pantalla por un segundo proceso "view" y escrito al archivo "output.txt"

## Compilacion 

Para compilar el archivo se utiliza el comando:
```
make all
```
## Ejecucion

Para ejecutar el programa que se encarga de resolver los archivos es necesario suministrarle los archivos a resolver como parametros. 
```
./master.out SATfiles/*
```
El programa enviara por salida estandar la cantidad de tareas a resolver, este valor es utilizado por el programa "view". Si se desea correr el mismo se puede, llamar al programa master y pipear su salida al programa view.
```
./master SATfiles/* | ./view.out
```
La otra opcion consiste en, una vez que se haya empezado a correr master, llamar al programa view desde otra terminal y proveerle como parametro el numero enviado por master a salida estandar
```
Terminal 1:
./master SATfiles/* 
48

Terminal 2:
./view.out 48
```

## Testeo

Existe una opcion de testeo implementada en el Makefile. Esta ejecuta PVS-Studio, cppcheck en todos los archivos .c y valgrind en ambos programas utilizando los archivos apuntados por la carpeta TF.
```
 make test
``` 
Los resultados estaran:
- PVS-Studio:   report.tasks
- Cppcheck:     master.cppout childCode.cppout view.cppout
- Valgrind:     master.valout view.valout
  
Estos podran ser eliminados con el comando 
```
make clean_test
```

## Aclaraciones

Cppcheck indica que hay dos variables que su scope puede ser reducido. Consideramos que por claridad del codigo es preferible dejarlas de esta forma.
