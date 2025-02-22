#include "lfu.h"


//!**********CONSTRUCTORS**********//

//! Function - constructor of empty freq

struct Freq* Empty_Freq_Constructor()
{
	struct Freq* new_freq = (struct Freq*) calloc(1, sizeof(struct Freq));
	assert(new_freq);

	new_freq->value = 0;
	new_freq->child = NULL;
	new_freq->next = new_freq;
	new_freq->prev = new_freq;

	return new_freq;
}


//! Function - constructor of single hash item

struct Item* New_Item_Constructor(data_type data, struct Freq* parent)
{
	struct Item* new_item = (struct Item*) calloc(1, sizeof(struct Item));
	assert(new_item);

	new_item->data = data;
	new_item->parent = parent;

	return new_item;
}


//! Function - constructor of cache

struct Cache* Cache_Constructor(int Cache_MAXSIZE)
{
	struct Cache* cache = (struct Cache*) calloc(1, sizeof(struct Cache));
	assert(cache);

	Hashtab_Constructor(cache->hashtable);

	cache->cache_hits = 0;

	cache->freq_head = Empty_Freq_Constructor();

	cache->cache_max_size = Cache_MAXSIZE;
	cache->cache_size = 0;

	return cache;
}

//!**********END_OF_CONSTRUCTORS**********//


//!**********FUNC_FOR_WORKING_WITH_CACHE**********//

//! Function - constructor of new freq node

struct Freq* Insert_new_Freq(int freq, struct Freq* prev, struct Freq* next)
{
	struct Freq* new_freq = Empty_Freq_Constructor();

	new_freq->value = freq;
	new_freq->prev = prev;
	new_freq->next = next;
	prev->next = new_freq;
	next->prev = new_freq;

	return new_freq;
}

//! Function for insertion in cache

struct Item* Insert_in_Cache(hash_t index, data_type data, struct Cache* cache)
{
	struct Freq* freq = NULL;
	int res = 0;
	assert(cache);

	freq = cache->freq_head->next;

	if (cache->cache_size == cache->cache_max_size)
		res = Remove_LFU(cache);

	if ((freq->value != 1) && (res == 0))
		freq = Insert_new_Freq(1, cache->freq_head, cache->freq_head->next);
	else
		if ((freq->value != 1) && (res == 1))
			freq = Insert_new_Freq(1, cache->freq_head, cache->freq_head->next);

	Add(freq, index, data, cache);

	return freq->child;
}


//! Accessory function for insertion in cache

void Add(struct Freq* freq, hash_t index, data_type data, struct Cache* cache)
{
	struct Item* item = New_Item_Constructor(data, freq), * tmp = NULL;

	assert(cache);

	if (cache->cache_size != cache->cache_max_size)
		cache->cache_size += 1;

	if (freq->child == NULL)
	{
		freq->child = item;
		freq->old_child = item;
	}
	else
	{
		tmp = freq->child;
		freq->child = item;
		item->next = tmp;
		tmp->prev = item;
	}

	Hashtab_Insert(item, cache->hashtable);
}


//! Function for removing item from selected place
void Remove_Item(struct Item* cur, hash_t index, struct Cache* cache)
{
	struct Freq* freq = cur->parent;
	cache->cache_size -= 1;

	Hashtab_Delete(cur->data, cache->hashtable);

	if (cur->prev == NULL && cur->next == NULL)
	{
		free(cur);
		freq->old_child = NULL;
		freq->child = NULL;

		if (freq->value != 1)
			Delete_Freq(freq);
	}
	else if (cur->next == NULL)
	{
		cur->prev->next = NULL;
		freq->old_child = cur->prev;

		free(cur);
	}
	else if (cur->prev == NULL)
	{
		freq->child = cur->next;
		cur->next->prev = NULL;

		free(cur);
	}
	else
	{
		cur->prev->next = cur->next;
		cur->next->prev = cur->prev;
		free(cur);
	}
}


//! Function for accessing to item

