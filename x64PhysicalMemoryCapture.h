


//Errors and Success

#define FILE_OPEN_SUCCESS_x64 0
#define MEMORY_DUMP_SUCCESSFUL 0

#define DRIVER_LOAD_SUCCESS 0
#define MAP_SUCCESS 0
#define ERR_NO_ADMIN_PRIVILEGE_x64 -1
#define ERR_PATH_NOT_FOUND_x64 -2
#define ERR_SC_OPEN_x64 -3
#define ERR_CREATE_SVC_x64 -4
#define ERR_START_SVC_x64 -5
#define ERR_INVALID_HANDLE_x64 -6
#define ERR_DIO_CTL_x64 -7
#define ERR_PHYSMEMFILE_x64 -8
#define ERR_NOTENOUGHSPACE_x64 -9
#define ERR_WRITEVIEWFAILED_x64 -10
#define ERR_NOHANDLE_x64 -11
#define ERR_LOADING_DRIVER_x64 -12
#define ERR_MAP_x64 -13

//Driver related

#define DRIVER_NAME L"WSTMemCap"
#define DRIVER_DISPLAYNAME L"Wetstone x64 memory capture driver"
#define PHYSMEMFILE_NAME L"TriageResults\\physmem"
//02/18/2011 RJM -- Multiple memory file capture change
#define GB3 2.0*1024*1024*1024

#define VAR_DECLARE 
#define GET_MEMORY_HANDLE CTL_CODE(0x8444,0x801,0,FILE_READ_ACCESS)



//REMOVE -- put it back to 128 if it runs successfully
#define SPEEDUP 128

struct os_memory_range
{
	LARGE_INTEGER start;
	LARGE_INTEGER end;
	LARGE_INTEGER size;
	int type;
};


//We need to release memory for both the physical_memory_accessible and the chunk_count_array
struct chunk_count_struct
{
	LARGE_INTEGER start;
	LARGE_INTEGER end;
	LARGE_INTEGER size;
	int type;
	int chunk_count;
	ULONGLONG partial;
};

//8190 * 512 * 1024
#define FOURGBLIMIT 4293918720