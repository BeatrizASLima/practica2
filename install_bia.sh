#!/bin/bash

if [ ! -d "build" ]; then
    mkdir build
    echo "build ja criado."
else
    echo "build ja existe."
fi

cd build || exit

cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ..
if [ $? -ne 0 ]; then
    echo "erro: ao executar cmake com exportação de comandos de compilação"
    exit 1
fi

cmake ..
if [ $? -ne 0 ]; then
    echo "erro ao executar cmake"
    exit 1
fi

make -j 12
if [ $? -ne 0 ]; then
    echo "erro ao executar make"
    exit 1
fi

cd ..

if [ -d "ply" ] && [ -d "mapas" ]; then
    cp -r ply mapas build/
    echo "copy de ply e mapas para o direct build."
else
    echo "erro: pastas ply e/ou mapas não encontradas no diretório raiz."
    exit 1
fi

if [ -f "include/modelos/jugador/linea.ply" ]; then
    mkdir -p build/include/modelos/jugador

    cp include/modelos/jugador/linea.ply build/include/modelos/jugador/
else
    echo "erro: arquivo include/modelos/jugador/linea.ply nao encontrado."
    exit 1
fi