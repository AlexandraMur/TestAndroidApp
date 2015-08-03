#include <stdio.h>
#include <cJSON.h>
#include <string.h>
#include "parser.h"

static char* cast_to_standart(char *str){
	str[strlen(str)-1] = '\0';
	char *newstr = strdup(str + 1);
	free(str);
	return newstr;
}

static void set_item(PlaylistItem *item, cJSON *item_in_json){
	cJSON *uri = cJSON_GetObjectItem(item_in_json, "link");
	item->uri = cast_to_standart(cJSON_Print(uri));
	cJSON *name = cJSON_GetObjectItem(item_in_json, "name");
	item->name = cast_to_standart(cJSON_Print(name));
}

static void clear_playlist(Playlist *playlist){
	for (int i = 0; i < playlist->items_count; i++){
		PlaylistItem *item = &playlist->items[i];	
		if (item->uri){
			free(item->uri);
			item->uri = NULL;
		}
		if (item->name){
			free(item->name);
			item->name = NULL;
		}
	}
	if (playlist->items){
		free(playlist->items);
		playlist->items = NULL;
	}
}

static char* copy_to_buffer(char* name_of_file){
	off_t size;
	char *buffer = NULL;
	FILE *file = fopen(name_of_file, "rb");
	if (!file) {
		goto done;
	}
	
	int err = fseeko(file, 0, SEEK_END);
	if (err){
		goto done;
	}
	size = ftello(file);
	fseeko(file, 0, SEEK_SET);
	if (err){
		goto done;
	}

	buffer = malloc(size);
	if(!buffer){ 
		goto done;
	}

	int res = fread(buffer, size, 1, file);
	
	if(!res){
		free(buffer);
		buffer = NULL;
		goto done;
	}
done:	
	if (file) {
		fclose(file);
	}
	return buffer;
}

bool playlist_parse(Playlist* playlist, char *name_of_file){
	bool result = false;

	if(!playlist){
		return false;
	}

	char *pl = copy_to_buffer(name_of_file);
	if(!pl){
		return false;
	}
	
	cJSON *root = cJSON_Parse(pl);
	if (!root){
		goto done;
	}
		
	cJSON *playlist_in_json = cJSON_GetObjectItem(root,"playlist");
	if (!playlist_in_json){
		goto done;
	}	
		
	cJSON *files = cJSON_GetObjectItem(playlist_in_json,"files");
	if (!files){
		goto done;
	}
	
	playlist->items_count = cJSON_GetArraySize(files);
	if (!playlist->items_count) {
		goto done;
	}	

	playlist->items = (PlaylistItem*)calloc(playlist->items_count, sizeof(playlist->items[0]));
	if (!playlist->items){
		playlist->items_count = 0;
		goto done;
	}

	for (int i = 0 ; i < playlist->items_count; i++){
		cJSON *subitem = cJSON_GetArrayItem(files, i);
		set_item(&playlist->items[i], subitem);		
	}
	result = true;
done:
	if(root){
		cJSON_Delete(root);
	}
	return result; 
}

void playlist_destroy(Playlist *playlist){
	if(!playlist){
		return;
	}
	clear_playlist(playlist);
	free(playlist);
}

Playlist* playlist_create(){
	Playlist *playlist = calloc(1, sizeof(Playlist));
	return playlist;
}
