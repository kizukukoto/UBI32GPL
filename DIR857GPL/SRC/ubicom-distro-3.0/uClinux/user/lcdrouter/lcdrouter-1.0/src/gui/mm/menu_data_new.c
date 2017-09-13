/*
 * Ubicom Network Player
 *
 * (C) Copyright 2009, Ubicom, Inc.
 * Celil URGAN curgan@ubicom.com
 * Ergun CELEBIOGLU ecelebioglu@ubicom.com
 *
 * The Network Player Reference Code is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Network Player Reference Code is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Network Audio Device Reference Code.  If not,
 * see <http://www.gnu.org/licenses/>.
 *
 */
#include "gui/mm/menu_data.h"

//#define TEST
#ifdef TEST
debug_levels_t global_debug_level = ERROR;
#endif

#define K (1024)
#define CHUNK_SIZE (2)
#define FREE(x) if( x != NULL ) free(x);
#define ITEM_LIST_CHUNK_SIZE (50 * CHUNK_SIZE * K)
#define METADATA_AREA_CHUNK_SIZE (1 * K * CHUNK_SIZE)

struct data_chunk {
	void *mem_pointer; //2k or 4k mem block pointer
	struct data_chunk *next;//swap !!! and use array
};

struct menu_data_instance {
	struct data_chunk *metadata_area;
	struct data_chunk *item_list;
	int	list_item_count;
	struct data_chunk *end_chunk;
	struct menu_data_instance  *prev;
	unsigned short end_offset;
	unsigned short focused_item;

	int refs;
#ifdef DEBUG
	int	magic;
#endif
};

#if defined(DEBUG)
#define MENU_DATA_MAGIC 0xCAFE
#define MENU_DATA_VERIFY_MAGIC(mdi) menu_data_verify_magic(mdi)

void menu_data_verify_magic(struct menu_data_instance *mdi)
{
	DEBUG_ASSERT(mdi->magic == MENU_DATA_MAGIC, "%p: bad magic", mdi);
}
#else
#define MENU_DATA_VERIFY_MAGIC(mdi)
#endif

struct list_item {
	struct data_chunk *start_chunk;
	struct data_chunk *end_chunk;
	unsigned short item_data_start_offset;//address of item in 2k or 4k data chunk (mem position)
	unsigned short item_data_end_offset;//address of item in 2k or 4k data chunk (mem position)
	int flag;
	struct menu_data_instance  *sub_menu;
	void	     *callback;
	void	     *callbackdata;

#ifdef DEBUG
	int	magic;
#endif
};//20 byte packed + magic

static void menu_data_instance_dump_chunk_memory(struct data_chunk *ptr);

#if defined(DEBUG)
#define LIST_ITEM_MAGIC 0xFECA
#define LIST_ITEM_VERIFY_MAGIC(li) list_item_verify_magic(li)

void list_item_verify_magic(struct list_item *li)
{
	DEBUG_ASSERT(li->magic == MENU_DATA_MAGIC, "%p: bad magic", li);
}
#else
#define LIST_ITEM_VERIFY_MAGIC(li)
#endif

static void menu_data_instance_destroy(struct menu_data_instance *mdi)
{
	DEBUG_ASSERT(mdi == NULL, " menu data destroy with NULL instance ");
	//free other internal members
#ifdef DEBUG
	mdi->magic = 0;
#endif
	FREE(mdi->item_list->mem_pointer);
	FREE(mdi->item_list);
	FREE(mdi->metadata_area->mem_pointer);
	FREE(mdi->metadata_area);
	FREE(mdi);
	mdi = NULL;
}

int  menu_data_instance_ref(struct menu_data_instance *mdi)
{
	MENU_DATA_VERIFY_MAGIC(mdi);
	//needs pthread llock
	
	return ++mdi->refs;
}

