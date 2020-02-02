DROP TABLE IF EXISTS `ftgame`;
CREATE TABLE `ftgame` (
  `id` int(9) NOT NULL AUTO_INCREMENT,
  `name` char(100) DEFAULT NULL,
  PRIMARY KEY (`id`)
);

DROP TABLE IF EXISTS `ftuser`;
CREATE TABLE `ftuser` (
  `id` int(9) NOT NULL AUTO_INCREMENT,
  `hostname` char(50) DEFAULT NULL,
  `stamp` datetime DEFAULT NULL,
  `initial` char(100) DEFAULT NULL,
  PRIMARY KEY (`id`)
);

DROP TABLE IF EXISTS `ftvote`;
CREATE TABLE `ftvote` (
  `id` int(9) NOT NULL AUTO_INCREMENT,
  `game` int(9) DEFAULT NULL,
  `user` int(9) DEFAULT NULL,
  `rate` int(9) DEFAULT NULL,
  PRIMARY KEY (`id`)
);

