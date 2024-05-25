/*___________________________________________________________________________________________________

Title:
	fat.h v2.0

Description:
	File system driver for FAT16/32
	
	For complete details visit:
	https://www.programming-electronics-diy.xyz/2022/07/sd-memory-card-library-for-avr.html

Author:
 	Liviu Istrate
	istrateliviu24@yahoo.com
	www.programming-electronics-diy.xyz

Donate:
	Software development takes time and effort so if you find this useful consider a small donation at:
	https://www.paypal.com/donate/?hosted_button_id=L5ZL3SAN6NABY
_____________________________________________________________________________________________________*/


/* ----------------------------- LICENSE - GNU GPL v3 -----------------------------------------------

* This license must be included in any redistribution.

* Copyright (C) 2022 Liviu Istrate, www.programming-electronics-diy.xyz (istrateliviu24@yahoo.com)

* Project URL: https://www.programming-electronics-diy.xyz/2022/07/sd-memory-card-library-for-avr.html

CAUTION:
	Although each functionality has been tested, it is very difficult to test each combination of functions.
	I am not responsible for any data loss that might occur using this library. The user should first 
	test the proper code functionality according to their usage before deploying the code.

* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program. If not, see <https://www.gnu.org/licenses/>.

--------------------------------- END OF LICENSE --------------------------------------------------*/

/*__________________________________CHANGELOG________________________________________________________

v2.0
	04-02-2024:
	- Included support for modern UPDI AVR microcontrollers.

v1.3
	- Added two functions for file deletion: fdelete() and fdeleteByName().
	- Made some code optimizations

v1.2
	- Fixed a bug with findNext() that didn't start from bginning of the directory when dirCountItems()
	was used previously.
	- Fixed some issues with buf.Long by initializing it to 0 and clearing it with 0 after use
	- Fixed some issues by resetting some variables after some functions made use of them

v1.1
	- Added a conditional include for the bool type that caused a compiler error since
	the utils.h was updated to include the sdtbool.h
_____________________________________________________________________________________________________*/

