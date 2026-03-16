from typing import Self

class FeatureCategory:
    __slots__ = ("category_name",)

    def __init__(self, category_name: str):
        self.category_name = category_name

    def __repr__(self) -> str:
        return self.category_name

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, FeatureCategory):
            return NotImplemented
        return self.category_name == other.category_name

    def __hash__(self) -> int:
        return hash(self.category_name)

CAT_ENTROPY = FeatureCategory("entropy")

class FeatureLocation:
    def __init__(
        self,
        feature_name: str,
        description: str = "",
        categories: list[FeatureCategory] | None = None,
        path: list['FeatureLocationGroup'] | None = None,
        location: int = -1,
    ):
        self.feature_name = feature_name
        self.description = description
        self.categories = categories or []
        self.path = (path or [])
        self.location = location
    
    def __repr__(self):
        return f"FeatureLocation({repr(self.feature_name)}, {repr(self.description)}, {repr([c.category_name for c in self.categories])}, {repr([g.group_name for g in self.path])}, {repr(self.location)})"
    
    def cat(self, category: FeatureCategory) -> Self:
        self.categories.append(category)
        return self
    
    def desc(self, description: str) -> Self:
        self.description = description
        return self
    
    def get_fqfn(self) -> str:
        """Returns the Fully-Qualified Feature Name (FQFN)"""
        return ";".join(g.group_name for g in self.path) + ";" + self.feature_name

class FeatureLocationGroup:
    def __init__(
        self,
        group_name: str,
        nested: list[Self | FeatureLocation],
        description: str = ""
    ):
        self.group_name = group_name
        self.nested = nested
        self.description = description
    
    def __repr__(self) -> str:
        return f"FeatureLocationGroup({repr(self.group_name)}, {repr(self.nested)}, {repr(self.description)})"
    
    def desc(self, description: str) -> Self:
        self.description = description
        return self









INTERESTING_STRING_REGEXES = [
    ".click",
    "/EmbeddedFile",
    "/FlateDecode",
    "/URI",
    "/bin/",
    "/dev/",
    "/proc/",
    "/tmp/",
    "/usr/",
    "<script",
    "Invoke-Command",
    "Invoke-Expression",
    "Start-process",
    "base64",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/",
    "[13][a-km-zA-HJ-NP-Z1-9]{25,34}",
    "cache",
    "certificate",
    "clipboard",
    "command",
    "connect",
    "cookie",
    "create",
    "crypt",
    "debug",
    "decode",
    "delete",
    "desktop",
    "directory",
    "disk",
    "!This program ",
    "download",
    "\\b(?:[0-9A-Fa-f]{2}[:-]){5}(?:[0-9A-Fa-f]{2})\\b",
    "encode",
    "enum",
    "environment",
    "exit",
    "file",
    "\\bC:/",
    "ftp:",
    "GET /",
    "hidden",
    "hostname",
    "html",
    "HTTP/",
    "http://",
    "https://",
    "install",
    "internet",
    "\\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\b",
    "\\b(?:[A-Fa-f0-9]{1,4}:){7}[A-Fa-f0-9]{1,4}\\b|\\b(?:[A-Fa-f0-9]{1,4}:){1,7}:\\b|\\b:[A-Fa-f0-9]{1,4}(?::[A-Fa-f0-9]{1,4}){1,6}\\b",
    "javascript",
    "keyboard",
    "\\b(?:[0-9A-Fa-f]{2}[:-]){5}(?:[0-9A-Fa-f]{2})\\b",
    "memory",
    "module",
    "mutex",
    "onclick",
    "password",
    "POST /",
    "powershell",
    "privilege",
    "process",
    "\\b(?:KHEY_|KHLM|HKCU)",
    "remote",
    "resource",
    "security",
    "service",
    "shell",
    "snapshot",
    "system",
    "thread",
    "token",
    "\\b(?:http|https|ftp):\\/\\/[a-zA-Z0-9-._~:?#[\\]@!$&'()*+,;=]+",
    "User-Agent",
    "wallet",
    "window",
]

EXPECTED_NUM_INTERESTING_STRING_REGEXES = 77
assert len(INTERESTING_STRING_REGEXES) == EXPECTED_NUM_INTERESTING_STRING_REGEXES, f"Unexpected number of INTERESTING_STRING_REGEXES: {len(INTERESTING_STRING_REGEXES)} (expecting: {EXPECTED_NUM_INTERESTING_STRING_REGEXES})"

