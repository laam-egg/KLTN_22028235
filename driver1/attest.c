#include "attest.h"
#include <bcrypt.h>

// Generated using the script driver1/generate_secret.py
static UCHAR g_AttestationSecret[128] = {
    0x2F, 0xDF, 0xED, 0x27, 0x5B, 0x58, 0x4E, 0xDF,
    0x0B, 0xFE, 0x83, 0x9B, 0xEF, 0x11, 0x4C, 0xD2,
    0x78, 0xCA, 0x36, 0xC0, 0x5E, 0x7C, 0xCE, 0x2A,
    0x87, 0x99, 0x1E, 0xAE, 0x66, 0x78, 0xE8, 0x39,
    0x73, 0xE3, 0xBD, 0xE5, 0x8C, 0x6F, 0xDA, 0x19,
    0xBA, 0xC1, 0x8B, 0x00, 0xB9, 0x60, 0x2A, 0xBC,
    0x46, 0xD3, 0x20, 0x28, 0xE2, 0x40, 0xFE, 0x7A,
    0x07, 0xA8, 0xE8, 0xD8, 0xD6, 0xDA, 0x92, 0x88,
    0x5D, 0x36, 0x5E, 0x0B, 0x17, 0x0D, 0x65, 0x9E,
    0x6D, 0x2E, 0x87, 0x06, 0xA8, 0x86, 0xC0, 0xA8,
    0xB7, 0xC6, 0x39, 0xFC, 0x6B, 0xD2, 0xCE, 0x74,
    0xF2, 0x6A, 0x3E, 0x7E, 0x39, 0xBE, 0xCD, 0xA1,
    0x2F, 0x8A, 0x21, 0x3C, 0x8A, 0x8A, 0xEF, 0xC8,
    0x88, 0xCE, 0x64, 0x14, 0x64, 0x4F, 0xBB, 0xE2,
    0xD3, 0x17, 0x95, 0x0E, 0x54, 0x1B, 0xE8, 0x68,
    0x43, 0xCD, 0x55, 0x2A, 0xC1, 0xA2, 0xE4, 0xB2
};

NTSTATUS VerifyAttestation(
    PSCAN_TASK_DTO pScanTaskDto,
    PSCAN_VERDICT_DTO pScanVerdictDto
) {
    if (!pScanTaskDto || !pScanVerdictDto)
        return STATUS_INVALID_PARAMETER;

    NTSTATUS status = STATUS_SUCCESS;
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;

    UCHAR hashObject[1024];     // internal buffer for state
    UCHAR computedHash[32];   // SHA256 digest
    UCHAR attestationBuf[sizeof(pScanVerdictDto->Attestation)];

    // Copy attestation, then zero it out before hashing
    RtlCopyMemory(attestationBuf, pScanVerdictDto->Attestation, sizeof(attestationBuf));
    RtlZeroMemory(pScanVerdictDto->Attestation, sizeof(pScanVerdictDto->Attestation));

    // Open SHA256 algorithm provider
    status = BCryptOpenAlgorithmProvider(
        &hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0);
    if (!NT_SUCCESS(status))
        goto Cleanup;

    status = BCryptCreateHash(
        hAlg,
        &hHash,
        hashObject,
        sizeof(hashObject),
        NULL,
        0,
        0);
    if (!NT_SUCCESS(status))
        goto Cleanup;

    // Combine both DTOs + secret padding into one continuous hash stream
    status = BCryptHashData(hHash, (PUCHAR)pScanTaskDto, sizeof(*pScanTaskDto), 0);
    if (!NT_SUCCESS(status))
        goto Cleanup;

    status = BCryptHashData(hHash, (PUCHAR)pScanVerdictDto, sizeof(*pScanVerdictDto), 0);
    if (!NT_SUCCESS(status))
        goto Cleanup;

    status = BCryptHashData(hHash, g_AttestationSecret, sizeof(g_AttestationSecret), 0);
    if (!NT_SUCCESS(status))
        goto Cleanup;

    // Finalize hash
    status = BCryptFinishHash(hHash, computedHash, sizeof(computedHash), 0);
    if (!NT_SUCCESS(status))
        goto Cleanup;

    // Compare against attestation (first 32 bytes of user-provided signature)
    if (RtlCompareMemory(computedHash, attestationBuf, sizeof(computedHash)) != sizeof(computedHash))
        status = STATUS_INVALID_SIGNATURE;

Cleanup:
    if (hHash) BCryptDestroyHash(hHash);
    if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);

    // Restore attestation for later use (optional)
    RtlCopyMemory(pScanVerdictDto->Attestation, attestationBuf, sizeof(attestationBuf));

    return status;
}