#ifndef FAT_H_
#define FAT_H_
#ifdef __cplusplus
extern "C" {
#endif
/*************************************************************
	INCLUDES
**************************************************************/
#include <string.h>
#include "utils.h"
#include "sd.h"

/*************************************************************
	USER DEFINED SETTINGS
**************************************************************/
#define FAT_SUPPORT_FAT32			1	// set to 0 to support only FAT16 and exclude FAT32 to save space

// FAT supports file names up to 260 characters including path 
// but that would take a lot of space so
// shorter file names could be used instead
#define FAT_MAX_FILENAME_LENGTH		255

/*************************************************************
	SYSTEM DEFINES
**************************************************************/
#define FAT_TASK_SEARCH_SFN			1
#define FAT_TASK_TABLE_COUNT_FREE	2
#define FAT_TASK_TABLE_FIND_FREE	3
#define FAT_TASK_MKDIR				4
#define FAT_TASK_MKFILE				5
#define FAT_TASK_TABLE_SET			6
#define FAT_TASK_TABLE_GET_NEXT		7
#define FAT_TASK_OPEN_DIR			8
#define FAT_TASK_COUNT_ITEMS		9
#define FAT_TASK_FIND_FILE			10
#define FAT_TASK_SET_FILESIZE		11
#define FAT_TASK_SET_START_CLUSTER	12
#define FAT_TASK_TABLE_REMOVE_CHAIN	13
#define FAT_TASK_MARK_DELETED		14

// Directory entry
#define FAT_DIR_FREE_SLOT						0x00	// root directory entry is free
#define FAT_FILE_DELETED						0xE5	// the filename has been used, but the file has been deleted
#define FAT_FILE_E5_CHAR						0x05	// the first character of the filename is actually 0xE5
#define FAT_LFN_MAX_CHARS						13		// a LFN entry can hold 13 characters

// FAT Table
#define FAT16_FREE_CLUSTER						0x0000		// the cluster is free
#define FAT16_BAD_CLUSTER						0xFFF7		// indicates a bad (defective) cluster
#define FAT16_EOF_CLUSTER						0xFFFF		// cluster is allocated and is the final cluster for the file (indicates end-of-file)

#define FAT32_FREE_CLUSTER						0x0000000	// the cluster is free
#define FAT32_BAD_CLUSTER						0xFFFFFF7	// indicates a bad (defective) cluster
#define FAT32_EOF_CLUSTER						0x0FFFFFFF	// cluster is allocated and is the final cluster for the file (indicates end-of-file)
// NOTE: No FAT32 volume should ever be configured containing cluster numbers available for allocation >= 0xFFFFFF7.
#define FAT32_MAX_CLUSTER						0xFFFFFF7

// The maximum Windows filename length to the operating system is 260 characters, 
// however that includes a number of required characters that lower the effective number.
// From the 260, you must allow room for the following: 
// Drive letter, Colon after drive letter, Backslash after drive letter, End-of-Line character,
// Backslashes that are part of the filename path (e.g. c:\dir-name\dir-name\filename).
// So, that takes the 260 down to 256 characters as an absolute maximum.
// Absolute (relative) maximum file length - including path - is 256 characters.
#define FS_MAX_PATH					260 - 4 - 1 - 8 // subtract 1 so max_length is 255 to fit in 1 byte minus 8 for drive label

/* File system type (FATFS.fs_type) */
#define FS_FAT12	0
#define FS_FAT16	1 // must be 1 so bit shifting << will multiply by 2
#define FS_FAT32	2 // must be 2 so bit shifting << will multiply by 4
#define FS_EXFAT	3

/* Offsets */
/* MBR */
#define FAT_MBR_PARTITION_ENTRY_OFFSET					0x1BE	// offset at which partition information starts
#define FAT_MBR_PARTITION_TYPE							0x04	// partition type offset relative to each 16-bit partition slot
#define FAT_MBR_PARTITION_START							0x08	// relative offset to the partition in sectors (LBA)
#define FAT_MBR_PARTITION_SIZE							0x0C	// size of the partition in sectors

/* Boot Record */
#define FAT_BPB_BYTES_PER_SECTOR						0x0B	// sector size in bytes
#define FAT_BPB_SECTORS_PER_CLUSTER						0x0D	// number of sectors per cluster
#define FAT_BPB_RESERVED_SECTORS						0x0E	// reserved sectors (including the boot sector)
#define FAT_BPB_NR_OF_FATS								0x10	// number of FATs
#define FAT_BPB_ROOT_DIR_ENTRIES						0x11	// number of directory entries in the root directory (N.A. for FAT32)
#define FAT_BPB_TOT_SEC_16								0x13	// total number of sectors on the disk/partition
#define FAT_BPB_FAT_SZ_16								0x16	// number of sectors occupied by a FAT (N.A. for FAT32)
#define FAT_BPB_TOT_SEC_32								0x20	// total number of sectors of the FAT volume in new 32-bit field

#define FAT16_BS_VOL_ID									39		// volume serial number (FAT12/16)

#define FAT32_BPB_FAT_SZ_32								36		// this field is the FAT32 32-bit count of sectors occupied by one FAT
#define FAT32_BPB_EXT_FLAFS								40		// indicates how many FATs are used and if only 1 which one
#define FAT32_BPB_FS_VER								42		// the version number of the FAT32 volume
#define FAT32_BPB_ROOT_CLUST							44		// cluster number of the first cluster of the root directory
#define FAT32_BPB_FS_INFO								48		// sector number of FSINFO structure in the reserved area of the FAT32 volume. Usually 1
#define FAT32_BPB_BK_BOOT_SECTOR						50		// if non-zero, indicates the sector number in the reserved area of the volume of a copy of the boot record
#define FAT32_BPB_RESERVED								52		// must be set to 0x0
#define FAT32_BS_DRV_NUM								64		// set value to 0x80 or 0x00
#define FAT32_BS_RESERVED1								65		// must be set to 0x0
#define FAT32_BS_BOOT_SIG								66		// extended boot signature
#define FAT32_BS_VOL_ID									67		// volume serial number (FAT32)
#define FAT32_BS_VOL_LABEL								71		// volume label (FAT32)
#define FAT32_BS_FS_TYPE								82		// set to the string:"FAT32 "

/* Directory Entry */
#define FAT_DIR_NAME									0x00
#define FAT_DIR_ATTR									11
#define FAT_DIR_NT_RES									12
#define FAT_DIR_CREAT_TIME_MILLIS						13
#define FAT_DIR_CREAT_TIME								14
#define FAT_DIR_CREAT_DATE								16
#define FAT_DIR_LAST_ACC_DATE							18
#define FAT_DIR_FIRST_CLUS_HIGH							20
#define FAT_DIR_WRITE_TIME								22
#define FAT_DIR_WRITE_DATE								24
#define FAT_DIR_FIRST_CLUS_LOW							26
#define FAT_DIR_FILE_SIZE								28

/* File attributes Offsets and Masks */
#define FAT_FILE_ATTR_READ_ONLY							0x01
#define FAT_FILE_ATTR_HIDDEN							0x02
#define FAT_FILE_ATTR_SYSTEM							0x04
#define FAT_FILE_ATTR_VOLUME_ID							0x08
#define FAT_FILE_ATTR_DIRECTORY							0x10
#define FAT_FILE_ATTR_ARCHIVE							0x20
#define FAT_FILE_ATTR_LONG_NAME							(FAT_FILE_ATTR_READ_ONLY | FAT_FILE_ATTR_HIDDEN | FAT_FILE_ATTR_SYSTEM | FAT_FILE_ATTR_VOLUME_ID)
#define FAT_FILE_ATTR_LONG_NAME_MASK					(FAT_FILE_ATTR_READ_ONLY | FAT_FILE_ATTR_HIDDEN | FAT_FILE_ATTR_SYSTEM |FAT_FILE_ATTR_VOLUME_ID | FAT_FILE_ATTR_DIRECTORY | FAT_FILE_ATTR_ARCHIVE)

/* FAT Long Directory Entry Structure Offsets and Masks */													
#define FAT_LONG_DIR_ORDER								0		// the order of this entry in the sequence of long dir entries
#define FAT_LAST_LONG_ENTRY_MASK						0x40
#define FAT_LONG_DIR_NAME								1		// characters 1-5 of the long-name sub-component in this dir entry
#define FAT_LONG_DIR_ATTR								11		// attributes - must be ATTR_LONG_NAME
#define FAT_LONG_DIR_TYPE								12		// if zero, indicates a directory entry that is a sub-component of a long name
#define FAT_LONG_DIR_CHECKSUM							13		// checksum of name in the short dir entry at the end of the long dir set
#define FAT_LONG_DIR_NAME2								14		// characters 6-11 of the long-name sub-component in this dir entry
#define FAT_LONG_DIR_FIRST_CLST_LOW						26		// must be ZERO. This is an artifact of the FAT "first cluster"
#define FAT_LONG_DIR_NAME3								28		// characters 12-13 of the long-name sub-component in this dir entry


/* Function pointers to low lever card interface functions */
//uint8_t (*card_read_single_block) (uint32_t addr, uint8_t *buf, uint8_t *token);

/*************************************************************
	TYPE DEFINITIONS
**************************************************************/
typedef uint32_t FSIZE_t;			// file size in bytes
typedef uint32_t SECTSIZE_t;		// sectors

#if FAT_SUPPORT_FAT32 == 0
	typedef uint16_t CLSTSIZE_t;	// clusters
#else
	typedef uint32_t CLSTSIZE_t;	// clusters
#endif

// Access a long variable as 4, 8-bit integer values
typedef union lng {
	uint32_t Long;
	uint8_t Int[4];
} lng;


/*************************************************************
	ENUMERATORS
**************************************************************/
/* Volume mounting return codes (MOUNT_RESULT) */
typedef enum {
	MR_OK = 0,				// (0) Succeeded
	MR_DEVICE_INIT_FAIL,	// (1) An error occurred during device initialization
	MR_ERR,					// (2) An error occurred in the low level disk I/O layer
	MR_NO_PARTITION,		// (3) No partition has been found
	MR_FAT_ERR,				// (4) General FAT error
	MR_UNSUPPORTED_FS,		// (5) Unsupported file system
	MR_UNSUPPORTED_BS		// (6) Unsupported block size
} MOUNT_RESULT;


/* File function return codes (FRESULT) */
typedef enum {
	FR_OK = 0,				// (0) Succeeded
	FR_EOF,					// (1) End of file
	FR_NOT_FOUND,			// (2) Could not find the file
	FR_NO_PATH,				// (3) Could not find the path
	FR_NO_SPACE,			// (4) Not enough space to create file
	FR_EXIST,				// (5) File exists
	FR_INCORRECT_ENTRY,		// (6) Some entry parameters are incorrect
	FR_DENIED,				// (7) Access denied due to prohibited access or directory full
	FR_PATH_LENGTH_EXCEEDED,// (8) Path too long
	FR_NOT_A_DIRECTORY,		// (9)
	FR_ROOT_DIR,			// (10) When going back the directory path this is returned when the active dir is root
	FR_INDEX_OUT_OF_RANGE,	// (11)
	FR_DEVICE_ERR,			// (12) A hard error occurred in the low level disk I/O layer
	FR_READ_ONLY_FILE,		// (13) File is read-only and cannot be deleted
	FR_INCORRECT_CHECKSUM,	// (14)
	FR_UNKNOWN_ERROR		// (15)
} FRESULT;


/*************************************************************
	STRUCTURES
**************************************************************/
/* File system object structure (FAT) */
typedef struct {
	uint32_t	fs_partition_offset;	// relative offset to the partition in sectors (LBA)
	uint32_t	BPB_TotSec32;			// total number of sectors of the FAT volume in new 32-bit field
	uint32_t	EOC;					// End Of Chain cluster value
	uint32_t	FATSz;					// number of sectors for one FAT table
	uint32_t	RootFirstCluster;		// first cluster of the root directory (FAT32 only)
	CLSTSIZE_t	CountofClusters;		// count of clusters
	uint16_t	BPB_BytsPerSec;			// sector size in bytes 
	uint16_t	RootDirSectors;			// count of sectors occupied by the root directory (N.A for FAT32)
	uint16_t	RootFirstSector;		// first sector of the root	directory
	uint16_t	FirstDataSector;		// this sector number is relative to the first sector of the volume that contains the BPB
	uint16_t	Fat1StartSector;
	uint16_t	Fat2StartSector;
	uint8_t		FATDataSize;			// 2-bytes if FAT16, 4-bytes if FAT32
	uint8_t		entries_per_sector;		// number of entries in a sector given a 32 bytes entry. For a 512 bytes per sector: 512 / 32 = 16	
	uint8_t		fs_low_level_code;		// low level status code from card controller
	uint8_t		fs_type;				// file system type (0:not mounted)
	uint8_t		BPB_SecPerClus;			// sectors per cluster
} FAT;


/* Directory object structure (DIR) */
typedef struct {
	CLSTSIZE_t	dir_start_cluster;
	CLSTSIZE_t	dir_active_cluster;
	SECTSIZE_t	dir_start_sector;
	const char*	ptr_path_buff;
	uint16_t	dir_nr_of_entries;			// number of files and folders inside a directory
	uint16_t	dir_active_sector;			// max 2^16 sectors per cluster
	uint16_t	dir_active_item;			// index of selected item inside directory starting from 1
	uint16_t	dir_nr_of_items;
	uint16_t	find_by_index;
	uint8_t		dir_entry_offset;			// entry offset inside the sector of selected item
	uint8_t		filename_length;
	bool		dir_open;
	bool		dir_open_by_idx;
} DIR;



/* File information structure (FILE) */
typedef struct {
	FSIZE_t		fptr;										// file read/write pointer (zeroed on file open)
	FSIZE_t		file_size;									// file size in bytes
	CLSTSIZE_t	file_start_cluster;							// entry's first cluster number
	CLSTSIZE_t	file_active_cluster;
	SECTSIZE_t	entry_start_sector;							// sector number that holds the file entry
	//uint16_t	file_last_access_date;						// this is the date of last read or write
	uint16_t	file_write_time;							// time of last write. Note that file creation is considered a write
	uint16_t	file_write_date;							// date of last write. Note that file creation is considered a write
	uint32_t	file_start_sector;
	uint16_t	file_active_sector;							// incremented after each sector read
	uint16_t	buffer_idx;
	uint8_t		entry_offset;								// offset inside the sector where the entry starts
	uint8_t		file_err;									// abort flag (error code)
	uint8_t		file_attrib;								// file attribute
	bool		file_update_size;
	bool		file_open;
	bool		w_sec_changed;								// write sector changed
	bool		eof;
} F_FILE;

#define FILE F_FILE
/*************************************************************
	MACRO FUNCTIONS
**************************************************************/
/* Character code support macros */
#define IsUpper(c)		((c) >= 'A' && (c) <= 'Z')
#define IsLower(c)		((c) >= 'a' && (c) <= 'z')
#define IsDigit(c)		((c) >= '0' && (c) <= '9')
#define IsSeparator(c)	((c) == '/' || (c) == '\\')


/*************************************************************
	FUNCTION PROTOTYPES
**************************************************************/
// Volume
MOUNT_RESULT FAT_mountVolume(sdmmc_card_t* _card);
uint64_t FAT_volumeFreeSpace(void);
uint64_t FAT_volumeCapacity(void);
float FAT_volumeCapacityKB(void);
float FAT_volumeCapacityMB(void);
float FAT_volumeCapacityGB(void);
FRESULT FAT_getLabel(char* label, uint32_t* vol_sn);

// Other
uint8_t FAT_createTimeMilli(void);
uint16_t FAT_createTime(void);
uint16_t FAT_createDate(void);

// Directory
FRESULT FAT_makeDir(DIR* dir_obj, const char* path);
FRESULT FAT_openDir(DIR* dir_p, const char* path);
FRESULT FAT_openDirByIndex(DIR* dir_p, FILE* finfo_p, uint16_t idx);
FRESULT FAT_dirBack(DIR* dir_p);
FRESULT FAT_findByIndex(DIR* dir_p, FILE* finfo_p, uint16_t idx);
FRESULT FAT_findNext(DIR* dir_p, FILE* finfo_p);
uint16_t FAT_dirCountItems(DIR* dir_p);

// File
FRESULT FAT_makeFile(DIR* dir_obj, const char* path);
FRESULT FAT_fwriteFloat(FILE* fp, float nr, uint8_t nrOfDecimals);
FRESULT FAT_fwriteInt(FILE* fp, INT_SIZE nr);
FRESULT FAT_fwriteString(FILE* fp, const char* string);
FRESULT FAT_fwrite(FILE* fp, const void* buff, uint16_t btw, uint16_t* bw);
FRESULT FAT_ftruncate(FILE* fp);
FRESULT FAT_fdelete(DIR* dir_p, uint16_t idx);
FRESULT FAT_fdeleteByName(DIR* dir_p, const char* fname);
FRESULT FAT_fsync(FILE* fp);
FRESULT FAT_fopen(DIR* dir_p, FILE* file_p, char *file_name);
FRESULT FAT_fopenByIndex(DIR* dir_p, FILE* file_p, uint16_t idx);

uint8_t* FAT_fread(FILE* file_p);
FSIZE_t FAT_getFptr(FILE* fp);
void FAT_fseekEnd(FILE* fp);
void FAT_fseek(FILE* fp, FSIZE_t fptr);
bool FAT_feof(FILE* fp);
uint8_t FAT_ferror(FILE* fp);
void FAT_fclear_error(FILE* fp);
char* FAT_getFilename(void);
uint16_t FAT_getItemIndex(DIR* dir_p);
FSIZE_t FAT_getFileSize(FILE* finfo_p);
uint16_t FAT_getWriteYear(FILE* finfo_p);
uint8_t FAT_getWriteMonth(FILE* finfo_p);
uint8_t FAT_getWriteDay(FILE* finfo_p);
uint8_t FAT_getWriteHour(FILE* finfo_p);
uint8_t FAT_getWriteMinute(FILE* finfo_p);
uint8_t FAT_getWriteSecond(FILE* finfo_p);
bool FAT_attrIsFolder(FILE* finfo_p);
bool FAT_attrIsFile(FILE* finfo_p);
bool FAT_attrIsHidden(FILE* finfo_p);
bool FAT_attrIsSystem(FILE* finfo_p);
bool FAT_attrIsReadOnly(FILE* finfo_p);
bool FAT_attrIsArchive(FILE* finfo_p);

uint8_t FAT_read_file_sector(FILE * file_p, unsigned int sector, char * buffer);

#ifdef __cplusplus
}
#endif
#endif /* FAT_H_ */