data_type Access_to_Item(data_type data, hash_t index, struct Cache* cache)
{
	int res1 = 0;
	struct Freq* freq = NULL, * next_freq = NULL;
	struct Hash_Item* cur_hash = cache->hashtable[index];
	struct Output* res = Hashtab_Lookup(cur_hash, data);

	assert(cache);

	switch (res->res)
	{
	case DOESNT_EXIST:
		res->item = Insert_in_Cache(index, data, cache);

		break;

	case EXIST:
		cache->cache_hits += 1;
		freq = res->item->parent;
		next_freq = freq->next;

		if ((next_freq == cache->freq_head) || (next_freq->value != (freq->value + 1)))
		{
			next_freq = Insert_new_Freq(freq->value + 1, freq, next_freq);
		}

		Remove_Item(res->item, index, cache);

		Add(next_freq, index, data, cache);

		if (freq->child == NULL)
		{
			Delete_Freq(freq);
		}

		break;
	}

	res1 = res->item->data;

	free(res);

	return res1;
}


//! Function for input data

hash_t Input(FILE* f, struct Cache* cache)
{
	int res = 0;
	data_type data = 0;
	hash_t hash = -1;

	assert(cache);

	res = fscanf(f, "%d", &data);
	assert(res == 1);

	hash = Hash_Func(data);
	assert(hash >= 0);

	Access_to_Item(data, hash, cache);

	return hash;
}

//! Function removing item - old- child

int Remove_LFU(struct Cache* cache)
{
	data_type data = 0;
	hash_t index, res = 0;

	assert(cache);

	struct Freq* freq = cache->freq_head->next;
	struct Item* item = freq->old_child;

	cache->cache_size -= 1;
	data = item->data;
	index = Hash_Func(data);

	if (item->prev == NULL)
	{
		free(item);
		freq->old_child = NULL;
		freq->child = NULL;

		if (freq->value != 1)
		{
			Delete_Freq(freq);
			res = 1;
		}
	}
	else
	{
		item->prev->next = NULL;
		freq->old_child = item->prev;
		free(item);
	}

	Hashtab_Delete(data, cache->hashtable);

	return res;
}


//!**********DISTRUCTORS**********//


//! Function-distructor for freq-list

void Freq_Distructor(struct Freq* freq)
{
	freq->prev->next = NULL;
	struct Freq* freq_next = freq->next;
	struct Freq* tmp = NULL;

	free(freq);

	while (freq_next != NULL)
	{
		tmp = freq_next->next;
		free(freq_next);
		freq_next = tmp;
	}
}

//! Function-distructor for cache

void Cache_Distructor(struct Cache* cache)
{
	assert(cache);

	Hashtab_Distructor(cache->hashtable);
	Freq_Distructor(cache->freq_head);

	free(cache);
}

//! Function for deleting single freq

void Delete_Freq(struct Freq* freq)
{
	freq->prev->next = freq->next;
	freq->next->prev = freq->prev;

	free(freq);
}


//!**********DEBUG_FUNC**********//

#if MODE == 1 

void Cache_Dump(struct Cache* cache, hash_t hash)
{
	struct Hash_Item* hash_debug = cache->hashtable[hash];
	struct Freq* freq_deb = cache->freq_head->next;
	struct Item* item_debug = freq_deb->child;

	assert(cache);

	printf("\n\n");
	printf("Cache_hits: %d\n", cache->cache_hits);
	printf("Cache_max_size: %d\n", cache->cache_max_size);
	printf("Cache_size: %d\n", cache->cache_size);

	while (freq_deb != cache->freq_head)
	{
		printf("Freq %d: ", freq_deb->value);
		item_debug = freq_deb->child;

		while (item_debug != NULL)
		{
			printf("%d ", item_debug->data);
			item_debug = item_debug->next;
		}

		printf("\n");
		freq_deb = freq_deb->next;
	}

	printf("\n");

	printf("| hashtable[%d]: |", hash);

	while (hash_debug != NULL)
	{
		printf("| %d | -> ", hash_debug->value);
		hash_debug = hash_debug->next;
	}
	printf("\n\n");
}

#endif
