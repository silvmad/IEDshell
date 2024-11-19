/*
Programme réalisant un shell rudimentaire.
Auteur : Victor Matalonga 18905451
Cours : Systèmes d'exploitation chapitre 11
*/

#include "commod.h"
#include "shell.h"
#include "redirections.h"

int main(int argc, string * argv)
{
  char pathname[MaxPathLength];
  char * dirs[MaxDirs];
  char ligne[MaxLigne] = {0};
  char * mots[MaxMot];
  memset(mots, 0, sizeof(char*) * MaxMot);
  bool arr_plan;
  
  char redir_flags;
  string targets[4];
  int pipefd[2][2];
  char commande_suite[MaxLigne] = {0};
  
  FILE * flux_script;
  exec_mode mode = analyse_ldc(argc, argv);

  int tmp;
  string strtmp;
 
  if (mode == SCRIPT_ARG)
    {
      flux_script = fopen(argv[1], "r");
      if (! flux_script)
	{
	  usage("Erreur lors de l'ouverture du fichier");
	}
    }

  decouper(strdup(getenv("PATH")), ":", dirs, MaxDirs);
  
  while (1)
    {
      redir_flags = 0;
      memset(mots, 0, sizeof(char*) * MaxMot);
      switch(mode) //Récupération de l'entrée selon le mode
	{
	case INTERACT:
	  get_user_input(ligne, PROMPT);
	  break;
	case COMM_ARG:
	  strncpy(ligne, argv[2], MaxLigne);
	  break;
	case SCRIPT_ARG:
	  tmp = fscanf(flux_script, " %[^;\n]", ligne);
	  if (tmp == EOF)
	    {
	      return 0;
	    }
	  break;
	default:
	  usage("Corruption de mémoire.");
	  break;
	}
      
      arr_plan = false; //execution en arrière plan
      if (Last(ligne) == '&')
	{
	  Last(ligne) = 0;
	  arr_plan = true;
	}
      
      decouper(ligne, " \t\n", mots, MaxMot);
      if (mots[0] == 0) //ligne vide
	{
	  continue;
	}
      if (!strcmp(mots[0], "cd")) //changement de répertoire
	{
	  if (mots[1] != 0 and mots[2] != 0)
	    {
	      fprintf(stderr, "cd : trop d'arguments\n");
	    }
	  else if (mots[1] == 0) //si pas d'argument on va dans HOME
	    {
	      chdir(getenv("HOME"));
	    }
	  else
	    {
              suppr_antislash(mots[1]);
	      tmp = chdir(mots[1]);
              if (tmp < 0)
                {
                  printf("Le répertoire %s n'existe pas.\n", mots[1]);
                }
	    }
	  continue;
	}
      if (!strcmp(mots[0], "exit")) //quitter
	{
	  exit(0);
	}

      //détection des redirections
      strtmp = parse_redir(mots, &redir_flags, targets, pipefd,
                                commande_suite);
      if (strtmp != 0)
        {
          fprintf(stderr, "Erreur de syntaxe près du symbole %s.\n", strtmp);
          continue;
        }
      if (redir_flags & R_HEREDOC) //écriture du heredoc dans le pipe
	{
	  input_heredoc(pipefd[P_HEREDOC], targets[T_HEREDOC]);
	}
      
      tmp = fork();
      if (tmp < 0)
	{
	  usage("Erreur lors du fork.");
	}

      if (tmp == 0) //enfant
	{
	  //suppression des caractères d'échappement
	  for (idx i = 0; mots[i]; i++)
	    {
	      suppr_antislash(mots[i]);
	    }
	  //effectuer les redirections
	  make_redir(redir_flags, targets, pipefd);
	  //exécution de la commande
	  for(int i = 0; dirs[i] != 0; i++)
	    {
	      snprintf(pathname, sizeof(pathname), "%s/%s", dirs[i], mots[0]);
	      execv(pathname, mots);
	    }
	  execv(mots[0], mots);
	  // aucun exec n'a fonctionne
	  fprintf(stderr, "%s: not found\n", mots[0]);
	  exit(1);
	}

      //parent
      if (redir_flags & R_PIPE) //lancer le reste de la commande après le pipe
	{
	  exec_comm_suite(commande_suite, pipefd[P_REG], argv[0]);
	  memset(commande_suite, 0, MaxLigne);
	}
      if (!arr_plan and redir_flags & R_PIPE)
	{
	  while (wait(0) > 0) //attendre que tous les enfants se terminent
	    ;
	}
      else if (!arr_plan)
	{
	  while (wait(0) != tmp) //attendre que l'enfant direct se termine
	    ;
	}
      
      if (mode == COMM_ARG) //Si on avait l'option -c on quitte
	{
	  exit(0);
	}
    }
  return 0;
}

void usage(string message)
/*
Fonction qui affiche un message d'erreur puis arrête le programme.
*/
{
  fprintf(stderr, "%s\n", message);
  exit(1);
} 

exec_mode analyse_ldc(int argc, string * argv)
/*
Analyse la ligne de commande pour déterminer le mode d'exécution du shell.
Renvoie le mode d'exécution.
*/
{
  if (argc > 3)
    {
      usage("Trop d'arguments");
    }
  else if (argc == 3)
    {
      if (!strcmp("-c", argv[1]))
	{
	  return COMM_ARG;
	}
      else
	{
	  usage("La seule option possible est -c, elle doit être placée"
		"avant l'argument.");
	}
    }
  else if (argc == 2)
    {
      return SCRIPT_ARG;
    }
  return INTERACT;
}

int decouper(string ligne, char * separ, string * mot, int maxmot)
/*
Découpe une chaine en un vecteur de mots, selon un séparateur. Si les
séparateurs sont échappés (avec \) ils ne sont pas pris en compte dans la
découpe et remplacés par un espace.
*/
{
  int i;
  string buffer;
  char ** saveptr = malloc(sizeof(char*));
  mot[0] = strtok_r(ligne, separ, saveptr);
  for(i = 1; mot[i - 1] != 0; i++)
    {
      if (i == maxmot)
	{
	  fprintf(stderr, "Erreur dans la fonction decouper: trop de mots\n");
	  mot[i - 1] = 0;
	  break;
	}
      if (Last(mot[i - 1]) == '\\' and **saveptr != 0) //recoller les séparateurs échappés
	{
	  buffer = malloc(sizeof(char) * MaxMot);
	  strncpy(buffer, mot[i - 1], MaxMot);
	  strncat(buffer, " ", MaxMot - strlen(buffer));
	  strncat(buffer, strtok_r(NULL, separ, saveptr), MaxMot - strlen(buffer));
	  mot[i - 1] = buffer;
	  i--;
	}
      else
	{
	  mot[i] = strtok_r(NULL, separ, saveptr);
	}
    }
  return i;
}

void suppr_antislash(string mot)
/*
Supprime les antislash d'une chaîne.
*/
{
  idx i, j;
  for (i = 0, j = 0; mot[j]; i++, j++)
    {
      if (mot[j] == '\\')
	{
	  j++;
	}
      mot[i] = mot[j];
    }
  mot[i] = 0;
}