int  menu_data_instance_deref(struct menu_data_instance *mdi)
{
	MENU_DATA_VERIFY_MAGIC(mdi);
	DEBUG_ASSERT(mdi->refs, "%p: menu has no references", mdi);
	//needs pthread llock
	if ( --mdi->refs == 0 ) {
#ifdef DEBUG
		mdi->magic = 0;
#endif
		FREE(mdi->item_list->mem_pointer);
		FREE(mdi->item_list);
		FREE(mdi->metadata_area->mem_pointer);
		FREE(mdi->metadata_area);
		FREE(mdi);
		mdi = NULL;
		return 0;
	}
	else {
		return mdi->refs;
	}
}

int menu_data_instance_get_list_item_count(struct menu_data_instance *menu_data)
{
	MENU_DATA_VERIFY_MAGIC(mdi);
	return menu_data->list_item_count;
}

unsigned short menu_data_instance_get_focused_item(struct menu_data_instance *mdi)
{
	MENU_DATA_VERIFY_MAGIC(mdi);
	return mdi->focused_item;
}

void menu_data_instance_set_focused_item(struct menu_data_instance *mdi, unsigned short item_no)
{
	MENU_DATA_VERIFY_MAGIC(mdi);
	mdi->focused_item = item_no;
}

void menu_data_instance_set_prev_menu(struct menu_data_instance *mdi, struct menu_data_instance *prev)
{
	MENU_DATA_VERIFY_MAGIC(mdi);
	mdi->prev = prev;
}

struct menu_data_instance* menu_data_instance_get_prev_menu(struct menu_data_instance *mdi)
{
	MENU_DATA_VERIFY_MAGIC(mdi);
	return mdi->prev;
}

void menu_data_instance_set_listitem_flag(struct list_item *list_item, int flag)
{
	LIST_ITEM_VERIFY_MAGIC(li);
	list_item->flag |= flag;
}

int menu_data_instance_get_listitem_flag(struct list_item *list_item)
{
	LIST_ITEM_VERIFY_MAGIC(li);
	return list_item->flag;
}

void menu_data_instance_set_listitem_submenu(struct list_item *list_item, struct menu_data_instance *mdi)
{
	LIST_ITEM_VERIFY_MAGIC(li);
	list_item->sub_menu = mdi;
}

struct menu_data_instance* menu_data_instance_get_listitem_submenu(struct list_item *list_item)
{
	LIST_ITEM_VERIFY_MAGIC(li);
	return list_item->sub_menu;
}

void menu_data_instance_set_listitem_callback(struct list_item *list_item, void *callback)
{
	LIST_ITEM_VERIFY_MAGIC(li);
	list_item->callback = callback;
}

void* menu_data_instance_get_listitem_callback(struct list_item *list_item)
{
	LIST_ITEM_VERIFY_MAGIC(li);
	return list_item->callback;
}

void menu_data_instance_set_listitem_callbackdata(struct list_item *list_item, void *callbackdata)
{
	LIST_ITEM_VERIFY_MAGIC(li);
	list_item->callbackdata = callbackdata;
}

void* menu_data_instance_get_listitem_callbackdata(struct list_item *list_item)
{
	LIST_ITEM_VERIFY_MAGIC(li);
	return list_item->callbackdata;
}

struct list_item* menu_data_instance_list_item_open(struct menu_data_instance *mdi)
{
	//check assert mdi
	struct list_item *list_item = calloc( 1 , sizeof(struct list_item) );
	if( list_item == NULL) {
		DEBUG_ERROR("list_item alloc failed!\n");
		return NULL;
	}

	//mdi->flag |= LIST_OPEN;
	list_item->sub_menu = NULL;
	list_item->callback = NULL;
	list_item->callbackdata = NULL;
	DEBUG_INFO("%p: allocated list_item", list_item);
	return list_item;
}

