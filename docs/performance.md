# Benchmarks de rendimiento de TrafficSim

## Propósito

Estos benchmarks miden la escalabilidad de la búsqueda de carreteras, el cálculo
de rutas, la actualización de vehículos, la recopilación de estadísticas y las
simulaciones completas.

Los benchmarks se ejecutan manualmente mediante GitHub Actions para evitar las
restricciones locales de control de aplicaciones y proporcionar un entorno
registrado y reproducible.

## Metodología

La suite de benchmarks utiliza:

- Compilaciones Release con C++20.
- Cuatro cantidades de vehículos: 100, 1,000, 5,000 y 10,000.
- Tres repeticiones por cada benchmark y cantidad de vehículos.
- Promedios del tiempo transcurrido y del tiempo de CPU del proceso.
- Memoria residente máxima del proceso completo.
- Un runner dedicado de GitHub para cada ejecución.

Los escenarios individuales de los benchmarks son:

| Benchmark | Trabajo realizado |
|---|---|
| Búsqueda de carreteras | 100 búsquedas de carretera por vehículo |
| Enrutamiento | Un cálculo de ruta con Dijkstra por vehículo sobre una red lineal de 100 carreteras |
| Actualización de vehículos | Diez pasos de simulación de 0.1 segundos |
| Estadísticas | Una observación de estadísticas de carretera por vehículo |
| Simulación completa | Cinco segundos simulados con un paso de tiempo de 0.1 segundos |
| Lote secuencial | Ocho simulaciones completas independientes ejecutadas en serie |
| Lote paralelo | Las mismas ocho simulaciones ejecutadas con cuatro trabajadores |

La creación de los datos de prueba se excluye de los tiempos de los componentes
individuales. El benchmark de simulación completa incluye la generación,
el enrutamiento y la actualización de vehículos, además del monitoreo del
tráfico y la recopilación de estadísticas.

## Entorno de la ejecución optimizada

Las mediciones optimizadas fueron producidas por el commit
`3619e95e098be7d8315ce8e166ddbb236ab51a50`.

- Runner: `ubuntu-24.04` hospedado por GitHub
- CPU: AMD EPYC 9V74, 4 procesadores lógicos
- Memoria: 15 GiB
- Compilador: GCC 13.3.0
- CMake: 3.31.6
- Ninja: 1.13.2
- Tipo de compilación: Release
- Validación: 199 pruebas aprobadas
- Duración de las pruebas: 0.78 segundos

## Resultados optimizados

Todos los valores son promedios de tres repeticiones.

| Vehículos | Búsqueda de carreteras (ms) | Enrutamiento (ms) | Actualización de vehículos (ms) | Estadísticas (ms) |
|---:|---:|---:|---:|---:|
| 100 | 0.07 | 1.05 | 0.11 | 0.01 |
| 1,000 | 0.57 | 10.08 | 1.22 | 0.09 |
| 5,000 | 2.56 | 49.92 | 7.93 | 0.41 |
| 10,000 | 5.27 | 99.80 | 17.05 | 0.68 |

| Vehículos | Simulación completa (ms) | Segundos simulados por segundo real |
|---:|---:|---:|
| 100 | 0.97 | 5,149.29 |
| 1,000 | 11.38 | 439.29 |
| 5,000 | 65.83 | 75.96 |
| 10,000 | 137.26 | 36.43 |

El benchmark completo de 60 muestras necesitó 1.25 segundos de tiempo real,
utilizó aproximadamente el 99 % de un núcleo de CPU y alcanzó un conjunto
residente máximo de 8,924 KiB.

## Cuello de botella identificado

El perfil inicial mostró un crecimiento cuadrático en la simulación completa:

| Vehículos | Simulación completa inicial (ms) | Optimizada (ms) | Aceleración observada |
|---:|---:|---:|---:|
| 100 | 0.93 | 0.97 | 0.95x |
| 1,000 | 33.32 | 11.38 | 2.93x |
| 5,000 | 779.67 | 65.83 | 11.84x |
| 10,000 | 3,077.16 | 137.26 | 22.42x |