GROUP = FeatureLocationGroup
F = FeatureLocation
features: list[FeatureLocationGroup | FeatureLocation] = [
    GROUP(
        "GeneralFileInfo",
        [
            F("size"),
            F("entropy").cat(CAT_ENTROPY),
            F("is_pe"),
            GROUP(
                "start_bytes",
                [ F(str(i)) for i in range(4) ]
            )
        ]
    ),

    GROUP(
        "ByteHistogram",
        [ F(str(i)) for i in range(256) ]
    ),

    GROUP(
        "ByteEntropyHistogram",
        [ F(str(i)).cat(CAT_ENTROPY) for i in range(256) ]
    ),

    GROUP(
        "StringExtractor",
        [
            F("num_strings").desc("Number of interesting strings found, based on some regexes"),
            F("avlength"),
            F("printables").desc("Total number of printable characters"),
            GROUP(
                "printabledist",
                [ F(f"{i} ({repr(chr(i))})") for i in range(0x20, 0x7f+1) ]
            ),
            F("printables_entropy").cat(CAT_ENTROPY),
            GROUP(
                "string_counts",
                [ F(f"{i} {regex}") for i, regex in enumerate(INTERESTING_STRING_REGEXES) ]
            )
        ]
    ),

    GROUP(
        "HeaderFileInfo",
        [
            GROUP(
                "COFF",
                [ F(x) for x in ['timestamp', 'num_sections', 'num_symbols', 'sizeof_optional_header', 'pointer_to_symbol_table', 'machine'] ]
            ),

            GROUP(
                "OPTIONAL",
                [ F(x) for x in [
                    'subsystem',
                    'major_image_version',
                    'minor_image_version',
                    'major_linker_version',
                    'minor_linker_version',
                    'major_operating_system_version',
                    'minor_operating_system_version',
                    'major_subsystem_version',
                    'minor_subsystem_version',
                    'sizeof_code',
                    'sizeof_headers',
                    'sizeof_image',
                    "sizeof_initialized_data",
                    "sizeof_uninitialized_data",
                    "sizeof_stack_reserve",
                    "sizeof_stack_commit",
                    "sizeof_heap_reserve",
                    "sizeof_heap_commit",
                    "address_of_entrypoint",
                    "base_of_code",
                    "image_base",
                    "section_alignment",
                    "checksum",
                    "number_of_rvas_and_sizes"
                ] ]
            ),

            GROUP(
                "COFF",
                [
                    GROUP(
                        "has_characteristics",
                        [ F(x) for x in [
                            "RELOCS_STRIPPED",
                            "EXECUTABLE_IMAGE",
                            "LINE_NUMS_STRIPPED",
                            "LOCAL_SYMS_STRIPPED",
                            "AGGRESIVE_WS_TRIM",
                            "LARGE_ADDRESS_AWARE",
                            "16BIT_MACHINE",
                            "BYTES_REVERSED_LO",
                            "32BIT_MACHINE",
                            "DEBUG_STRIPPED",
                            "REMOVABLE_RUN_FROM_SWAP",
                            "NET_RUN_FROM_SWAP",
                            "SYSTEM",
                            "DLL",
                            "UP_SYSTEM_ONLY",
                            "BYTES_REVERSED_HI",
                        ] ]
                    )
                ]
            ),

            GROUP(
                "OPTIONAL",
                [
                    GROUP(
                        "has_dll_characteristics",
                        [ F(x) for x in [
                            "HIGH_ENTROPY_VA",
                            "DYNAMIC_BASE",
                            "FORCE_INTEGRITY",
                            "NX_COMPAT",
                            "NO_ISOLATION",
                            "NO_SEH",
                            "NO_BIND",
                            "APPCONTAINER",
                            "WDM_DRIVER",
                            "GUARD_CF",
                            "TERMINAL_SERVER_AWARE",
                        ] ]
                    )
                ]
            ),

            GROUP(
                "DOS",
                [ F(x) for x in [
                    "e_magic",
                    "e_cblp",
                    "e_cp",
                    "e_crlc",
                    "e_cparhdr",
                    "e_minalloc",
                    "e_maxalloc",
                    "e_ss",
                    "e_sp",
                    "e_csum",
                    "e_ip",
                    "e_cs",
                    "e_lfarlc",
                    "e_ovno",
                    "e_oemid",
                    "e_oeminfo",
                    "e_lfanew",
                ] ]
            )
        ]
    ),

    GROUP(
        "SectionInfo",
        [
            GROUP(
                "general",
                [
                    *( F(x) for x in [
                    "n_sections",
                    "n_sections_zero_size",
                    "n_sections_empty_name",
                    "n_sections_rx",
                    "n_sections_w",
                    ] ),

                    GROUP(
                        "section_entropy",
                        [ F(x).cat(CAT_ENTROPY) for x in ['max', 'min'] ]
                    ),

                    GROUP(
                        "section_size_ratio",
                        [ F(x) for x in ['max', 'min'] ]
                    ).desc("ratio = section.SizeOfRawData / fileSize"),

                    GROUP(
                        "section_vsize_ratio",
                        [ F(x) for x in ['max', 'min'] ]
                    ).desc("ratio = section.SizeOfRawData / section.Misc_VirtualSize"),
                ],
            ),

            GROUP(
                "section_sizes_hashed",
                [ F(str(x)) for x in range(50) ]
            ),

            GROUP(
                "section_vsizes_hashed",
                [ F(str(x)) for x in range(50) ]
            ),

            GROUP(
                "section_names_entropies_hashed",
                [ F(str(x)) for x in range(50) ]
            ),

            GROUP(
                "section_characteristics_hashed",
                [ F(str(x)) for x in range(50) ]
            ),

            GROUP(
                "entry_section_name_hashed",
                [ F(str(x)) for x in range(10) ]
            ),

            GROUP(
                "overlay",
                [
                    F("size"),
                    F("size_ratio").desc("overlay.size / fileSize"),
                    F("entropy").cat(CAT_ENTROPY),
                ]
            )
        ]
    ),

    GROUP(
        "ImportsInfo",
        [
            F("num_imported_functions"),
            F("num_imported_libraries"),
            GROUP(
                "libraries_names_hashed",
                [ F(str(x)) for x in range(256) ]
            ),

            GROUP(
                "imports_names_hashed",
                [ F(str(x)) for x in range(1024) ]
            )
        ]
    ),

    GROUP(
        "ExportsInfo",
        [
            F("num_exports"),
            GROUP(
                "exports_names_hashed",
                [ F(str(x)) for x in range(128) ]
            )
        ]
    ),

    GROUP(
        "DataDirectories",
        [
            *[
                GROUP(x, [F('size'), F('virtual_address')])
                for x in [
                    "EXPORT",
                    "IMPORT",
                    "RESOURCE",
                    "EXCEPTION",
                    "SECURITY",
                    "BASERELOC",
                    "DEBUG",
                    "COPYRIGHT",
                    "GLOBALPTR",
                    "TLS",
                    "LOAD_CONFIG",
                    "BOUND_IMPORT",
                    "IAT",
                    "DELAY_IMPORT",
                    "COM_DESCRIPTOR",
                    "RESERVED",
                ]
            ],

            F("has_relocs"),
            F("has_dynamic_relocs"),
        ]
    ),

    GROUP(
        "RichHeader",
        [
            F("num_byte_pairs"),
            GROUP(
                "byte_pairs_hashed",
                [ F(str(x)) for x in range(32) ]
            )
        ]
    ),

    GROUP(
        "AuthenticodeSignature",
        [
            F(x) for x in [
                "num_certs",
                "is_any_cert_self_signed",
                "is_program_name_empty",
                "no_countersigner",
                "parse_error",
                "chain_max_depth",
                "latest_signing_time",
                "signing_time_diff"
            ]
        ]
    ),

    GROUP(
        "PEFormatWarnings",
        [
            GROUP(
                "hash",
                [ F(str(x)) for x in range(87) ]
            ),

            F("num_warnings")
        ]
    )
]