int menu_data_instance_list_item_close(struct menu_data_instance *mdi, struct list_item *list_item)
{
	//check assert mdi and list_item
	//if(!(mdi->flag & LIST_OPEN))  return -1;

	struct data_chunk *ptr = mdi->item_list;	
	while(ptr->next) {
		//DEBUG_TRACE("Goto next chunk: %p", ptr->next);
		ptr = ptr->next;
	}

	if ( mdi->list_item_count && !(mdi->list_item_count * sizeof(struct list_item) % ITEM_LIST_CHUNK_SIZE) ) {
		//check if enough space exists for list_item
		DEBUG_TRACE("we need new chunk for item list!");
		struct data_chunk *last = calloc( 1, sizeof(struct data_chunk) );
		if( last == NULL) {
			DEBUG_ERROR("calloc for last fails");
			return -1;
		}

		last->mem_pointer = calloc( 1, ITEM_LIST_CHUNK_SIZE );
		if( last->mem_pointer == NULL) {
			DEBUG_ERROR("calloc for last->mempointer fails");
			free(last);
			return -1;
		}

		ptr->next = last;
		last->next = NULL;
		DEBUG_TRACE("new allocated item list chunk: %p\n", ptr->next);
		ptr = last;
	}

	int chunk_offset = 0;
	DEBUG_TRACE("list_item_count: %d", mdi->list_item_count);
	
	chunk_offset = (mdi->list_item_count * sizeof(struct list_item)) % ITEM_LIST_CHUNK_SIZE;

	DEBUG_TRACE("chunk_offset: %d", chunk_offset);

	char *tmp = (char*)ptr->mem_pointer;
	tmp += chunk_offset;

	memcpy(tmp, list_item, sizeof(struct list_item));

	mdi->list_item_count++;

	free(list_item);

	DEBUG_TRACE("list_item closed");
	//mdi->flag &= ~LIST_OPEN;
	return 1;
}

struct list_item* menu_data_instance_get_list_item(struct menu_data_instance *mdi, int index)
{
	//check assert mdi and list_item
	if(mdi == NULL) {
		DEBUG_ERROR("menu_Data is NULL...");	
	}
	if(mdi->list_item_count <= index) {
		return NULL;
	}
	
	int chunk_num = (index * sizeof(struct list_item)) / ITEM_LIST_CHUNK_SIZE;
	DEBUG_TRACE("chunk_num %d", chunk_num);
	int chunk_offset = (index * sizeof(struct list_item)) % ITEM_LIST_CHUNK_SIZE;	

	struct data_chunk *ptr = mdi->item_list;	
	while(chunk_num--) {
		ptr = ptr->next;
	}
	
	char *tmp = (char*)ptr->mem_pointer;
	tmp += chunk_offset;

	DEBUG_TRACE("index %d  chunk_offset %d  get_list_item() = %p %p",index, chunk_offset, tmp, ptr);
	
	return (struct list_item*)(tmp);	
}

static
int menu_data_instance_find_metadata(struct list_item *list_item, menu_data_types_t type,
					struct data_chunk **start_chunk,int *start_offset, int *length)
{
	if(list_item == NULL) {
		DEBUG_ERROR("Null list_item instance");
		return -1;	
	}
	unsigned short read_type ;
	unsigned short len ;

