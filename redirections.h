//flags des redirections
#define R_IN 0x01
#define R_IN_APP 0x02
#define R_OUT 0x04
#define R_OUT_APP 0x08
#define R_ERR 0x10
#define R_ERR_APP 0x20
#define R_PIPE 0x40
#define R_HEREDOC 0x80

//cibles
#define T_IN 0
#define T_OUT 1
#define T_ERR 2
#define T_HEREDOC 3

//types de pipes
#define P_REG 0 //pipe utilisé pour |
#define P_HEREDOC 1 //pipe utilisé pour <<

#define PROMPT_H "> "

void usage(string);
void get_user_input(string, string);
void suppr_antislash(string);