Anteriormente, `VehicleManager::addVehicle()` recorría todo el vector de
vehículos en cada inserción. Por lo tanto, generar `n` vehículos realizaba
aproximadamente `O(n²)` comparaciones de identificadores.

Ahora, `VehicleManager` mantiene un `unordered_map` que relaciona cada
identificador de vehículo con su índice dentro del vector. Agregar, buscar y
obtener un vehículo son operaciones `O(1)` en promedio. El índice se actualiza
después de eliminar los vehículos que llegaron a su destino y se limpia cuando
el administrador se reinicia.

En la ejecución optimizada, aumentar la carga de 5,000 a 10,000 vehículos
incrementó el tiempo de la simulación completa aproximadamente 2.08 veces,
demostrando una escalabilidad casi lineal en las cantidades más grandes
medidas.

## Limitación de la comparación

GitHub asignó procesadores diferentes a las dos ejecuciones del workflow:

- Ejecución inicial: Intel Xeon Platinum 8573C
- Ejecución optimizada: AMD EPYC 9V74

Por lo tanto, los valores de aceleración entre ambas ejecuciones son
observacionales y no representan una comparación controlada sobre el mismo
hardware. La eliminación del comportamiento cuadrático también está respaldada
por la escalabilidad interna de la ejecución optimizada y por la estructura sin
cambios de los benchmarks de componentes.

## Experimento limitado de concurrencia

La concurrencia está limitada a lotes de simulaciones independientes. Las
actualizaciones de vehículos, las operaciones de enrutamiento dentro de una
simulación y las estadísticas compartidas permanecen en un solo hilo para
conservar su semántica de sincronización y su comportamiento determinista.

La comparación controlada utilizó:

- Ocho simulaciones independientes por lote.
- Cuatro hilos de trabajo en la versión paralela.
- Los mismos escenarios, semillas, proceso y runner hospedado por GitHub.
- CPU AMD EPYC 7763 con cuatro procesadores lógicos.
- Compilación Release con GCC 13.3.0.
- Commit `4e5b3b425688c0e839fad55a8f931c9728b78663`.
- 201 pruebas aprobadas antes de realizar las mediciones.

| Vehículos por simulación | Lote secuencial (ms) | Lote paralelo (ms) | Aceleración |
|---:|---:|---:|---:|
| 100 | 6.67 | 3.33 | 2.00x |
| 1,000 | 85.17 | 30.53 | 2.79x |
| 5,000 | 486.94 | 175.22 | 2.78x |
| 10,000 | 1,039.45 | 359.42 | 2.89x |

Con 10,000 vehículos, la ejecución paralela utilizó aproximadamente el 397.7 %
de CPU y consiguió cerca del 72 % de eficiencia paralela con cuatro
trabajadores. El rendimiento agregado aumentó de 38.5 a 111.3 segundos
simulados por segundo real.

El proceso completo de benchmarks alcanzó un conjunto residente máximo de
27,668 KiB. Este incremento de memoria está acotado y se debe a la ejecución
simultánea de varias instancias independientes de la simulación.

El experimento demuestra que la concurrencia resulta beneficiosa en el nivel de
los lotes. La API secuencial permanece como opción predeterminada, mientras que
quien utiliza la biblioteca puede seleccionar explícitamente la ejecución
paralela y una cantidad limitada de trabajadores.

## Reproducción

Ejecuta manualmente el workflow `Performance benchmarks` desde GitHub Actions.
El workflow:

1. Compila y ejecuta la suite completa de pruebas en modo Debug.
2. Compila el ejecutable de benchmarks en modo Release.
3. Ejecuta un benchmark rápido de siete muestras.
4. Ejecuta las 84 muestras de rendimiento.
5. Publica como artefacto los resultados CSV, las métricas del proceso y los
   detalles del entorno.

Los resultados generados se excluyen intencionalmente del control de versiones.