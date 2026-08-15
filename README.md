# TrafficSim

TrafficSim es un motor de simulación de tráfico desarrollado con C++ moderno.

El proyecto modelará vehículos que circulan por redes de carreteras representadas
mediante grafos, respetando rutas, semáforos y condiciones de tráfico. El desarrollo
se realiza en sprints pequeños y verificables, implementando cada capacidad
únicamente cuando sea necesaria.

## Estado actual

Sprint 0: fundamentos del proyecto.

Actualmente implementado:

- Configuración de un proyecto C++20
- Compilación portable mediante CMake
- Presets para Debug y Release
- Advertencias estrictas del compilador
- Pruebas automatizadas con GoogleTest y CTest
- Formato de código mediante clang-format
- Análisis estático mediante clang-tidy
- Ejecutable básico de consola
- Documentación arquitectónica inicial

El motor de simulación todavía no está implementado.

## Requisitos

- CMake 3.25 o posterior
- Un compilador compatible con C++20:
  - MSVC
  - GCC
  - Clang
- LLVM para ejecutar clang-format y clang-tidy
- Git

GoogleTest se descarga automáticamente durante la configuración de pruebas.

## Configurar

Ejecuta desde la raíz del repositorio:

```powershell
cmake --preset debug
```

## Compilar

```powershell
cmake --build --preset debug
```

## Ejecutar

```powershell
.\out\build\debug\Debug\trafficsim.exe
```

Salida esperada:

```text
TrafficSim
```

## Ejecutar las pruebas

```powershell
ctest --preset debug
```

## Verificar el formato

```powershell
clang-format --dry-run --Werror `
    .\src\app\main.cpp `
    .\tests\unit\ProjectSetupTests.cpp
```

## Ejecutar el análisis estático

```powershell
clang-tidy .\src\app\main.cpp `
    --extra-arg-before=--driver-mode=cl `
    --extra-arg=/std:c++20 `
    --
```

## Compilar una versión Release

```powershell
cmake --preset release
cmake --build --preset release
```

El preset Release no descarga ni compila las pruebas.

## Estructura del proyecto

```text
traffic-sim/
├── docs/
│   └── architecture.md
├── src/
│   └── app/
│       └── main.cpp
├── tests/
│   └── unit/
│       └── ProjectSetupTests.cpp
├── .clang-format
├── .clang-tidy
├── .gitignore
├── CMakeLists.txt
├── CMakePresets.json
└── README.md
```

La estructura crecerá sprint por sprint. No se crearán capas arquitectónicas
vacías antes de que sean necesarias.

## Hoja de ruta

- Fundamentos del proyecto y pruebas automatizadas
- Modelo de red de carreteras
- Planificación de rutas con Dijkstra
- Movimiento de vehículos
- Simulación con paso de tiempo fijo
- Semáforos y seguimiento entre vehículos
- Estadísticas y exportación CSV
- Carga de escenarios
- Medición de congestión

La visualización queda fuera del MVP inicial y permanecerá separada del motor
de simulación.