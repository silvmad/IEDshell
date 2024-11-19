#include "commod.h"
#include "redirections.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

string parse_redir(string * mots, char * redir_f_pt, string * targets,
		 int pipefd[][2], string comm_suite)
/*
Analyse une commande en recherchant la présence de redirections.
Met à jour les flags et le vecteurs de cibles en fonctions redirections trouvées, crée un pipe si besoin et dans ce cas 
conserve la suite de la commande dans une chaine.
Mots est la commande sous forme de vecteur de mots, redir_f_pt est 
un pointeur sur les flags de redirection, targets le vecteur de
cibles, pipefd le vecteur des descripteurs de fichiers des pipes et
comm_suite une chaine qui contient la suite de la commande si un
pipe est trouvé.
*/
{
  idx i;
  int tmp;
  /* Le test de continuation de la boucle est fait sur mots[i+1] au lieu de
  mots[i] pour éviter les erreurs lorsque l'utilisateur a entré un
  symbole de redirection sans cible en fin de commande 
  (par exemple "echo a >").  
  */
  for (i = 0; mots[i] != 0; i++)
    {
      if (!strcmp(mots[i], ">"))
      {
        if (mots[i+1] == 0)
          {  
            return mots[i];
          }
	*redir_f_pt |= R_OUT;
	targets[T_OUT] = strdup(mots[i+1]);
	mots[i] = NULL;
	mots[++i] = NULL;
      }
      else if (!strcmp(mots[i], ">>"))
	{
        if (mots[i+1] == 0)
          {  
            return mots[i];
          }
	  *redir_f_pt |= R_OUT_APP;
	  targets[T_OUT] = strdup(mots[i+1]);
	  mots[i] = NULL;
	  mots[++i] = NULL;
	}
      else if (!strcmp(mots[i], "<"))
	{
        if (mots[i+1] == 0)
          {  
            return mots[i];
          }
	  *redir_f_pt |= R_IN;
	  targets[T_IN] = strdup(mots[i+1]);
	  mots[i] = NULL;
	  mots[++i] = NULL;
	}
      else if (!strcmp(mots[i], "<<"))
	{
        if (mots[i+1] == 0)
          {  
            return mots[i];
          }
	  *redir_f_pt |= R_HEREDOC;
	  targets[T_HEREDOC] = strdup(mots[i+1]);
	  mots[i] = NULL;
	  mots[i+1] = NULL;
	  int tmp = pipe(pipefd[P_HEREDOC]);
	  if (tmp < 0)
	    {
	      perror("pip");
	      usage("Erreur lors de l'ouverture du pipe.");
	    }
	}
      else if (!strcmp(mots[i], "2>"))
	{
        if (mots[i+1] == 0)
          {  
            return mots[i];
          }
	  *redir_f_pt |= R_ERR;
	  targets[T_ERR] = strdup(mots[i+1]);
	  mots[i] = NULL;
	  mots[++i] = NULL;
	}
      else if (!strcmp(mots[i], "2>>"))
	{
        if (mots[i+1] == 0)
          {  
            return mots[i];
          }
	  *redir_f_pt |= R_ERR_APP;
	  targets[T_ERR] = strdup(mots[i+1]);
	  mots[i] = NULL;
	  mots[++i] = NULL;
	}
      else if (!strcmp(mots[i], "|"))
	{
        if (mots[i+1] == 0)
          {  
            return mots[i];
          }
	  mots[i] = NULL;
	  *redir_f_pt |= R_PIPE;
	  //copie de la suite de la commande dans commande_suite
	  for (int j = i + 1; mots[j]; j++)
	    {
	      strncat(comm_suite, mots[j], MaxLigne - strlen(comm_suite));
	      strncat(comm_suite, " ", MaxLigne - strlen(comm_suite));
	    }
	  tmp = pipe(pipefd[P_REG]);
	  if (tmp < 0)
	    {
	      usage("Erreur lors de l'ouverture du pipe");
	    }
	  break; //si un pipe a été trouvé on ne cherche pas les suivants
	}
    }
  /* Recomposition de la commande, on supprime les éventuels 0 laissés
     par la suppresssion de redirections. */
  int k = 0;
  for (int j = 0; j < i; j++)  
    {
      if (mots[j] != NULL)
	{
	  mots[k] = mots[j];
	  k++;
	}
    }
  return 0;
}

