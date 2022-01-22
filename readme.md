(English below)

Ceci est un système d'exploitation en développement pour l'ordinateur A2560U. Il sera plus tard adapté pour le C256 Foenix GenX équippé de carte processeur 68000.
Ce projet est au départ un fork d'EmuTOS, le système d'exploitation libre pour ordinateurs Atari ST.

Pour le construire il faut le cross compilateur GCC 4.6.4 produit par Vincent Rivière:
http://vincent.riviere.free.fr/soft/m68k-atari-mint/
```
git clone https://github.com/vinz6751/genxtos.git
cd genxtos
make a2560u UNIQUE=fr
make -f Makefile.ld
make a2560u UNIQUE=fr
```
Notes:
1. Vous pouvez utiliser utilise UNIQUE=de, UNIQUE=us, UNIQUE=es etc. pour avoir une autre langue, voir doc d'EmuTOS)
2. Il y a un problème avec les makefiles c'est pour ça qu'il faut lancer plusieurs commandes.

L'image emutos-a2560u.rom produite est une image de debuggage située en 0x100000 , vous pouvez la charger dans le A2560U en faisant:
poke32 4 0x100000
Puis l'uploader à cette adresse à l'aide de [C256Mgr](https://github.com/pweingar/C256Mgr) ou [GenX uploader](https://github.com/Trinity-11/GenXUploader)

Voilà grossièrement l'état du projet:
L'OS démarre vers EmuCON (petit shell) et des utilitaires en mode texte peuvent être exécutés.
* Le curseur et l'affichage en mode texte 8x8 fonctionne.
* Il est possible d'utiliser une police 8x16 mais il y a quelques bugs d'affichage à corriger.
* Le clavier français est correctement mappé.
* L'IDE fonctionne (avec un seul périphérique)
* Le lecteur de carte SD fonctionne.
* Le timer système 200Hz fonctionne (il utilise le timer 1)
* RTC presque fonctionnelle
* Souris, son, GEM etc sont à faire.

*English*
This is a fork of the EmuTOS operating system, aiming at providing an OS to the Foenix Retro Systems computers' A2560U and C256 GenX retrocomputer with 68000 CPU module board.
This is very early, look at the ChangeLog.txt to see history.
