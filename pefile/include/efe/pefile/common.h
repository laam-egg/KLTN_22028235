#ifndef PEFILE_COMMON_INCLUDED
#define PEFILE_COMMON_INCLUDED

/**
 * This will set a maximum length of a string to be retrieved from the file.
 * It's there to prevent loading massive amounts of data from memory mapped
 * files. Strings longer than 1MB (0x100000) should be rather rare.
 */
#define MAX_STRING_LENGTH 0x100000 // 2^20

/**
 * Maximum number of imports to parse.
 */
#define MAX_IMPORT_SYMBOLS 0x2000

/**
 * Limit maximum length for specific string types separately:
 * Maximum length for an import name.
 */
#define MAX_IMPORT_NAME_LENGTH 0x200

/**
 * Limit maximum length for specific string types separately:
 * Maximum length for a DLL name.
 */
#define MAX_DLL_LENGTH 0x200

/**
 * Limit maximum length for specific string types separately:
 * Maximum length for a symbol name.
 */
#define MAX_SYMBOL_NAME_LENGTH 0x200

/**
 * Limit maximum number of sections before processing of sections will stop.
 */
#define MAX_SECTIONS 0x800

/**
 * The global maximum number of resource entries to parse per file.
 */
#define MAX_RESOURCE_ENTRIES 0x8000

/**
 * The maximum depth of nested resource tables.
 */
#define MAX_RESOURCE_DEPTH 32

/**
 * Limit number of exported symbols.
 */
#define MAX_SYMBOL_EXPORT_COUNT 0x2000

// --- Signature Constants ---

/**
 * MS-DOS executable signature 'MZ'.
 */
#define IMAGE_DOS_SIGNATURE 0x5A4D

/**
 * Alternate MS-DOS executable signature 'ZM'.
 */
#define IMAGE_DOSZM_SIGNATURE 0x4D5A

/**
 * 16-bit New Executable signature 'NE'.
 */
#define IMAGE_NE_SIGNATURE 0x454E

/**
 * Linear Executable signature 'LE'.
 */
#define IMAGE_LE_SIGNATURE 0x454C

/**
 * Linear Executable (386) signature 'LX'.
 */
#define IMAGE_LX_SIGNATURE 0x584C

/**
 * Terse Executables signature 'VZ'.
 */
#define IMAGE_TE_SIGNATURE 0x5A56

/**
 * NT (PE) file signature 'PE\0\0'.
 */
#define IMAGE_NT_SIGNATURE 0x00004550

/**
 * Number of directory entries in the Optional Header.
 */
#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

/**
 * Flag to indicate an import is by ordinal (32-bit).
 */
#define IMAGE_ORDINAL_FLAG 0x80000000

/**
 * Flag to indicate an import is by ordinal (64-bit).
 */
#define IMAGE_ORDINAL_FLAG64 0x8000000000000000ULL

/**
 * Magic number for a standard PE32 Optional Header.
 */
#define OPTIONAL_HEADER_MAGIC_PE 0x10B

/**
 * Magic number for a PE32+ (64-bit) Optional Header.
 */
#define OPTIONAL_HEADER_MAGIC_PE_PLUS 0x20B

#endif // PEFILE_COMMON_INCLUDED