	struct data_chunk *current_chunk = list_item->start_chunk;
	char *data_ptr = (char*)(current_chunk->mem_pointer) + list_item->item_data_start_offset;
	char *end_ptr = (char *)list_item->end_chunk->mem_pointer + list_item->item_data_end_offset;
	DEBUG_ERROR("New FIND");
	do {
		//menu_data_instance_dump_chunk_memory(current_chunk);
		memcpy(&read_type, data_ptr, sizeof(unsigned short));
		data_ptr += sizeof(unsigned short);
		memcpy(&len, data_ptr, sizeof(unsigned short));
		data_ptr += sizeof(unsigned short);
		
		int skip_bytes = ((int)current_chunk->mem_pointer - (int)data_ptr) +METADATA_AREA_CHUNK_SIZE ;

		DEBUG_ERROR("read type %2x len %d skip %d", read_type, len, skip_bytes);

		if (read_type == type) {
			DEBUG_ERROR("item found");
			*start_chunk = current_chunk;
			*start_offset = (int)data_ptr -(int)current_chunk->mem_pointer;
			*length = len;
			return 1;
		}
		
		skip_bytes -= len;
		
		if ( skip_bytes > (int)(2*sizeof(unsigned short))) {
			DEBUG_ERROR("find in same chunk %d %d", skip_bytes, len);
			data_ptr += len;
			if (current_chunk == list_item->end_chunk && ((int)data_ptr > (int)end_ptr)) {
				DEBUG_ERROR("can not find item");
				return -1;
			}			
			continue;
		}
		else {
			DEBUG_ERROR("find in next chunk %d %d", skip_bytes, len);
			if (current_chunk == list_item->end_chunk) {
				DEBUG_ERROR("can not find item");
				return -1;
			}

			if (skip_bytes < 0) {
				skip_bytes *= -1;
			}
			else {
				skip_bytes = 0;
			}
			current_chunk = current_chunk->next;
			data_ptr = (char*)(current_chunk->mem_pointer) + skip_bytes;
			continue;
		}
	}while (1);
}

char* menu_data_instance_get_metadata(struct list_item *list_item, menu_data_types_t type)
{
	DEBUG_TRACE("function call");	
	if(list_item == NULL) {
		DEBUG_ERROR("Null list_item instance");
		return NULL;	
	}

	struct data_chunk *current_chunk;
	int start_offset;
	int length;

	if (menu_data_instance_find_metadata(list_item, type, &current_chunk, &start_offset, &length) > 0) {

		char *tmp_data1, *tmp_data2, *return_data;
		char *data_ptr;
		int bytes_in_prev_chunk = METADATA_AREA_CHUNK_SIZE - start_offset;

		DEBUG_ERROR("bytes_in_prev_chunk =%d , start_offset %d, length %d", bytes_in_prev_chunk, start_offset, length);
		data_ptr = (char*)(current_chunk->mem_pointer) + start_offset;

		//menu_data_instance_dump_chunk_memory(current_chunk);
		if (bytes_in_prev_chunk >= length) {
			return_data = (char*)malloc(length);
			memcpy(return_data, data_ptr, length);
			//DEBUG_ERROR("item FOUND part =%s", return_data);
			return return_data;
		}

		tmp_data1 = (char*)malloc( bytes_in_prev_chunk);
		memcpy(tmp_data1, data_ptr, bytes_in_prev_chunk);
		//DEBUG_ERROR("items first part =%s", tmp_data1);

		current_chunk = list_item->start_chunk->next;
		data_ptr = (char*)(current_chunk->mem_pointer);

		menu_data_instance_dump_chunk_memory(current_chunk);
		tmp_data2 = (char*)malloc( length - bytes_in_prev_chunk);
		memcpy(tmp_data2, data_ptr, length - bytes_in_prev_chunk);
		//DEBUG_ERROR("items second part =%s", tmp_data2);

		return_data = (char*)malloc(length);
		memcpy(return_data, tmp_data1, bytes_in_prev_chunk);
		memcpy(return_data + bytes_in_prev_chunk, tmp_data2, length - bytes_in_prev_chunk);

		free(tmp_data1);
		free(tmp_data2);

		DEBUG_ERROR("full part =%s", return_data);
		return return_data;
	}

	DEBUG_WARN("item not found");
	return NULL;
}

