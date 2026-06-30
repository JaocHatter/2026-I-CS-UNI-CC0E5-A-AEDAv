-- ============================================================
-- Sirve para: rangos, ordenamientos e igualdad.
-- Operadores soportados: <, <=, =, >=, >, BETWEEN, IN, IS NULL.
-- También acelera ORDER BY.
-- ============================================================

DROP TABLE IF EXISTS personajes;
CREATE TABLE personajes (
    id          SERIAL PRIMARY KEY,
    nombre      VARCHAR(50) NOT NULL,
    raza        VARCHAR(30),
    poder       BIGINT,
    saga        VARCHAR(50)
);

-- Datos de ejemplo
INSERT INTO personajes (nombre, raza, poder, saga) VALUES
    ('Goku',     'Saiyajin',    9000,      'Saiyajin'),
    ('Vegeta',   'Saiyajin',    8000,      'Saiyajin'),
    ('Piccolo',  'Namekiano',   3500,      'Saiyajin'),
    ('Krillin',  'Humano',      1770,      'Freezer'),
    ('Gohan',    'Saiyajin',    1500,      'Saiyajin'),
    ('Freezer',  'Alien',       1200000, 'Freezer'),
    ('Cell',     'Bioandroide', 9000000, 'Cell'),
    ('Bulma',    'Humano',      5,         'Saiyajin');


-- "USING btree" es opcional ya que es el tipo por defecto.
CREATE INDEX idx_personajes_poder ON personajes USING btree (poder);

CREATE INDEX idx_personajes_nombre ON personajes (nombre);

-- ------------------------------------------------------------
-- Ejemplos de consultas que aprovechan el índice B-Tree
-- ------------------------------------------------------------

-- 1) Igualdad
SELECT * FROM personajes WHERE nombre = 'Goku';

-- 2) Rango
SELECT * FROM personajes WHERE poder > 5000;

-- 3) BETWEEN
SELECT * FROM personajes WHERE poder BETWEEN 1000 AND 10000;

-- 4) IN
SELECT * FROM personajes WHERE nombre IN ('Goku', 'Vegeta', 'Piccolo');

-- 5) ORDER BY: el índice ayuda a devolver los resultados ya ordenados
SELECT nombre, poder FROM personajes ORDER BY poder DESC;
