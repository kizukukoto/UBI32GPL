extern int _system(const char *fmt, ...);


#define ROOT_USB_DEV		"storage"
#define	LOGIN_HTM			"login"
#define CATEGORY_HTM		"category_view"
#define FOLDER_HTM			"folder_view"
#define LOGIN_IDLE_TIMEOUT	180	// Idle time out (Seconds)
#define MAX_ENTRIES 		50	// the max number of input items from GUI
#define MAX_NODE_NUM		5
#define MAX_SESSION_NUM		4
#define MAX_USER_NUM		10
#define MAX_NODE_NAME		256
#define MAX_PATH_LEN 		1024
#define MAX_SESSION_LEN		8
#define MAX_USER_NAME_LEN	15		// must match the maxlength of storage_name on GUI
#define MAX_USER_PWD_LEN	15		// must match the maxlength of storage_pwd on GUI
#define MAX_CHALLENGE_LEN	36
#define MAX_HASH_LEN		32
#define MEDIA_INFO_FILE		"/var/tmp/media_info"
#define UID_COUNT_FILE		"/var/tmp/uid_count"
#define UID_INFO_FILE		"/var/tmp/uid_info"
#define WS_LOGIN_INFO_FILE	"/var/tmp/ws_login_info"

#define ENTRY_VALUE_LEN 256
#define WEBFILE_USER_PATH "/var/tmp/run"
#define WEBFILE_STATE_FILE "/var/tmp/web_file"
#define USB_MOUNT_DEV "/www/webfile_access/storage"
#define DEFAULT_WEBFILE_PATH "/www/webfile_access"

#define JSON_ROOT_RESP	"{\"status\": \"%s\", \"errno\": %s, \"rootnode\": ["
#define JSON_ROOT_NODE	"{\"volname\": \"%s\", \"volid\": %d, \"status\": \"%s\", \"mode\": %d, \"nodename\": [%s]}"

#define JSON_LIST_FILE_RESP	"{\"status\": \"%s\", \"errno\": %s, \"count\": %d, \"files\": ["
#define JSON_FILE_NODE		"{\"name\": \"%s\", \"type\": %d, \"size\": %ld, \"mode\": %d, \"mtime\": %d, \"desp\": \"%s\", \"thumb\": %d}"
#define JSON_CATEGORY_NODE	"{\"name\": \"%s\", \"volid\": \"%d\", \"path\": \"%s\", \"size\": %ld, \"mode\": %d, \"mtime\": %d, \"desp\": \"%s\", \"thumb\": %d}"

#define JSON_COMM_RESP	"{\"status\": \"%s\", \"errno\": %s}"

#define JSON_UPLOAD_FILE_RESP	"{\"status\": \"%s\", \"errno\": %s, \"size\": %d}"

#define JSON_LOGIN_RESP1	"{\"status\": \"%s\", \"errno\": %s, \"uid\": \"%s\", \"challenge\": \"%s\"}"
#define JSON_LOGIN_RESP2	"{\"status\": \"%s\", \"errno\": %s, \"id\": \"%s\", \"key\": \"%s\"}"

#define PERM_READ_ONLY	"ro"
#define PERM_READ_WRITE	"rw"

typedef struct{		
    char name[MAX_USER_NAME_LEN+1];   
} login_account;

typedef struct{		
	char uid[MAX_SESSION_LEN+1];
	unsigned long login_time;
}uid_info;

typedef struct{		
	char path[MAX_PATH_LEN];
	char permission[3];		// ro: Read Only, rw: Read/Write
}path_info;

typedef struct{	
	int level;		// 0: admin, 1: Guest, 2: other users
	int path_num;
	uid_info session_uid[MAX_SESSION_NUM];
	path_info access_path[MAX_NODE_NUM];	
}login_user;

typedef struct{	
	char uid[MAX_SESSION_LEN+1];
	char challenge[MAX_CHALLENGE_LEN+1];
	char user_name[MAX_USER_NAME_LEN+1];
	char user_pwd[MAX_USER_PWD_LEN+1];
	int level;		// 0: admin, 1: Guest, 2: other users
	unsigned long login_time;
	int path_num;
	path_info access_path[MAX_NODE_NUM];
}storage_user;

typedef struct{	
	char volname[MAX_NODE_NAME];	
	char volid;
	char status[5];	// exist: ok, not exist: fail
	char permission[3];
	char nodename[MAX_NODE_NAME];
}node_info;

typedef struct{	
	char status[5];	// exist: ok, not exist: fail
	char error_code[5];
	int num;
	node_info node[MAX_NODE_NUM];
}root_node;

/* status definitions */
#define WS_STATUS1	"ok"
#define WS_STATUS2	"fail"

/* errno definitions */
#define WS_ERRNO0	"null"
#define WS_ERRNO1	"5001"	// Unexpected server error
#define WS_ERRNO2	"5002" 	// No devices
#define WS_ERRNO3	"5003"	// Authentication fail
#define WS_ERRNO4	"5004"	// Path doesn't exist
#define WS_ERRNO5	"5005"	// File doesn't exist
#define WS_ERRNO6	"5006"	// Lack of required parameters
#define WS_ERRNO7	"5007"	// The method is not implemented
#define WS_ERRNO8	"5008"	// Disk full
#define WS_ERRNO9	"5009"	// Destination file already exists
#define WS_ERRNO10	"5010"	// Read-only file system
#define WS_ERRNO11	"5101"	// Request expired
#define WS_ERRNO12	"5102"	// Permission denied
#define WS_ERRNO13	"5103"	// Invalid user
#define WS_ERRNO14	"5104"	// Invalid application ID

/* sort order definitions */
#define WS_SORT_INCR	0
#define WS_SORT_DECR	1

enum{
	IS_FILE,
	IS_FOLDER,	
	IS_LINK
};

enum{
	ALL_FILES,
	AUDIO_ONLY,
	VIDEO_ONLY,
	IMAGES_ONLY,
	DOC_ONLY
};

enum{
	NOT_ALLOW,
	READ_ONLY,
	READ_WRITE
};

typedef struct { 	
	char type;
	char media_type;
	char path[MAX_PATH_LEN];
	char name[MAX_NODE_NAME];
	char permission;	// 0: not allow to access, 1: read only, 2: read/write
	long size;
	long mtime;
} media_file;

/* database definitions */


#define DB_NAME_LEN		31
#define DB_VALUE_LEN	600
typedef struct {
    char name[DB_NAME_LEN+1];
    char value[DB_VALUE_LEN+1];    
} ware_entry;

#define cprintf(fmt, args...) do { \
        FILE *cfp = fopen("/dev/console", "w"); \
        if (cfp) { \
                fprintf(cfp, fmt, ## args); \
                fclose(cfp); \
        } \
} while (0)

extern int filter_all(const struct dirent *dir);
extern int filter_folder(const struct dirent *dir);
extern int filter_file(const struct dirent *dir);