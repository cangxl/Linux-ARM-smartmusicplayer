#ifndef LINK_H
#define LINK_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "player.h"


typedef struct Node
{
	char music_name[64];
	struct Node *next;
	struct Node *prior;
}Node;

void create_link(const char *s);
int inserk_link(const char *name);
int init_link();
void traverse_link();
int find_next_music(char *cur, int mode, char *next);
void clear_link();
void update_music();
void get_singer(char *s);
void find_prior_music(char *cur, char *prior);


#endif