int menu_data_instance_update_metadata(struct list_item *list_item, menu_data_types_t type, char *data, int len)
{
	DEBUG_TRACE("function call");	
	if(list_item == NULL) {
		DEBUG_ERROR("Null list_item instance");
		return NULL;	
	}

	struct data_chunk *current_chunk;
	int start_offset;
	int length;

	if (menu_data_instance_find_metadata(list_item, type, &current_chunk, &start_offset, &length) > 0) {

		char *tmp_data1, *tmp_data2, *return_data;
		char *data_ptr;
		int bytes_in_prev_chunk = METADATA_AREA_CHUNK_SIZE - start_offset;

		if (len != length) {
			DEBUG_ERROR("update len %d, length %d", len, length);
			return -1;
		}

		DEBUG_ERROR("bytes_in_prev_chunk =%d , start_offset %d, length %d", bytes_in_prev_chunk, start_offset, length);
		data_ptr = (char*)(current_chunk->mem_pointer) + start_offset;

		//menu_data_instance_dump_chunk_memory(current_chunk);
		if (bytes_in_prev_chunk >= length) {
			memcpy(data_ptr, data, length);
			//DEBUG_ERROR("item FOUND part =%s", return_data);
			return 1;
		}

		memcpy(data_ptr, data, bytes_in_prev_chunk);
		data += bytes_in_prev_chunk;

		current_chunk = list_item->start_chunk->next;
		data_ptr = (char*)(current_chunk->mem_pointer);

		//menu_data_instance_dump_chunk_memory(current_chunk);
		memcpy(data_ptr, data, length - bytes_in_prev_chunk);

		//DEBUG_ERROR("full part =%s", return_data);
		return 1;
	}

	DEBUG_WARN("item not found");
	return -1;
}

static
int menu_data_instance_get_free_space(struct menu_data_instance *mdi)
{
	return METADATA_AREA_CHUNK_SIZE - mdi->end_offset;
}

