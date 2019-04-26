#include <math.h>
#include "esp_err.h"
#include "esp_log.h"
#include <stdint.h>

static char* TAG = "statistics";

/*Naive implementation of this as floatig point values have numerical limits*/
esp_err_t floatMaxValue(float myArray[], size_t size, float* maxValuePtr)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(myArray == NULL || size <= 0 || maxValuePtr == NULL)
    {
        return ESP_FAIL;
    }

    float maxValue = myArray[0];

    for (size_t i = 1; i < size; ++i)
    {
        if ( myArray[i] > maxValue )
        {
            maxValue = myArray[i];
        }
    }
    *maxValuePtr = maxValue;
    ESP_LOGD(TAG, "Obtained max as %f", maxValue);
    return ESP_OK;
}

esp_err_t intMaxValue(int myArray[], size_t size, int* maxValuePtr)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(myArray == NULL || size <= 0 || maxValuePtr == NULL)
    {
        return ESP_FAIL;
    }

    int maxValue = myArray[0];

    for (size_t i = 1; i < size; ++i)
    {
        if ( myArray[i] > maxValue )
        {
            maxValue = myArray[i];
        }
    }

    *maxValuePtr = maxValue;
    ESP_LOGD(TAG, "Obtained min as %d", maxValue);
    return ESP_OK;
}

/*Naive implementation of this as floatig point values have numerical limits*/
esp_err_t floatMinValue(float myArray[], size_t size, float* minValuePtr)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(myArray == NULL || size <= 0 || minValuePtr == NULL)
    {
        return ESP_FAIL;
    }
    
    float minValue = myArray[0];

    for (size_t i = 1; i < size; ++i)
    {
        if ( myArray[i] < minValue )
        {
            minValue = myArray[i];
        }
    }
    *minValuePtr = minValue;
    ESP_LOGD(TAG, "Obtained min as %f", minValue);
    return ESP_OK;
}

esp_err_t intMinValue(int myArray[], size_t size, int* minValuePtr)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(myArray == NULL || size <= 0 || minValuePtr == NULL)
    {
        return ESP_FAIL;
    }
    
    int minValue = myArray[0];

    for (size_t i = 1; i < size; ++i)
    {
        if ( myArray[i] < minValue )
        {
            minValue = myArray[i];
        }
    }
    *minValuePtr = minValue;
    ESP_LOGD(TAG, "Obtained min as %d", minValue);
    return ESP_OK;
}

esp_err_t floatSumValue(float myArray[], size_t size, float* sumValuePtr)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(myArray == NULL || size <= 0 || sumValuePtr == NULL)
    {
        return ESP_FAIL;
    }

    float sum = 0.0;

    for (size_t i = 1; i < size; i++)
    {
        sum +=  myArray[i];
    }
    *sumValuePtr = sum;
    ESP_LOGD(TAG, "Obtained sum as %f", sum);
    return ESP_OK;
}

esp_err_t intSumValue(int myArray[], size_t size, int* sumValuePtr)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(myArray == NULL || size <= 0 || sumValuePtr == NULL)
    {
        return ESP_FAIL;
    }

    int sum = 0.0;

    for (size_t i = 1; i < size; i++)
    {
        sum +=  myArray[i];
    }
    *sumValuePtr = sum;
    ESP_LOGD(TAG, "Obtained sum as %d", sum);
    return ESP_OK;
}

esp_err_t meanValue(float myArray[], size_t size, float* meanValuePtr) 
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(myArray == NULL || size <= 0 || meanValuePtr == NULL)
    {
        return ESP_FAIL;
    }

    float sum = 0.0;

    if(floatSumValue(myArray, size, &sum) != ESP_OK)
    {
        return ESP_FAIL;        
    }

    *meanValuePtr = sum/size;
    
    ESP_LOGD(TAG, "Obtained mean as %f", *meanValuePtr);
    return ESP_OK;
}

esp_err_t variance(float myArray[], size_t size, float* varianceValuePtr) 
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(myArray == NULL || size <= 0 || varianceValuePtr == NULL)
    {
        return ESP_FAIL;
    }

    float sum = 0.0;
    float avg = 0.0;

    if(meanValue(myArray, size, &avg) != ESP_OK)
    {
        return ESP_FAIL;        
    }

    for (size_t i = 0; i < size; i++)
    {
        sum += pow((myArray[i] - avg), 2);
    }

    *varianceValuePtr = sum / size;
    ESP_LOGD(TAG, "Obtained variance as %f", *varianceValuePtr);

    return ESP_OK;
}

esp_err_t stddev(float myArray[], size_t size, float* stddevPtr)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(myArray == NULL || size <= 0 || stddevPtr == NULL)
    {
        return ESP_FAIL;
    }

    float var = 0.0;

    if(variance(myArray, size, &var) != ESP_OK)
    {
        return ESP_FAIL;
    }

    *stddevPtr = (float)sqrt(var);

    return ESP_OK;
}