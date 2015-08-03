#ifndef PARSER__H
#define PARSER__H

#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct PlaylistItem {
	char *uri;
	char *name;
	size_t size;
};
typedef struct PlaylistItem PlaylistItem;
struct Playlist{
	PlaylistItem *items;
	size_t items_count;
};
typedef struct Playlist Playlist;

Playlist* playlist_create();
bool playlist_parse(Playlist *playlist, char* name_of_file);
void playlist_destroy(Playlist *playlist);

#ifdef __cplusplus
}
#endif

#endif//PARSER__H