int menu_data_instance_add_metadata(struct menu_data_instance *mdi, struct list_item *list_item, menu_data_types_t type, int len, void *data)
{
	//check assert mdi and list_item
	if(!len) {
		DEBUG_ERROR("element item length is zero!");
		return -1;
	}

	if(list_item == NULL) {
		DEBUG_ERROR("Null list_item instance");
		return NULL;	
	}

	unsigned short type_tmp = type;
	unsigned short len_tmp = len;

	/* get last pos pointer , writing will start from here*/
	char *data_ptr = (char*)mdi->end_chunk->mem_pointer;
	data_ptr += mdi->end_offset;

	int free_space =  menu_data_instance_get_free_space(mdi);
	int needed_space = len + 2 * sizeof(unsigned short); // coming data with null terminated and 2 byte for type and len values.

	if ( needed_space > free_space) {
		DEBUG_ERROR("free space in last chunk is %d and needed %d",  free_space,needed_space);
		if (free_space > 2 * sizeof(unsigned short)) {
		#ifdef	PC
			*data_ptr = type_tmp & 0x00ff;
			data_ptr++;
			*data_ptr = type_tmp >> 8;	
			data_ptr++;

			*data_ptr = len_tmp & 0x00ff;
			data_ptr++;
			*data_ptr = len_tmp >> 8;
			data_ptr++;	
		#else
			*data_ptr = type_tmp >> 8;	
			data_ptr++;
			*data_ptr = type_tmp & 0x00ff;
			data_ptr++;

			*data_ptr = len_tmp >> 8;
			data_ptr++;
			*data_ptr = len_tmp & 0x00ff;
			data_ptr++;

		#endif
			memcpy(data_ptr, data, free_space - 2 * sizeof(unsigned short));
			len -= free_space - 2 * sizeof(unsigned short); // we write some so decrease len for future usages.
			data += free_space - 2 * sizeof(unsigned short);

			if (list_item->start_chunk == NULL ) {
				list_item->start_chunk = mdi->end_chunk;
				list_item->item_data_start_offset = mdi->end_offset;
			} 
		}
		//else {
		//	DEBUG_ERROR("last chunk has less then 2 bytes!!!");
		//	abort();
		//	return -1;
		//}

		//menu_data_instance_dump_chunk_memory(mdi->end_chunk);

		DEBUG_TRACE("needs to create new chunk and copy with truncated");
		struct data_chunk *new_chunk = calloc( 1, sizeof(struct data_chunk) );
		if( new_chunk == NULL) {
			DEBUG_ERROR("calloc for new_chunk fails");
			return -1;
		}

		new_chunk->next = NULL;

		new_chunk->mem_pointer = calloc( 1, METADATA_AREA_CHUNK_SIZE );
		if( new_chunk->mem_pointer == NULL) {
			DEBUG_ERROR("calloc for new_chunk->mempointer fails");
			free(new_chunk);
			return -1;
		}
		//add new chunk to linked list.

		mdi->end_chunk->next = new_chunk;
		mdi->end_chunk = new_chunk;
		mdi->end_offset = 0;

		data_ptr = (char*)mdi->end_chunk->mem_pointer;

		if (free_space <= 2 * sizeof(unsigned short)) {
			if (list_item->start_chunk == NULL ) {
				list_item->start_chunk = mdi->end_chunk;
				list_item->item_data_start_offset = mdi->end_offset;
			}
			#ifdef	PC
				*data_ptr = type_tmp & 0x00ff;
				data_ptr++;
				*data_ptr = type_tmp >> 8;	
				data_ptr++;

				*data_ptr = len_tmp & 0x00ff;
				data_ptr++;
				*data_ptr = len_tmp >> 8;
				data_ptr++;	
			#else
				*data_ptr = type_tmp >> 8;	
				data_ptr++;
				*data_ptr = type_tmp & 0x00ff;
				data_ptr++;

				*data_ptr = len_tmp >> 8;
				data_ptr++;
				*data_ptr = len_tmp & 0x00ff;
				data_ptr++;
			#endif
			mdi->end_offset += 2 * sizeof(unsigned short);			
		}

		if (len > menu_data_instance_get_free_space(mdi)) {
			DEBUG_ASSERT(1==2,"you wanted to consume more than one chunk WHY???");
			return -1;
		}

		memcpy(data_ptr, data, len);
		mdi->end_offset += len;

		list_item->end_chunk = mdi->end_chunk;
		list_item->item_data_end_offset = mdi->end_offset;
		//menu_data_instance_dump_chunk_memory(mdi->end_chunk);
		return 1;
		
	}

	DEBUG_TRACE("add metadata p=%p len=%d type=%d",data_ptr, len , type);

#ifdef	PC
	*data_ptr = type_tmp & 0x00ff;
	data_ptr++;
	*data_ptr = type_tmp >> 8;	
	data_ptr++;

	*data_ptr = len_tmp & 0x00ff;
	data_ptr++;
	*data_ptr = len_tmp >> 8;
	data_ptr++;	
#else
	*data_ptr = type_tmp >> 8;	
	data_ptr++;
	*data_ptr = type_tmp & 0x00ff;
	data_ptr++;

	*data_ptr = len_tmp >> 8;
	data_ptr++;
	*data_ptr = len_tmp & 0x00ff;
	data_ptr++;

#endif
	memcpy(data_ptr, data, len);

	//update list_item values
	if (list_item->start_chunk == NULL ) {
		list_item->start_chunk = mdi->end_chunk;
		list_item->item_data_start_offset = mdi->end_offset;
	}	

	mdi->end_offset += len + 2 * sizeof(unsigned short) ;	

	list_item->end_chunk = mdi->end_chunk;
	list_item->item_data_end_offset = mdi->end_offset;
	DEBUG_TRACE("added new element:");
	DEBUG_TRACE("start_chunk: %p, start_offset:%x", list_item->start_chunk, list_item->item_data_start_offset);
	DEBUG_TRACE("end_chunk: %p, end_offset:%x", list_item->end_chunk, list_item->item_data_end_offset);

	return 1;
}

