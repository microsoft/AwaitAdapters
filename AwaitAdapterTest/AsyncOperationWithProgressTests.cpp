#include "pch.h"
#include "awaitadaptertesthelper.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Platform;
using namespace Platform::Collections;
using namespace Concurrency;
using namespace std;
using namespace std::experimental;
using namespace winrt_await_adapters;

namespace AwaitAdapterTest
{
    IAsyncOperationWithProgress<IVector<int>^, double>^ GetPrimesWithProgressAsync(int first, int last, std::shared_ptr<bool> done)
    {
        auto UIContext = await_resume_context::current();
        co_await await_resume_context::any().get_awaitable();

        auto pr = co_await get_progress_reporter<IAsyncOperationWithProgress<IVector<int>^, double>, double>();

        // Ensure that the input values are in range.
        if (first < 0 || last < 0)
        {
            throw ref new InvalidArgumentException();
        }
        // Perform the computation in parallel.
        concurrent_vector<int> primes;
        long operation = 0;
        long range = last - first + 1;
        double lastPercent = 0.0;
        parallel_for(first, last + 1, [&primes, &operation, range, &lastPercent, &pr](int n)
        {
            // Report progress message.
            double progress = 100.0 * (++operation) / range;
            if (progress >= lastPercent)
            {
                pr.report_progress(progress);
                lastPercent += 1.0;
            }

            // If the value is prime, add it to the local vector. 
            if (AwaitAdapterTestHelper::is_prime(n))
            {
                primes.push_back(n);
            }
        });
        pr.report_progress(100.0);

        // Sort the results.
        sort(begin(primes), end(primes), less<int>());

        // Copy the results to an IVector object. The IVector 
        // interface makes collections of data available to other 
        // Windows Runtime components.
        auto results = ref new Vector<int>();
        for (int prime : primes)
        {
            results->Append(prime);
        }

        *done = true;
        return results;
    }

    TEST_CLASS(AsyncOperationWithProgressAwaitTest)
    {
    public:

        TEST_METHOD(AwaitInIAsyncOperationWithProgress)
        {
            std::shared_ptr<bool> spDone = std::make_shared<bool>(false);
            std::shared_ptr<bool> failed = std::make_shared<bool>(false);
            std::shared_ptr<Concurrency::event> ev = std::make_shared<Concurrency::event>();
            std::atomic<double> progressCounter = 0;
            int first = 1, last = 100;

            auto vecExpected = AwaitAdapterTestHelper::GetPrimesAsyncTask(first, last).get();
            IAsyncOperationWithProgress<IVector<int>^, double>^ op = GetPrimesWithProgressAsync(first, last, spDone);
            op->Progress = ref new AsyncOperationProgressHandler<IVector<int>^, double>([&progressCounter](IAsyncOperationWithProgress<IVector<int>^, double>^ op, double progressval)
            {
                progressCounter.exchange(progressval);
            });

            op->Completed = ref new AsyncOperationWithProgressCompletedHandler<IVector<int>^, double>([ev, failed, vecExpected](IAsyncOperationWithProgress<IVector<int>^, double>^ op, AsyncStatus status)
            {
                if (status != AsyncStatus::Completed)
                    *failed = true;

                auto vecActual = op->GetResults();
                *failed = !std::equal(begin(vecActual), end(vecActual), begin(vecExpected));

                ev->set();
            });

            ev->wait();
            Assert::IsTrue(progressCounter == 100, L"Progress reporting not as expected.");
            Assert::IsTrue(*spDone == true, L"await in IAsyncOperation did not complete.");
            Assert::IsTrue(*failed == false, L"IAsyncOperation did not complete successfully.");
        }

        TEST_METHOD(ThrowIAsyncOperationWithProgress)
        {
            std::shared_ptr<bool> spDone = std::make_shared<bool>(false);
            std::shared_ptr<bool> spFailed = std::make_shared<bool>(false);
            std::shared_ptr<Concurrency::event> ev = std::make_shared<Concurrency::event>();

            IAsyncOperationWithProgress<IVector<int>^, double>^ op = GetPrimesWithProgressAsync(-1, -1, spDone);
            op->Completed = ref new AsyncOperationWithProgressCompletedHandler<IVector<int>^, double>([ev, spFailed](IAsyncOperationWithProgress<IVector<int>^, double>^ op, AsyncStatus status)
            {
                if (status == AsyncStatus::Error && op->Status == AsyncStatus::Error && FAILED(op->ErrorCode.Value))
                    *spFailed = true;

                ev->set();
            });

            auto ret = ev->wait(IASYNC_AWAIT_TEST_TIMEOUT);
            Assert::AreEqual<int>(0, ret, L"IAsyncOperation Completed handler not called.");
            Assert::IsFalse(*spDone, L"await in IAsyncOperation completed, exception did not propagate and cancel the await.");
            Assert::IsTrue(*spFailed == true, L"IAsyncOperation error did not propagate.");
        }

        TEST_METHOD(CancelIAsyncOperationWithProgress)
        {
            std::shared_ptr<bool> spDone = std::make_shared<bool>(false);
            std::shared_ptr<bool> spFailed = std::make_shared<bool>(false); // Completed Handler sets this variable to notify if the test failed.
            std::shared_ptr<Concurrency::event> ev = std::make_shared<Concurrency::event>();
            int first = 1, last = 100;

            IAsyncOperationWithProgress<IVector<int>^, double>^ op = GetPrimesWithProgressAsync(first, last, spDone);
            op->Completed = ref new AsyncOperationWithProgressCompletedHandler<IVector<int>^, double>([ev, spFailed](IAsyncOperationWithProgress<IVector<int>^, double>^ op, AsyncStatus status)
            {
                if (status != AsyncStatus::Canceled || op->Status != AsyncStatus::Canceled)
                    *spFailed = true;

                ev->set();
            });

            op->Cancel();
            // Wait for the prime computation to complete
            auto ret = ev->wait(IASYNC_AWAIT_TEST_TIMEOUT);
            Assert::IsFalse(*spFailed, L"IAsyncOperationWithProgress status != Cancelled.");
        }
    };
}