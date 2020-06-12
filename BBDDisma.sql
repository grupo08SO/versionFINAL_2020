DROP DATABASE IF EXISTS isma13;
CREATE DATABASE isma13;

USE isma13

CREATE TABLE jugador (
	jugadorID int NOT NULL,
	usuario text,
	pasword text,
	veces int,
	puntos int,
	PRIMARY KEY(jugadorID)
)ENGINE=InnoDB;

INSERT INTO jugador(jugadorID,usuario,pasword) VALUES (1,'isma','isma');
INSERT INTO jugador(jugadorID,usuario,pasword) VALUES (2,'maria','maria');
INSERT INTO jugador(jugadorID,usuario,pasword) VALUES (3,'marta','marta');
INSERT INTO jugador(jugadorID,usuario,pasword) VALUES (4,'juan','juan');

CREATE TABLE partida (
	partidaID int NOT NULL,
	duracion INT,
	ganador text,
	jugador2 text,
	PRIMARY KEY (partidaID)
)ENGINE=InnoDB;

INSERT INTO partida VALUES (1,10,'isma','marta');
INSERT INTO partida VALUES (2,15,'marta','juan');
INSERT INTO partida VALUES (3,10,'isma','marta');

CREATE TABLE relacion (
	jugadorID INT NOT NULL,
	partidaID INT NOT NULL,
	puntuacion INT,
	FOREIGN KEY (jugadorID) REFERENCES jugador(jugadorID),
	FOREIGN KEY (partidaID) REFERENCES partida(partidaID)
)ENGINE=InnoDB;

INSERT INTO relacion(jugadorID,partidaID,puntuacion) VALUES (1,1,200);
INSERT INTO relacion(jugadorID,partidaID,puntuacion) VALUES (3,1,150);
INSERT INTO relacion(jugadorID,partidaID,puntuacion) VALUES (3,2,100);
INSERT INTO relacion(jugadorID,partidaID,puntuacion) VALUES (4,2,50);
INSERT INTO relacion(jugadorID,partidaID,puntuacion) VALUES (1,3,100);
INSERT INTO relacion(jugadorID,partidaID,puntuacion) VALUES (3,3,150);

	