struct menu_data_instance* menu_data_instance_create()
{
	struct menu_data_instance *mdi = calloc( 1, sizeof(struct menu_data_instance));
	if( mdi == NULL) {
		DEBUG_ERROR("menu_data alloc failed!");
		goto error;
	}

	mdi->metadata_area = calloc( 1, sizeof(struct data_chunk) );
	if( mdi->metadata_area == NULL) {
		DEBUG_ERROR("metadata_area alloc failed!");
		goto error;
	}

	mdi->metadata_area->mem_pointer = calloc( 1, METADATA_AREA_CHUNK_SIZE);
	if( mdi->metadata_area->mem_pointer == NULL) {
		DEBUG_ERROR("mem_pointer alloc failed!");
		goto error;
	}

	mdi->metadata_area->next = NULL;

	mdi->end_chunk = mdi->metadata_area;
	mdi->end_offset = 0;


	mdi->item_list = calloc( 1, sizeof(struct data_chunk) );
	if( mdi->item_list == NULL) {
		DEBUG_ERROR("item_list alloc failed!");
		goto error;
	}

	mdi->item_list->mem_pointer  = calloc(1, ITEM_LIST_CHUNK_SIZE );
	if( mdi->item_list->mem_pointer  == NULL) {
		DEBUG_ERROR("item_list->mem_pointer alloc failed!");
		goto error;
	}

	mdi->item_list->next = NULL;
	mdi->list_item_count = 0;

	mdi->prev = NULL;

	mdi->refs = 1;	

#ifdef DEBUG
	mdi->magic = MENU_MAGIC ;
#endif

	DEBUG_TRACE("%p: allocated instance for mdi\n", mdi);
	return mdi;

error:
	menu_data_instance_destroy(mdi);
	return NULL; 
}

void menu_data_instance_dump_metadata_chunk_memory(struct menu_data_instance *mdi)
{
	struct data_chunk *ptr = mdi->metadata_area;
	unsigned char *block;
	int i,j;

	while (ptr) {
		block = ptr->mem_pointer;
		printf("\n");
		for(i=0 ; i< METADATA_AREA_CHUNK_SIZE/8 ; i++){
			printf(" | ");

			for(j=0 ; j< 8 ; j++)
				printf("%2x ",*(block++));

			printf("| ");
			block -=8 ;

			for(j=0 ; j< 8 ; j++) {
				printf("%c ",(*(block) > 32 && *(block) < 127  ? *(block) : '.' ));
				block++;		
			}
			printf("|\n");
		}
		ptr = ptr->next;		
	}
}

static void menu_data_instance_dump_chunk_memory(struct data_chunk *ptr)
{
	unsigned char *block = ptr->mem_pointer;
	int i,j;
	
	printf("\n");
	for(i=0 ; i< METADATA_AREA_CHUNK_SIZE/8 ; i++){
		printf(" | ");

		for(j=0 ; j< 8 ; j++)
			printf("%2x ",*(block++));

		printf("| ");
		block -=8 ;

		for(j=0 ; j< 8 ; j++) {
			printf("%c ",(*(block) > 32 && *(block) < 127  ? *(block) : '.' ));
			block++;		
		}
		printf("|\n");
	}
}

void menu_data_instance_dump_listitem_chunk_memory(struct menu_data_instance *mdi)
{
	struct data_chunk *ptr = mdi->item_list;
	unsigned char *block = ptr->mem_pointer;
	int i,j;
	
	printf("\n");
	for(i=0 ; i< 16 ; i++){
		printf(" | ");

		for(j=0 ; j< 8 ; j++)
			printf("%2x ",*(block++));

		printf("| ");
		block -=8 ;

		for(j=0 ; j< 8 ; j++) {
			printf("%c ",(*(block) > 32 && *(block) < 127  ? *(block) : '.' ));
			block++;		
		}
		printf("|\n");
	}

	block = ptr->next->mem_pointer;

	printf("\n");
	for(i=0 ; i< 16 ; i++){
		printf(" | ");

		for(j=0 ; j< 8 ; j++)
			printf("%2x ",*(block++));

		printf("| ");
		block -=8 ;

		for(j=0 ; j< 8 ; j++) {
			printf("%c ",(*(block) > 32 && *(block) < 127  ? *(block) : '.' ));
			block++;		
		}
		printf("|\n");
	}
}

