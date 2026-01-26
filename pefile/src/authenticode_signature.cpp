#include "efe/pefile/authenticode_signature.h"
#include <chrono>
#include <array>
#include <queue>
#include "efe/common/timestamps.h"

#include "authenticode-parser/authenticode.h"

/**
 * Return the first program_name found in the signature's
 * authenticated attributes (SpcSpOpusInfo structures).
 * 
 * Return an empty string if not found.
 */
static std::string getSignatureProgramName(LIEF::PE::Signature const& sig) {
    for (LIEF::PE::SignerInfo const& signer : sig.signers()) {
        // NOTE: If tests fail, try signer.get_attribute instead
        LIEF::PE::Attribute const* attr = signer.get_auth_attribute(
            LIEF::PE::Attribute::TYPE::SPC_SP_OPUS_INFO
        );

        if (attr == nullptr) {
            continue;
        }

        LIEF::PE::SpcSpOpusInfo const* opusInfo = attr->cast<LIEF::PE::SpcSpOpusInfo>();

        if (opusInfo == nullptr) {
            continue;
        }
        if (opusInfo->program_name().empty()) {
            continue;
        }

        return opusInfo->program_name();
    }
    return "";
}

static uint64_t getSigningTimeFromSigner(LIEF::PE::SignerInfo const& signer) {
    LIEF::PE::Attribute const* signingTimeAttr =
        signer.get_attribute(
            LIEF::PE::Attribute::TYPE::PKCS9_SIGNING_TIME
        );
    if (signingTimeAttr == nullptr) {
        return 0;
    }

    LIEF::PE::PKCS9SigningTime const* signingTime =
        signingTimeAttr->cast<LIEF::PE::PKCS9SigningTime>();
    if (signingTime == nullptr) {
        return 0;
    }

    return to_timestamp_utc(signingTime->time());
}

struct CounterSignatureInfo {
    bool hasCounterSigner = false;
    uint64_t signingTime = 0;
};

static CounterSignatureInfo getCounterSignatureInfo(LIEF::PE::Signature const& sig) {
    CounterSignatureInfo info = {};

    std::queue<LIEF::PE::SignerInfo const*> signersQueue;

    for (LIEF::PE::SignerInfo const& signer : sig.signers()) {
        signersQueue.push(&signer);
    }

    auto const inspectAttribute = [&signersQueue, &info](LIEF::PE::Attribute const& attr) -> bool {
        if (attr.type() == LIEF::PE::Attribute::TYPE::PKCS9_COUNTER_SIGNATURE) {
            LIEF::PE::PKCS9CounterSignature const* counterSig =
                attr.cast<LIEF::PE::PKCS9CounterSignature>();
            
            if (counterSig != nullptr) {
                uint64_t signingTime = getSigningTimeFromSigner(counterSig->signer());
                if (signingTime != 0) {
                    info.hasCounterSigner = true;
                    info.signingTime = signingTime;
                    return true;
                }
            }

            signersQueue.push(&counterSig->signer());
        } else if (attr.type() == LIEF::PE::Attribute::TYPE::MS_COUNTER_SIGN) {
            LIEF::PE::MsCounterSign const* counterSig =
                attr.cast<LIEF::PE::MsCounterSign>();
            
            if (counterSig != nullptr) {
                for (LIEF::PE::SignerInfo const& counterSigner : counterSig->signers()) {
                    uint64_t signingTime = getSigningTimeFromSigner(counterSigner);
                    if (signingTime != 0) {
                        info.hasCounterSigner = true;
                        info.signingTime = signingTime;
                        return true;
                    }

                    signersQueue.push(&counterSigner);
                }
            }
        } else if (attr.type() == LIEF::PE::Attribute::TYPE::MS_SPC_NESTED_SIGN) {
            LIEF::PE::MsSpcNestedSignature const* nestedSig =
                attr.cast<LIEF::PE::MsSpcNestedSignature>();
            
            if (nestedSig != nullptr) {
                for (LIEF::PE::SignerInfo const& nestedSigner : nestedSig->sig().signers()) {
                    uint64_t signingTime = getSigningTimeFromSigner(nestedSigner);
                    if (signingTime != 0) {
                        info.hasCounterSigner = true;
                        info.signingTime = signingTime;
                        return true;
                    }

                    signersQueue.push(&nestedSigner);
                }
            }
        }

        return false;
    };

    while (!signersQueue.empty()) {
        LIEF::PE::SignerInfo const& signer = *(signersQueue.front());
        signersQueue.pop();

        for (LIEF::PE::Attribute const& attr : signer.authenticated_attributes()) {
            if (inspectAttribute(attr)) {
                return info;
            }
        }

        for (LIEF::PE::Attribute const& attr : signer.unauthenticated_attributes()) {
            if (inspectAttribute(attr)) {
                return info;
            }
        }
    }

    return info;
}