void make_redir(char redir_flags, string * targets, int pipefd[][2])
/*
Effectue les redirections marquées dans redir_flags vers les cibles 
contenues dans targets et les pipes contenus dans pipefd. 
*/
{
  if (redir_flags & R_PIPE)
    //redirection de stdout vers la partie écriture du pipe REG
    {
      close(pipefd[P_REG][0]);
      close(1);
      dup2(pipefd[P_REG][1], 1);
      close(pipefd[P_REG][1]);
    }
  if (redir_flags & R_HEREDOC)
    //redirection de stdin vers la partie lecture du pipe HEREDOC
    {
      close(pipefd[P_HEREDOC][1]);
      close(0);
      dup2(pipefd[P_HEREDOC][0], 0);
      close(pipefd[P_HEREDOC][0]);
    }
  if (redir_flags & R_OUT)
    {
      suppr_antislash(targets[T_OUT]);
      freopen(targets[T_OUT], "w", stdout);
    }
  if (redir_flags & R_OUT_APP)
    {
      suppr_antislash(targets[T_OUT]);
      freopen(targets[T_OUT], "a", stdout);
    }
  if (redir_flags & R_IN)
    {
      suppr_antislash(targets[T_IN]);
      freopen(targets[T_IN], "r", stdin);
    }
  if (redir_flags & R_ERR)
    {
      suppr_antislash(targets[T_ERR]);
      freopen(targets[T_ERR], "w", stderr);
    }
  if (redir_flags & R_ERR_APP)
    {
      suppr_antislash(targets[T_ERR]);
      freopen(targets[T_ERR], "a", stderr);
    }
}

void exec_comm_suite(string comm_suite, int * pipefd, string nom_prog)
/*
Crée un processus enfant, lui redirige stdin vers la partie lecture du 
pipe dont les descripteurs de fichiers sont dans pipefd et lui fait 
lancer un shell avec l'option -c et comm_suite en argument, ce shell
lancera donc la commande et quittera directement après. 
Nom_prog doit contenir le nom du programme (argv[0]).
*/
{
  int tmp = fork();
  char pathname[MaxPathLength + 1];
  if (tmp < 0)
    {
      perror("fork");
      exit(1);
    }
  if (tmp == 0) // enfant
    {
      //redirection de stdin vers la partie lecture du pipe
      close(pipefd[1]);
      close(0);
      dup2(pipefd[0], 0);
      close(pipefd[0]);
      //construction du chemin vers le shell
      char cwd[MaxPathLength];
      getcwd(cwd, MaxPathLength);
      snprintf(pathname, sizeof(pathname), "%s/%s", cwd, nom_prog + 2);
      //execution du shell avec la suite de la commande en argument
      execl(pathname, nom_prog, "-c", comm_suite, NULL);
    }
  //parent : fermeture du pipe
  close(pipefd[0]);
  close(pipefd[1]);
}

void input_heredoc(int * pipefd, string stop)
/*
Écrit l'entrée de l'utilisateur dans le pipe dont les descripteurs de 
fichier sont dans pipefd, jusqu'à ce qu'il entre la chaine stop.
*/
{
  char ligne[MaxLigne] = {0};
  for (get_user_input(ligne, PROMPT_H); strcmp(ligne, stop);
       get_user_input(ligne, PROMPT_H))
    {
      write(pipefd[1], ligne, strlen(ligne));
      write(pipefd[1], "\n", 1);
    }
  close(pipefd[1]);
}