#ifdef TEST
#define TEST_COUNT 100
int main (int argc, char *argv[])
{
	struct menu_data_instance *test_md = menu_data_instance_create();
	if (!test_md) {
		DEBUG_WARN("md alloc failed");
		return NULL;
	}

	char *tmp = strdup("Test");
	struct list_item *list_item = menu_data_instance_list_item_open(test_md);
	menu_data_instance_add_metadata(test_md, list_item, HEADER_TEXT, strlen(tmp)+1,tmp);
	menu_data_instance_list_item_close(test_md, list_item);
	free(tmp);
	
	int i = TEST_COUNT;
			
	while(i--) {
		tmp = strdup("test string 1");
		list_item = menu_data_instance_list_item_open(test_md);
		menu_data_instance_add_metadata(test_md, list_item, LIST_TEXT1, strlen(tmp)+1, tmp);
		free(tmp);
		tmp = strdup("test string 2");
		menu_data_instance_add_metadata(test_md, list_item, LIST_TEXT2, strlen(tmp)+1, tmp);
		menu_data_instance_set_listitem_submenu(list_item,  NULL);
		menu_data_instance_list_item_close(test_md, list_item);
		free(tmp);
		DEBUG_ERROR("add item %d",TEST_COUNT - i);
	}
#if 1
	i = TEST_COUNT -1;//jump over header
			
	while(i--) {
		char *text = menu_data_instance_get_metadata(menu_data_instance_get_list_item(test_md,TEST_COUNT - i+1), LIST_TEXT2);
		if( text != NULL) {
			if ( !strstr(text,"test string 2")){
				DEBUG_ERROR("can not get item in good shape");
				menu_data_instance_dump_metadata_chunk_memory(test_md);
				break;
			}
			//free(text);
		}
		DEBUG_ERROR("check item %d",TEST_COUNT - i);
	}

	i = TEST_COUNT -1;//jump over header
			
	while(i--) {
		char *text = menu_data_instance_get_metadata(menu_data_instance_get_list_item(test_md,TEST_COUNT - i+1), LIST_TEXT2);
		if( text != NULL) {
			if ( !strstr(text,"test string 2")){
				DEBUG_ERROR("can not get item in good shape");
				//menu_data_instance_dump_metadata_chunk_memory(test_md);
				break;
			}
			//free(text);
		}
		DEBUG_ERROR("check item %d",TEST_COUNT - i);
	}

	i = TEST_COUNT -1;
	tmp = strdup("test string 4");
	while(i--) {

		int res = menu_data_instance_update_metadata(menu_data_instance_get_list_item(test_md,TEST_COUNT - i+1), LIST_TEXT2, tmp, strlen(tmp)+1 );
		if( res < 0) {
			DEBUG_ERROR("can not update item %d", TEST_COUNT - i+1);
			menu_data_instance_dump_metadata_chunk_memory(test_md);
			break;
		}
		DEBUG_ERROR("update item %d",TEST_COUNT - i);
	}
	free(tmp);
	menu_data_instance_dump_metadata_chunk_memory(test_md);
#else
	i = TEST_COUNT -1;//jump over header
			
	while(i--) {
		int a = menu_data_instance_find_metadata(menu_data_instance_get_list_item(test_md,TEST_COUNT - i+1), LIST_TEXT3, NULL,NULL,NULL);
		if ( a != -1) {
			DEBUG_ERROR("can not get item in good shape %d",i);
			break;
		}
	}
	DEBUG_ERROR("test end %d",i);
#endif
}
#endif
