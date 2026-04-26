# FeatureHasher

This module is a C++ implementation
of sklearn `FeatureHasher`.

It has been tested to be consistent
with the original code in simple
cases. For example, the following
Python code:

```py
from sklearn.feature_extraction import FeatureHasher
FeatureHasher(50, input_type="pair").transform([ [("Abc", 1), ("DEF", 12.3)] ]).toarray()[0]
```

would produce the array

    array([ 0. ,  0. ,  0. ,  0. ,  0. ,  0. ,  0. ,  0. ,  0. ,  0. , 12.3,
            0. ,  0. ,  0. ,  0. ,  0. ,  0. ,  0. ,  0. ,  0. ,  0. ,  0. ,
            0. ,  0. ,  0. ,  0. ,  0. ,  0. ,  0. ,  0. ,  0. ,  0. ,  0. ,
            0. ,  0. ,  0. ,  0. ,  0. ,  1. ,  0. ,  0. ,  0. ,  0. ,  0. ,
            0. ,  0. ,  0. ,  0. ,  0. ,  0. ])

which is the same as the result
of this C++ implementation:

```sh
# Assume you've built the whole project, or this target alone
cd <project_root>
./build/featurehasher/fh hash 50 Abc 1 DEF 12.3
```

which also produces an identical result

    0 0 0 0 0 0 0 0 0 0 12.3 
    0 0 0 0 0 0 0 0 0 0 0 
    0 0 0 0 0 0 0 0 0 0 0 
    0 0 0 0 0 1 0 0 0 0 0 
    0 0 0 0 0 0
