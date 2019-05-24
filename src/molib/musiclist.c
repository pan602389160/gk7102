#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mozart_musicplayer.h"
//#include "musiclist_interface.h"
#include "linklist_interface.h"

struct music_list {
	List list;
	int max_index;
	int current_index;
	enum play_mode mode;
	void (*free_data)(void *);
};

#define pr_err(fmt, args...)						\
	fprintf(stderr, "[MUSICPLAYER] [ERROR] {%s, %d}: "fmt, __func__, __LINE__, ##args)


struct music_info *mozart_musiclist_get_info(int id, char *music_name, char *music_url,
					     char *music_picture, char *albums_name,
					     char *artists_name, void *data)
{
	struct music_info *info = calloc(sizeof(struct music_info), 1);

	if (info == NULL)
		return NULL;

	info->id = id;

	if (music_name) {
		info->music_name = strdup(music_name);
		if (info->music_name == NULL)
			goto err;
	}
	if (music_url) {
		info->music_url = strdup(music_url);
		if (info->music_url == NULL)
			goto err;
	}
	if (music_picture) {
		info->music_picture = strdup(music_picture);
		if (info->music_picture == NULL)
			goto err;
	}
	if (albums_name) {
		info->albums_name = strdup(albums_name);
		if (info->albums_name == NULL)
			goto err;
	}
	if (artists_name) {
		info->artists_name = strdup(artists_name);
		if (info->artists_name == NULL)
			goto err;
	}

	info->data = data;

	return info;

err:
	free(info->music_name);
	free(info->music_url);
	free(info->music_picture);
	free(albums_name);
	free(artists_name);
	free(info);

	return NULL;
}

struct music_info *mozart_musiclist_copy_info(struct music_info *info)
{
	return mozart_musiclist_get_info(info->id, info->music_name, info->music_url,
					 info->music_picture, info->albums_name,
					 info->artists_name, info->data);
}

void mozart_musiclist_free_music_info(void *music_info, void (*free_data)(void *))
{
	struct music_info *info = (struct music_info *)music_info;

	if (info == NULL)
		return ;

	free(info->music_name);
	free(info->music_url);
	free(info->music_picture);
	free(info->albums_name);
	free(info->artists_name);
	if (free_data)
		free_data(info->data);

	memset((char *)info, 0, sizeof(struct music_info));
	free(info);
}

int mozart_musiclist_get_length(struct music_list *list)
{
	if (list == NULL)
		return -1;
	else
		return list_get_length(&list->list);
}

int mozart_musiclist_set_max_index(struct music_list *list, int max_index)
{
	if (list == NULL)
		return -1;

	list->max_index = max_index;

	return 0;
}

int mozart_musiclist_get_max_index(struct music_list *list)
{
	if (list == NULL)
		return -1;
	else
		return list->max_index;
}

int mozart_musiclist_set_play_mode(struct music_list *list, enum play_mode mode)
{
	if (list == NULL)
		return -1;

	list->mode = mode;

	return 0;
}

enum play_mode mozart_musiclist_get_play_mode(struct music_list *list)
{
	if (list == NULL)
		return play_mode_order;
	else
		return list->mode;
}

int mozart_musiclist_get_current_index(struct music_list *list)
{
	if (list == NULL)
		return -1;

	return list->current_index;
}

struct music_info *mozart_musiclist_get_current(struct music_list *list)
{
	if (list == NULL)
		return NULL;

	if (mozart_musiclist_get_length(list) == 0)
		return NULL;

	return list_get_element(&list->list, list->current_index);
}

struct music_info *mozart_musiclist_get_index(struct music_list *list, int index)
{
	if (list == NULL)
		return NULL;

	if (index >= mozart_musiclist_get_length(list) || index < 0)
		return NULL;

	return list_get_element(&list->list, index);
}

struct music_info *mozart_musiclist_set_prev(struct music_list *list)
{
	if (list == NULL)
		return NULL;

	if (mozart_musiclist_get_length(list) == 0)
		return NULL;

	switch (list->mode) {
	case play_mode_random:
		list->current_index = (unsigned int)random() % mozart_musiclist_get_length(list);
		break;
	case play_mode_single:
	case play_mode_order:
	default:
		if (list->current_index <= 0)
			list->current_index = mozart_musiclist_get_length(list) - 1;
		else
			list->current_index--;
		break;
	}

	return list_get_element(&list->list, list->current_index);
}

struct music_info *mozart_musiclist_set_next(struct music_list *list, bool force)
{
	if (list == NULL)
		return NULL;

	if (mozart_musiclist_get_length(list) == 0)
		return NULL;

	switch (list->mode) {
	case play_mode_random:
		list->current_index = (unsigned int)random() % mozart_musiclist_get_length(list);
		break;
	case play_mode_single:
		if (force == false)
			break;
	case play_mode_order:
	default:
		list->current_index = (list->current_index + 1) %
			mozart_musiclist_get_length(list);
		break;
	}

	return list_get_element(&list->list, list->current_index);
}

struct music_info *mozart_musiclist_set_index(struct music_list *list, int index)
{
	if (list == NULL){
		pr_err("mozart_musiclist_set_index list = NULL\n");
		return NULL;
	}

	if (index >= mozart_musiclist_get_length(list) || index < 0){
		pr_err("mozart_musiclist_set_index index  = %d\n",index);
		return NULL;
	}

	list->current_index = index;
	return list_get_element(&list->list, list->current_index);
}

int mozart_musiclist_insert(struct music_list *list, struct music_info *info)
{
	if (list == NULL || info == NULL)
		return -1;

	if (info->data && list->free_data == NULL) {
		fprintf(stderr, "free_data of musiclist is null\n");
		return -1;
	}

	if (list->max_index >= 0 && list->max_index <= mozart_musiclist_get_length(list)) {
		if (mozart_musiclist_delete_index(list, 0) < 0) {
			fprintf(stderr, "delete oldest info failed\n");
			return -1;
		}
	}

	list_insert_at_tail(&list->list, info);

	if (list->current_index < 0)
		list->current_index = 0;

	return mozart_musiclist_get_length(list) - 1;
}

int mozart_musiclist_delete_index(struct music_list *list, int index)
{
	struct music_info *info;

	if (list == NULL)
		return -1;

	if (index >= mozart_musiclist_get_length(list) || index < 0)
		return -1;

	info = list_delete_at_index(&list->list, index);
	mozart_musiclist_free_music_info(info, list->free_data);

	if (index < list->current_index)
		list->current_index--;

	return 0;
}

struct music_list *mozart_musiclist_copy(struct music_list *list)
{
	int i, len;
	struct music_info *info, *new_info;
	struct music_list *new;

	if (list == NULL)
		return NULL;

	new = calloc(sizeof(struct music_list), 1);
	if (new == NULL)
		return NULL;

	new->max_index = list->max_index;
	new->current_index = list->current_index;
	new->mode = list->mode;
	new->free_data = list->free_data;

	len = mozart_musiclist_get_length(list);
	for (i = 0; i < len; i++) {
		info = mozart_musiclist_get_index(list, i);
		if (info) {
			new_info = mozart_musiclist_copy_info(info);
			if (new_info)
				mozart_musiclist_insert(new, new_info);
		}
	}

	return new;
}

struct music_list *mozart_musiclist_create(void (*free_data)(void *))
{
	struct music_list *list;

	list = calloc(sizeof(struct music_list), 1);
	if (list == NULL)
		return NULL;

	list_init(&list->list);
	list->max_index = -1;
	list->current_index = -1;
	list->mode = play_mode_order;
	list->free_data = free_data;

	srandom(time(NULL));

	return list;
}

int mozart_musiclist_clean(struct music_list *list)
{
	int len, i;
	struct music_info *info;

	if (list == NULL)
		return -1;

	len = mozart_musiclist_get_length(list);
	for (i = 0; i < len; i++) {
		info = list_delete_at_index(&list->list, 0);
		mozart_musiclist_free_music_info(info, list->free_data);
	}

	list->max_index = -1;
	list->current_index = -1;
	list->mode = play_mode_order;

	return 0;
}

int mozart_musiclist_destory(struct music_list *list)
{
	if (list == NULL)
		return -1;

	mozart_musiclist_clean(list);
	list->free_data = NULL;
	free(list);

	return 0;
}

