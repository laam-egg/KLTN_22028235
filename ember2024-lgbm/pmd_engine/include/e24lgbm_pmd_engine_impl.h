#pragma once

extern "C" {

typedef struct _PMDImpl_Engine_Decision {
    double score;
    int label;
} PMDImpl_Engine_Decision;

void* PMDImpl_Engine_Init(void);
int PMDImpl_Engine_Predict(void* engine, const wchar_t* filePath, PMDImpl_Engine_Decision* decision);
void PMDImpl_Engine_Destroy(void* engine);

} // extern "C"