def walk_features(
    features: list[FeatureLocationGroup | FeatureLocation],
    feature_list: list[FeatureLocation],
    categories_to_features_map: dict[FeatureCategory, list[FeatureLocation]],
    path: list[FeatureLocationGroup],
):
    for f in features:
        if isinstance(f, FeatureLocation):
            f.location = len(feature_list)
            f.path = [g for g in path]
            # print(f)
            feature_list.append(f)

            for c in f.categories:
                l = categories_to_features_map.get(c, [])
                l.append(f)
                categories_to_features_map[c] = l
        elif isinstance(f, FeatureLocationGroup):
            subpath = [x for x in path]
            subpath.append(f)
            walk_features(f.nested, feature_list, categories_to_features_map, subpath)
        else:
            raise TypeError(f"neither a feature nor a group: {f}")



class FeatureLocationAnalyzer:
    def __init__(
        self,
        features: list[FeatureLocationGroup | FeatureLocation],
        num_features: int|None = None,
    ):
        self.feature_list: list[FeatureLocation] = []
        self.categories_to_features_map: dict[FeatureCategory, list[FeatureLocation]] = {}
        walk_features(features, self.feature_list, self.categories_to_features_map, [])
        if num_features is not None:
            assert len(self.feature_list) == num_features, f"Unexpected number of features: {len(self.feature_list)} (expecting: {num_features})"
    
    def at(self, i: int) -> FeatureLocation:
        return self.feature_list[i]


if __name__ == "__main__":
    analyzer = FeatureLocationAnalyzer(features, 2568)
    for f in analyzer.feature_list:
        print(repr(";".join([*(p.group_name for p in f.path), f.feature_name])) + ",")
