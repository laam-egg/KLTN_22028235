#ifndef PEFILE_DOS_HEADER_INCLUDED
#define PEFILE_DOS_HEADER_INCLUDED

#include <cstdint>
#include <LIEF/PE.hpp>

#define WORD_t uint16_t
#define LONG_t uint32_t

// https://0xrick.github.io/win-internals/pe3/
extern "C" typedef struct _DOS_HEADER {   // DOS .EXE header
    WORD_t   e_magic;                     // Magic number
    WORD_t   e_cblp;                      // Bytes on last page of file
    WORD_t   e_cp;                        // Pages in file
    WORD_t   e_crlc;                      // Relocations
    WORD_t   e_cparhdr;                   // Size of header in paragraphs
    WORD_t   e_minalloc;                  // Minimum extra paragraphs needed
    WORD_t   e_maxalloc;                  // Maximum extra paragraphs needed
    WORD_t   e_ss;                        // Initial (relative) SS value
    WORD_t   e_sp;                        // Initial SP value
    WORD_t   e_csum;                      // Checksum
    WORD_t   e_ip;                        // Initial IP value
    WORD_t   e_cs;                        // Initial (relative) CS value
    WORD_t   e_lfarlc;                    // File address of relocation table
    WORD_t   e_ovno;                      // Overlay number
    WORD_t   e_res[4];                    // Reserved WORD_ts
    WORD_t   e_oemid;                     // OEM identifier (for e_oeminfo)
    WORD_t   e_oeminfo;                   // OEM information; e_oemid specific
    WORD_t   e_res2[10];                  // Reserved WORD_ts
    LONG_t   e_lfanew;                    // File address of new exe header
} DOS_HEADER;

#define DOS_HEADER_SIZE (sizeof(DOS_HEADER))

static_assert(DOS_HEADER_SIZE == 64);

struct DOSHeader {
    DOS_HEADER fields;

    static DOSHeader fromPEFile(uint8_t const* fileContent, size_t const fileSize);
};

#endif // PEFILE_DOS_HEADER_INCLUDED
