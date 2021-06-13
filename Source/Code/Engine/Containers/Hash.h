#pragma once

#include "String.h"

template<typename T>
uint64_t DefaultHashFunction(const T& Arg);

template<>
uint64_t DefaultHashFunction(const String& Arg);