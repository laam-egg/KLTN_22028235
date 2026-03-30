# EMBER2024 Feature Extractor, C++ implementation

import ctypes
import platform
import os
import pathlib
import threading

SCRIPT_DIR = pathlib.Path(__file__).parent.resolve()

"""
extern "C" {
    EFE_SHARED_EXPORT void* EFE_Init(void);

    EFE_SHARED_EXPORT int EFE_GetNumFeatures(IN void* extractor, OUT int* numFeatures);

    EFE_SHARED_EXPORT int EFE_ExtractFeatures32(
        IN void* extractor,
        IN uint8_t const* data,
        IN int const dataSize,
        OUT float* features,
        IN int const numFeatures
    );

    EFE_SHARED_EXPORT int EFE_ExtractFeatures32FromFileA(
        IN void* extractor,
        IN char const* filePath,
        OUT float* features,
        IN int const numFeatures
    );

    EFE_SHARED_EXPORT int EFE_ExtractFeatures32FromFileW(
        IN void* extractor,
        IN wchar_t const* filePath,
        OUT float* features,
        IN int const numFeatures
    );

    EFE_SHARED_EXPORT void EFE_Cleanup(IN void* extractor);
}
"""

c_void_p = ctypes.c_void_p
c_int = ctypes.c_int
c_uint8_p = ctypes.POINTER(ctypes.c_uint8)
c_float_p = ctypes.POINTER(ctypes.c_float)
c_char_p = ctypes.c_char_p
c_wchar_p = ctypes.c_wchar_p

class FeatureExtractor:
    @staticmethod
    def load_efe():
        if platform.system() == "Windows":
            dll = ctypes.CDLL(SCRIPT_DIR / "efe_shared.dll")
        elif platform.system() == "Linux":
            dll = ctypes.CDLL(SCRIPT_DIR / "libefe_shared.so")
        else:
            raise RuntimeError("Unsupported platform")

        dll.EFE_Init.restype = c_void_p
        dll.EFE_Cleanup.argtypes = [c_void_p]
        dll.EFE_Cleanup.restype = None

        dll.EFE_GetNumFeatures.argtypes = [c_void_p, ctypes.POINTER(c_int)]
        dll.EFE_GetNumFeatures.restype = c_int

        dll.EFE_ExtractFeatures32.argtypes = [
            c_void_p, c_uint8_p, c_int, c_float_p, c_int
        ]
        dll.EFE_ExtractFeatures32.restype = c_int

        dll.EFE_ExtractFeatures32FromFileA.argtypes = [
            c_void_p, c_char_p, c_float_p, c_int
        ]
        dll.EFE_ExtractFeatures32FromFileA.restype = c_int

        dll.EFE_ExtractFeatures32FromFileW.argtypes = [
            c_void_p, c_wchar_p, c_float_p, c_int
        ]
        dll.EFE_ExtractFeatures32FromFileW.restype = c_int

        return dll

    def __init__(self):
        self._dll = self.load_efe()
        self._handle = self._dll.EFE_Init()
        if not self._handle:
            raise RuntimeError("EFE_Init failed")
        self._lock = threading.RLock()
        self._closed = False

    def close(self):
        with self._lock:
            if not self._closed and self._handle:
                self._dll.EFE_Cleanup(self._handle)
                self._handle = None
                self._closed = True

    # ---------- Context manager ----------
    def __enter__(self):
        with self._lock:
            return self

    def __exit__(self, exc_type, exc, tb):
        with self._lock:
            self.close()

    # ---------- Best-effort safety net ----------
    def __del__(self):
        # DO NOT rely on this alone
        try:
            self.close()
        except Exception:
            pass

    def __copy__(self):
        raise TypeError("FeatureExtractor cannot be copied")

    def __deepcopy__(self, memo):
        raise TypeError("FeatureExtractor cannot be deep-copied")

    def num_features(self) -> int:
        with self._lock:
            n = ctypes.c_int()
            rc = self._dll.EFE_GetNumFeatures(self._handle, ctypes.byref(n))
            if rc != 0:
                raise RuntimeError(f"EFE_GetNumFeatures failed ({rc})")
            return n.value

    def extract_from_bytes(self, data: bytes) -> list[float]:
        with self._lock:
            n = self.num_features()
            features = (ctypes.c_float * n)()

            rc = self._dll.EFE_ExtractFeatures32(
                self._handle,
                (ctypes.c_uint8 * len(data)).from_buffer_copy(data),
                len(data),
                features,
                n,
            )
            if rc != 0:
                raise RuntimeError(f"Extract failed ({rc})")

            return list(features)
    
    def extract_from_file(self, path: str | os.PathLike) -> list[float]:
        with self._lock:
            if self._closed:
                raise RuntimeError("FeatureExtractor is closed")
            
            path = os.fspath(path)
            if not os.path.isfile(path):
                raise FileNotFoundError(path)

            n = self.num_features()
            features = (ctypes.c_float * n)()

            system = platform.system()

            if system == "Windows":
                # wchar_t* (UTF-16)
                rc = self._dll.EFE_ExtractFeatures32FromFileW(
                    self._handle,
                    ctypes.c_wchar_p(path),
                    features,
                    n,
                )
            else:
                # char* (UTF-8)
                rc = self._dll.EFE_ExtractFeatures32FromFileA(
                    self._handle,
                    path.encode("utf-8"),
                    features,
                    n,
                )

            if rc != 0:
                raise RuntimeError(f"ExtractFeaturesFromFile failed ({rc})")

            return list(features)
