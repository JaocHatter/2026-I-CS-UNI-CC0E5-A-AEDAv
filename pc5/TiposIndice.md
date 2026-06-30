# Tipos de Indice en PostgresSQL
A continuación los tres tipos de índices en PostgreSQL: **B-Tree**, **Hash** y **R-Tree**.

---

## 1. Índice B-Tree 
Es el tipo de índice estándar y el que se crea automáticamente cuando usas `CREATE INDEX` sin especificar el tipo.

* **¿Cómo funciona?** Organiza los datos en una estructura de árbol balanceado autocomplementaria. Mantiene los datos ordenados, lo que permite búsquedas de complejidad temporal $O(\log n)$.
* **¿Para qué sirve?** Es el más versátil. Sirve para casi todo.
* **Operadores soportados:** `<`, `<=`, `=`, `>=`, `>`, `BETWEEN`, `IN`, `IS NULL`. También ayuda en ordenamientos (`ORDER BY`).

> **Ejemplo de uso:** Buscar usuarios por su edad, filtrar por rangos de fechas de creación o buscar un ID específico.

---

## 2. Índice Hash
Este índice está diseñado para una sola cosa y la hace extremadamente bien: buscar coincidencias exactas.

* **¿Cómo funciona?** Aplica una función hash a la clave de la columna y desglosa los datos en "buckets" (cubetas). Permite búsquedas con una complejidad promedio de $O(1)$.
* **¿Para qué sirve?** **Solo** sirve para comparaciones de igualdad estricta. No sirve para rangos, ni para ordenamientos, ni para buscar si algo es "mayor que".
* **Operadores soportados:** Únicamente `=`.

> **Ejemplo de uso:** Buscar un token de sesión exacto o un hash MD5 de un archivo donde solo te interesa saber si es idéntico.

---

## 3. Índice Espacial R-Tree / GiST (El Geométrico)
PostgreSQL no tiene un tipo llamado "RTree" como tal de forma nativa, sino que implementa la estructura R-Tree dentro de los índices **GiST** (Generalized Search Tree) o mediante la famosa extensión **PostGIS**.

* **¿Cómo funciona?** Divide el espacio bidimensional (o multidimensional) en "cajas de delimitación mínima" (Bounding Boxes) que se superponen entre sí. Permite buscar elementos que están "dentro de", "cerca de" o que "intersecan" un área.
* **¿Para qué sirve?** Para datos geométricos, geográficos, coordenadas GPS y rangos multidimensionales.
* **Operadores soportados:** `&&` (intersección), `~=` (igualdad espacial), `@>` (contiene), `<@` (contenido en), entre otros.

> **Ejemplo de uso:** Buscar los restaurantes que están a menos de 1 km de la ubicación actual de un usuario o trazar polígonos de terrenos.

---

## Tabla Comparativa Completa

| Característica | B-Tree | Hash | R-Tree (GiST / PostGIS) |
| --- | --- | --- | --- |
| **Caso de uso principal** | Rangos, ordenamiento e igualdad | Igualdad exacta ultrarrápida | Datos geométricos y espaciales |
| **Velocidad de búsqueda** | Muy rápida ($O(\log n)$) | Instantánea ($O(1)$ promedio) | Rápida para búsquedas espaciales |
| **Soporta Rangos (`<`, `>`)** | Sí | No | No (usa inclusión/proximidad) |
| **Soporta `ORDER BY**` | Sí | No | No |
| **Sintaxis en Postgres** | `USING btree` (u omitido) | `USING hash` | `USING gist` |