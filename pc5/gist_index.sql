-- ============================================================
-- PostgreSQL no tiene un tipo "R-Tree" nativo: implementa esa estructura
-- dentro de los índices GiST (Generalized Search Tree).
-- Divide el espacio en "cajas de delimitación mínima" (bounding boxes) que
-- pueden superponerse, permitiendo buscar elementos que están "dentro de",
-- "cerca de" o que "intersecan" un área
--
-- Sirve para: datos geométricos, geográficos, coordenadas y rangos
-- multidimensionales.
-- Operadores soportados: && (intersección), ~= (igualdad espacial),
--                        @> (contiene), <@ (contenido en), entre otros
-- ============================================================


DROP TABLE IF EXISTS ubicaciones;
CREATE TABLE ubicaciones (
    id         SERIAL PRIMARY KEY,
    personaje  VARCHAR(50) NOT NULL,
    posicion   POINT NOT NULL        
);

INSERT INTO ubicaciones (personaje, posicion) VALUES
    ('Goku',     point(0, 0)),      
    ('Krillin',  point(2, 1)),      
    ('Roshi',    point(3, -2)),     
    ('Vegeta',   point(50, 50)),    
    ('Freezer',  point(120, 80));   


-- Se debe indicar "USING gist".
CREATE INDEX idx_ubicaciones_pos ON ubicaciones USING gist (posicion);


-- 1) Contención (<@): personajes que están DENTRO de un cuadro/área.
--    El área es un "box" definido por dos esquinas: (xmin,ymin) y (xmax,ymax)
SELECT personaje, posicion
FROM ubicaciones
WHERE posicion <@ box(point(-5, -5), point(5, 5));   -- zona cercana al origen

-- 2) Proximidad (<->): los 3 personajes MÁS CERCANOS a Goku (0,0).
--    El operador <-> calcula la distancia, GiST permite búsquedas KNN
SELECT personaje, posicion,
       posicion <-> point(0, 0) AS distancia
FROM ubicaciones
ORDER BY posicion <-> point(0, 0)   -- ordena por cercanía
LIMIT 3;

-- 3) Igualdad espacial (~=): buscar quién está EXACTAMENTE en (2,1)
SELECT personaje
FROM ubicaciones
WHERE posicion ~= point(2, 1);

-- Personajes dentro de un círculo de radio 10 centrado en Goku (0,0)
-- El operador @> indica que el círculo CONTIENE el punto.
SELECT personaje, posicion
FROM ubicaciones
WHERE circle(point(0, 0), 10) @> posicion;

-- Adicional: para datos geográficos reales (GPS, lat/long, polígonos)
-- se suele usar la extensión PostGIS, que también se apoya en GiST:
