#ifndef _ARRAYLIST_H
#define _ARRAYLIST_H

typedef struct {
	unsigned size;
	unsigned capacity;
	int *data;
} list_t;

int  al_init(list_t *list, unsigned capacity);
void al_destroy(list_t *list);

unsigned al_length(list_t *list);
int al_lookup(int *dest, list_t *list, unsigned index);
int al_insert(list_t *list, unsigned index, int src);

int al_push(list_t *list, int src);
int al_pop(int *dest, list_t *list);

#endif
