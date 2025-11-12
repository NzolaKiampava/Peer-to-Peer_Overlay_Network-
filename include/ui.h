#ifndef UI_H
#define UI_H

#include "common.h"

/* Comandos da interface */
void ui_start(PeerState *state);
void ui_process_command(PeerState *state, const char *command);

/* Handlers de comandos */
void cmd_join(PeerState *state);
void cmd_leave(PeerState *state);
void cmd_show_neighbors(PeerState *state);
void cmd_release(PeerState *state, int seqnumber);
void cmd_list_identifiers(PeerState *state);
void cmd_post(PeerState *state, const char *identifier);
void cmd_search(PeerState *state, const char *identifier);
void cmd_unpost(PeerState *state, const char *identifier);
void cmd_exit(PeerState *state);

/* Funções auxiliares de UI */
void print_prompt();
void print_help();

#endif /* UI_H */
