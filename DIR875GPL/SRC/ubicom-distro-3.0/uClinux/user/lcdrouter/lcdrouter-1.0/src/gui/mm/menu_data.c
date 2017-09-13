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

#define K (1024)
#define CHUNK_SIZE (4)
#define FREE(x) if( x != NULL ) free(x);
#define ITEM_LIST_CHUNK_SIZE (CHUNK_SIZE * K)
#define METADATA_AREA_CHUNK_SIZE (100 * K * CHUNK_SIZE)

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
		DEBUG_TRACE("Goto next chunk: %p", ptr->next);
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

char* menu_data_instance_get_metadata(struct list_item *list_item, menu_data_types_t type)
{
	DEBUG_TRACE("function call");	
	if(list_item == NULL) {
		DEBUG_ERROR("Null list_item instance");
		return NULL;	
	}

	struct data_chunk *current_chunk = list_item->start_chunk;
	char *data_ptr = (char*)(current_chunk->mem_pointer) + list_item->item_data_start_offset;
	DEBUG_TRACE("data_ptr=%p", data_ptr);
#if 0
	while(current_chunk != list_item->end_chunk) {
		// read whole chunk until find the element
		while (1) {
			unsigned short read_type = *(unsigned short*)data_ptr;
			if(read_type == 0xFFFF) {
				break;
			}
			//jump type
			data_ptr += sizeof(unsigned short);

			if (read_type == type) {
				//jump len
				data_ptr += sizeof(unsigned short);

				return (char *)data_ptr;
			}

			unsigned short len = *(unsigned short *)data_ptr;
			//jump len
			data_ptr += sizeof(unsigned short);
			data_ptr += len;
			
		}
		current_chunk = current_chunk->next;
	}
#endif
	//int current_offset = 0;
	//data_ptr = (char *)(current_chunk->mem_pointer) + current_offset;
	DEBUG_TRACE("mempointer=%p endoffset=%d",list_item->end_chunk->mem_pointer, list_item->item_data_end_offset);
	char *end_ptr = (char *)list_item->end_chunk->mem_pointer + list_item->item_data_end_offset;
	DEBUG_TRACE("end_ptr=%p", end_ptr);
	while (data_ptr < end_ptr) {
		unsigned short read_type ;
		memcpy(&read_type, data_ptr, sizeof(unsigned short));
		if(read_type == 0xFFFF) {
			break;
		}

		//jump type
		data_ptr += sizeof(unsigned short);
		if (read_type == type) {
			//jump len
			data_ptr += sizeof(unsigned short);
			DEBUG_TRACE("item found");	
			return (char *)data_ptr;
		}
		unsigned short len ;
		memcpy(&len, data_ptr, sizeof(unsigned short));
		//jump len
		data_ptr += sizeof(unsigned short);
		data_ptr += len;
	}
	DEBUG_WARN("item not found");
	return NULL;
}

int menu_data_instance_add_metadata(struct menu_data_instance *mdi, struct list_item *list_item, menu_data_types_t type, int len, void *data)
{
	//check assert mdi and list_item
	if(!len) {
		DEBUG_ERROR("element item length is zero!");
		return -1;
	}
#if 0
	if ( mdi->end_offset + len + 2 * sizeof(unsigned short) > METADATA_AREA_CHUNK_SIZE ) {
		DEBUG_TRACE("we need new chunk for metadata!");
		 //we need new chunk and ignore empty bytes from last chunk :)
		unsigned short end_of_chunk = 0xFFFF;
		char *end_ptr = (char *)mdi->end_chunk->mem_pointer + mdi->end_offset;
		memcpy(end_ptr, &end_of_chunk, 2);
		DEBUG_TRACE("FFFF written to add:%p!", end_ptr);
		mdi->end_offset = 0;

		struct data_chunk *last = mdi->metadata_area;
		while(last->next) {
			DEBUG_TRACE("goto next chunk: %p!", last->next);
			last = last->next;
		}

		struct data_chunk *ptr = malloc( sizeof(struct data_chunk) );
		if( ptr == NULL) {
			DEBUG_ERROR("error2");
			return -1;
		}

		ptr->mem_pointer = malloc( METADATA_AREA_CHUNK_SIZE );
		if( ptr->mem_pointer == NULL) {
			DEBUG_ERROR("error3");
			free(ptr);
			return -1;
		}

		last->next = ptr;
		mdi->end_chunk = last->next;
		ptr->next = NULL;
		DEBUG_TRACE("new allocated metadata chunk: %p!", last->next);
	}
#endif
	char *data_ptr = (char*)mdi->end_chunk->mem_pointer;
	data_ptr += mdi->end_offset;

	DEBUG_TRACE("add metadata p=%p len=%d type=%d\n",data_ptr, len , type);

	unsigned short type_tmp = type;
	unsigned short len_tmp = len;
	
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
	unsigned char *block = ptr->mem_pointer;
	int i,j;
	
	printf("\n");
	for(i=0 ; i< 5*8 ; i++){
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
