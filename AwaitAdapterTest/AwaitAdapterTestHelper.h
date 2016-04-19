#pragma once
#include "pch.h"
#include "winrtawaitadapter.h"

#ifndef _WINRT_AWAITADAPTER_HELPER_H
#define _WINRT_AWAITADAPTER_HELPER_H

#define IASYNC_AWAIT_TEST_TIMEOUT 5000

namespace AwaitAdapterTest
{
    namespace AwaitAdapterTestHelper
    {
        // Determines whether the input value is prime. 
        bool is_prime(int n);
        concurrency::task<unsigned int> ComputePrimesTask(int first, int last);
        concurrency::task<Windows::Foundation::Collections::IVector<int>^> GetPrimesAsyncTask(int first, int last);
    }
}
#endif