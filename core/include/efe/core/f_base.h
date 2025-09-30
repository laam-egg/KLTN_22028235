#ifndef FEATURE_BASE_INCLUDED
#define FEATURE_BASE_INCLUDED

/**
 * Base class from which each feature type may inherit
 */
class FeatureType {
public:
    virtual char const* getName() const = 0;

    virtual int getDim() const = 0;

    virtual void calculateFeatureVector(int* output, )
};

#endif // FEATURE_BASE_INCLUDED
