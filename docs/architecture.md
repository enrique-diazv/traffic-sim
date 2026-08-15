# Arquitectura de TrafficSim

## Estado del documento

Este documento describe las decisiones arquitectónicas iniciales de TrafficSim.
La implementación se encuentra actualmente en el Sprint 0.

Las secciones sobre componentes futuros expresan la dirección prevista del
diseño; no indican que esos componentes ya estén implementados.

## Objetivo arquitectónico

TrafficSim será un motor de simulación de tráfico determinista desarrollado en
C++20. El motor modelará una red de carreteras como un grafo y coordinará
vehículos, rutas, intersecciones, semáforos y estadísticas.

La prioridad del diseño es:

1. Corrección
2. Comprensibilidad
3. Facilidad de prueba
4. Mantenibilidad
5. Determinismo
6. Rendimiento medido

## Arquitectura actual

En el Sprint 0 existe un único ejecutable de consola:

```text
src/app/main.cpp
        │
        ▼
   trafficsim.exe
```

Su propósito actual es verificar la cadena de compilación, CMake y la
infraestructura de pruebas.

## Dirección prevista

```text
Aplicación de consola
        │
        ▼
Motor de simulación
        │
        ├── Red de carreteras
        ├── Planificación de rutas
        ├── Vehículos
        ├── Control de tráfico
        ├── Eventos
        └── Estadísticas
```

La estructura física del repositorio crecerá conforme se implemente cada
sprint. No se crearán directorios o abstracciones vacías para componentes que
todavía no existen.

## Reglas de dependencias

- La aplicación de consola podrá depender del motor de simulación.
- El motor no dependerá de la aplicación de consola.
- La planificación de rutas consultará la red mediante su interfaz pública.
- Los vehículos no serán propietarios de carreteras o intersecciones.
- Una futura visualización solo leerá el estado público del motor.
- El motor nunca dependerá de una biblioteca gráfica.

## Decisiones iniciales

### C++20 y CMake

C++20 permite usar características modernas del lenguaje y la biblioteca
estándar. CMake mantiene el proyecto portable entre MSVC, GCC y Clang.

### Red basada en grafos

Las intersecciones se modelarán como nodos y las carreteras dirigidas como
aristas. Esta representación permite aplicar algoritmos conocidos de rutas,
comenzando con Dijkstra.

### Identificadores estables

Las entidades utilizarán identificadores numéricos estables en lugar de
relaciones de propiedad mediante punteros sin procesar. Esto facilita las
pruebas, el diagnóstico y una futura serialización.

### Paso de tiempo fijo

La simulación avanzará con un intervalo fijo, inicialmente de 0.1 segundos.
Esto permitirá repetir pruebas, reproducir errores y evitar que el resultado
dependa de la velocidad real de la computadora.

### Ejecución determinista

Toda aleatoriedad futura se concentrará en una abstracción que reciba una
semilla explícita. La misma configuración, escenario y semilla deberán producir
resultados equivalentes.

### Procesamiento en un solo hilo

El MVP será de un solo hilo. La concurrencia solo se evaluará después de medir
el rendimiento y encontrar un cuello de botella que la justifique.

### Visualización separada

La visualización no forma parte del MVP. Si se agrega posteriormente, será una
capa opcional que consumirá estado de solo lectura del motor.

## Gestión de memoria

Se preferirá la semántica por valor y la asignación automática. Cuando exista
propiedad dinámica exclusiva se utilizarán `std::unique_ptr` y
`std::make_unique`.

No se utilizarán `new` o `delete` directamente en el código de aplicación.

## Estrategia de pruebas

Las pruebas usan GoogleTest y se ejecutan mediante CTest.

Cada sprint incorporará:

- Pruebas unitarias para el nuevo comportamiento
- Pruebas de validación y casos límite
- Pruebas de integración cuando colaboren varios componentes
- Pruebas de determinismo cuando exista el motor de simulación

## Rendimiento

No se optimizará antes de medir. El diseño evitará búsquedas globales
innecesarias y favorecerá estructuras locales por carretera cuando se
implemente el movimiento de vehículos.