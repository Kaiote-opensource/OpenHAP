#ifndef _STATISTICS_H_
#define _STATISTICS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

esp_err_t floatMaxValue(const float myArray[], const size_t size, float* maxValuePtr, const bool exitInfNan, size_t* computeSize);

esp_err_t intMaxValue(const int myArray[], const size_t size, int* maxValuePtr);

esp_err_t floatMinValue(const float myArray[], const size_t size, float* minValuePtr, const bool exitInfNan, size_t* computeSize);

esp_err_t intMinValue(const int myArray[], const size_t size, int* minValuePtr);

esp_err_t floatSumValue(const float myArray[], const size_t size, float* sumValuePtr, const bool exitInfNan, size_t* computeSize);

esp_err_t intSumValue(const int myArray[], const size_t size, int* sumValuePtr);

esp_err_t meanValue(const float myArray[], const size_t size, float* meanValuePtr, const bool exitInfNan, size_t* computeSize);

esp_err_t variance(const float myArray[], const size_t size, float* varianceValuePtr, const bool exitInfNan, size_t* computeSize);

esp_err_t stddev(const float myArray[], const size_t size, float* stddevPtr, const bool exitInfNan, size_t* computeSize);

#endif
