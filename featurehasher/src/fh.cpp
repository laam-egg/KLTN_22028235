#include "efe/featurehasher.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>

void hash(size_t N, int argc, char* argv[]) {
    auto output = std::make_unique<feature_t[]>(N);

    FeatureHasher fh;
    fh.reset();
    fh.setSize(N);
    fh.setOutput(output.get());
    fh.start();

    for (int i = 0; i < argc; ++i) {
        char const* p1 = argv[i];
        feature_t p2 = static_cast<feature_t>(std::atof(argv[++i]));

        fh.reduce(p1, p2);
    }

    fh.finalize();

    for (size_t i = 0; i < N; ++i) {
        if (i % 11 == 0) {
            std::cout << '\n';
        }
        std::cout << output.get()[i] << " ";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4 || argc % 2 == 0) {
        std::cerr << "Usage: fh hash <BUCKET_SIZE> <STRING1> <FLOAT1> <STRING2> <FLOAT2> ..." << '\n';
        std::cerr << "e.g. : fh hash       50      abc 0 def 12 ghi -4 jkl 15.02" << '\n';
        return 1;
    }

    if (strcmp(argv[1], "hash") == 0) {
        int bucketSize = std::atoi(argv[2]);
        hash(static_cast<size_t>(bucketSize), argc - 3, argv + 3);
    } else {
        std::cerr << "Unknown command: " << argv[1] << '\n';
        return 1;
    }
    std::cout << '\n';
    return 0;
}
