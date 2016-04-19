#include "pch.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Platform;
using namespace Platform::Collections;
using namespace Concurrency;
using namespace std;

namespace AwaitAdapterTest
{
    namespace AwaitAdapterTestHelper
    {
        bool is_prime(int n)
        {
            if (n < 2)
            {
                return false;
            }
            for (int i = 2; i < n; ++i)
            {
                if ((n % i) == 0)
                {
                    return false;
                }
            }
            return true;
        }

        task<IVector<int>^> GetPrimesAsyncTask(int first, int last)
        {
            return create_task([first, last]() -> IVector<int>^
            {
                // Ensure that the input values are in range. 
                if (first < 0 || last < 0)
                {
                    throw ref new InvalidArgumentException();
                }
                // Perform the computation in parallel.
                concurrent_vector<int> primes;
                parallel_for(first, last + 1, [&primes](int n)
                {
                    // If the value is prime, add it to the global vector.
                    if (AwaitAdapterTestHelper::is_prime(n))
                    {
                        primes.push_back(n);
                    }
                });
                // Sort the results.
                sort(begin(primes), end(primes), less<int>());

                // Copy the results to an IVector object. The IVector 
                // interface makes collections of data available to other 
                // Windows Runtime components.
                IVector<int>^ results = ref new Vector<int>();
                for (int prime : primes)
                {
                    results->Append(prime);
                }
                return results;
            });
        }
    }
}