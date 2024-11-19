#ifndef COMMOD_H
#define COMMOD_H

#define and &&
#define or ||

typedef char* string;
typedef int idx;

typedef enum {false, true} bool;
enum
  {
   MaxLigne = 1024,              // longueur max d'une ligne de commandes
   MaxMot = MaxLigne / 2,        // nbre max de mot dans la ligne
   MaxDirs = 100,		// nbre max de repertoire dans PATH
   MaxPathLength = 512,		// longueur max d'un nom de fichier
  };
#endif