class ThreadLocalInitialization {
public:
    static ThreadLocalInitialization& getInstance() {
        thread_local ThreadLocalInitialization instance;
        return instance;
    }

private:
    ThreadLocalInitialization() {
        initialize_authenticode_parser();
    }
    ~ThreadLocalInitialization() = default;
    ThreadLocalInitialization(const ThreadLocalInitialization&) = delete;
    ThreadLocalInitialization& operator=(const ThreadLocalInitialization&) = delete;
};

static CounterSignatureInfo getCounterSignatureInfo_Avast(uint8_t const* data, size_t dataSize) {
    CounterSignatureInfo info = {};

    ThreadLocalInitialization::getInstance();
    AuthenticodeArray* auth = parse_authenticode(data, static_cast<uint64_t>(dataSize));
    if (auth && auth->count > 0) {
        for (size_t i = 0; i < auth->count; ++i) {
            Authenticode const* authenticode = auth->signatures[i];

            for (size_t i = 0; i < authenticode->countersigs->count; ++i) {
                Countersignature *counter = authenticode->countersigs->counters[i];

                int64_t signTime = counter->sign_time;
                if (signTime > 0) {
                    info.hasCounterSigner = true;
                    info.signingTime = static_cast<uint64_t>(signTime);
                    goto FINISH;
                }
            }
        }
    }
    FINISH:
    authenticode_array_free(auth); // null-safe operation
    return info;
}

PEAuthenticodeSignatureInfo
PEAuthenticodeSignatureInfo::extractFromPEFile(
    LIEF::PE::Binary const& pe,
    uint8_t const* fileData,
    size_t fileSize
) {
    PEAuthenticodeSignatureInfo info = {};

    if (!pe.has_signatures() || pe.signatures().empty()) {
        return info;
    }

    info.numSignatures = pe.signatures().size();

    for (LIEF::PE::Signature const& sig : pe.signatures()) {
        LIEF::PE::Signature::VERIFICATION_FLAGS verificationFlags = sig.check();
        if (verificationFlags != LIEF::PE::Signature::VERIFICATION_FLAGS::OK) {
            info.parseErrors += 1;
        }

        if (getSignatureProgramName(sig).empty()) {
            info.emptyProgramName = true;
        }

        // CounterSignatureInfo counterSigInfo = getCounterSignatureInfo(sig);
        // if (counterSigInfo.hasCounterSigner) {
        //     info.noCounterSigner = false;
        //     info.latestSigningTime = std::max(
        //         info.latestSigningTime,
        //         counterSigInfo.signingTime
        //     );

        //     uint64_t peTimestamp = pe.header().time_date_stamp();
        //     int64_t diff = (
        //         static_cast<int64_t>(counterSigInfo.signingTime)
        //         - static_cast<int64_t>(peTimestamp)
        //     );
        //     info.signingTimeDiff = diff;
        // } else {
        //     info.noCounterSigner = true;
        // }
        
        info.chainMaxDepth = std::max(
            info.chainMaxDepth,
            sig.certificates().size()
        );

        // The original implementation skips the last certificate. I don't know why.
        int maxIndex = static_cast<int>(sig.certificates().size()) - 1;
        for (int i = 0; i < maxIndex; ++i) {
            LIEF::PE::x509 const& cert = sig.certificates()[i];
            if (cert.issuer() == cert.subject()) {
                info.numSelfSignedCerts += 1;
            }
        }
    }

    CounterSignatureInfo counterSigInfo = getCounterSignatureInfo_Avast(
        fileData,
        fileSize
    );
    if (counterSigInfo.hasCounterSigner) {
        info.noCounterSigner = false;
        info.latestSigningTime = std::max(
            info.latestSigningTime,
            counterSigInfo.signingTime
        );

        uint64_t peTimestamp = pe.header().time_date_stamp();
        int64_t diff = (
            static_cast<int64_t>(counterSigInfo.signingTime)
            - static_cast<int64_t>(peTimestamp)
        );
        info.signingTimeDiff = diff;
    } else {
        info.noCounterSigner = true;
    }

    return info;
}
