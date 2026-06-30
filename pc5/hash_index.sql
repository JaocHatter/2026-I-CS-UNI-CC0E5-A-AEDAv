-- ============================================================
-- Diseñado para una sola cosa, y la hace muy bien: COINCIDENCIAS EXACTAS
-- Aplica una función hash a la clave y reparte los datos en "buckets"
-- Búsquedas con complejidad promedio O(1)
--
-- Sirve SOLO para igualdad estricta (=).
-- NO sirve para rangos (<, >), ni para ORDER BY
-- Operador soportado: únicamente =
-- ============================================================

DROP TABLE IF EXISTS sesiones;
CREATE TABLE sesiones (
    id            SERIAL PRIMARY KEY,
    personaje     VARCHAR(50) NOT NULL,
    token_sesion  CHAR(32) NOT NULL,
    activa        BOOLEAN DEFAULT TRUE
);

-- Datos de ejemplo (tokens MD5 calculados sobre el nombre)
INSERT INTO sesiones (personaje, token_sesion) VALUES
    ('Goku',    md5('goku-sesion-001')),
    ('Vegeta',  md5('vegeta-sesion-002')),
    ('Piccolo', md5('piccolo-sesion-003')),
    ('Krillin', md5('krillin-sesion-004')),
    ('Gohan',   md5('gohan-sesion-005'));


-- Se debe indicar explícitamente "USING hash".
CREATE INDEX idx_sesiones_token ON sesiones USING hash (token_sesion);

-- Ejemplos:
-- 1) Búsqueda exacta de un token (caso perfecto para Hash)
SELECT * FROM sesiones
WHERE token_sesion = md5('goku-sesion-001');

-- 2) Validar si un token concreto pertenece a una sesión activa
SELECT personaje, activa
FROM sesiones
WHERE token_sesion = md5('vegeta-sesion-002');
