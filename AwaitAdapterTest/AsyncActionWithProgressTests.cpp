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
    IAsyncActionWithProgress<double>^ ComputePrimesWithProgressAsync(int first, int last, bool& done)
    {
        auto UIContext = await_resume_context::current();
        co_await await_resume_context::any().get_awaitable();
        
        auto pr = co_await get_progress_reporter<double>();

        // Ensure that the input values are in range.
        if (first < 0 || last < 0)
        {
            throw ref new InvalidArgumentException();
        }

        // Perform the computation in parallel. 
        atomic<long> operation = 0;
        long range = last - first + 1;
        double lastPercent = 0.0;
        parallel_for(first, last + 1, [&operation, range, &lastPercent, &pr](int n)
        {
            // Report progress message.
            double progress = 100.0 * (++operation) / range;

            if (progress >= lastPercent)
            {
                pr.report_progress(progress);
                lastPercent += 1.0;
            }

            if (AwaitAdapterTestHelper::is_prime(n))
            {
                // Perhaps store the value somewhere...
            }
        });

        pr.report_progress(100.0);

        done = true;
        return;
    }

    TEST_CLASS(AsyncActionWithProgressAwaitTest)
    {
    public:

        TEST_METHOD(AwaitInIAsyncActionWithProgress)
        {
            bool done = false;
            bool failed = false;
            event ev;
            std::atomic<double> progressCounter = 0;
            int first = 1, last = 100;
            IAsyncActionWithProgress<double>^ op = ComputePrimesWithProgressAsync(first, last, done);
            op->Progress = ref new AsyncActionProgressHandler<double>([&progressCounter](IAsyncActionWithProgress<double>^ action, double progressval)
            {
                progressCounter.exchange(progressval);
            });

            op->Completed = ref new AsyncActionWithProgressCompletedHandler<double>([&](IAsyncActionWithProgress<double>^ action, AsyncStatus status)
            {
                if (status != AsyncStatus::Completed)
                    failed = true;

                ev.set();
            });

            ev.wait();
            Assert::IsTrue(progressCounter == 100, L"Progress reporting not as expected.");
            Assert::IsTrue(done == true, L"await in IAsyncAction did not complete.");
            Assert::IsTrue(failed == false, L"IAsyncAction did not complete.");
        }

        TEST_METHOD(ThrowIAsyncActionWithProgress)
        {
            bool done = false;
            bool failed = false;
            event ev;
            int progress_counter = 0;
            int first = -1, last = -1;

            IAsyncActionWithProgress<double>^ op = ComputePrimesWithProgressAsync(first, last, done);
            op->Progress = ref new AsyncActionProgressHandler<double>([&](IAsyncActionWithProgress<double>^ action, double progressval)
            {
                progress_counter = progress_counter + 1;
            });

            op->Completed = ref new AsyncActionWithProgressCompletedHandler<double>([&](IAsyncActionWithProgress<double>^ action, AsyncStatus status)
            {
                if (status == AsyncStatus::Error && action->Status == AsyncStatus::Error && FAILED(action->ErrorCode.Value))
                    failed = true;

                ev.set();
            });

            // Wait for the prime computation to complete
            auto ret = ev.wait(IASYNC_AWAIT_TEST_TIMEOUT);
            Assert::AreEqual<int>(0, ret, L"IAsyncActionWithProgress Completed handler not called.");
            Assert::IsFalse(done, L"await in IAsyncActionWithProgress completed, exception did not propagate and cancel the await.");
            Assert::IsTrue(failed, L"IAsyncActionWithProgress error did not propagate.");
        }

        TEST_METHOD(CancelIAsyncActionWithProgress)
        {
            bool done = false;
            unsigned int primeCountActual = 0;
            bool failed = false; // Completed Handler sets this variable to notify if the test failed.
            event ev;
            int progress_counter = 0;
            int first = 1, last = 100;

            IAsyncActionWithProgress<double>^ op = ComputePrimesWithProgressAsync(first, last, done);
            op->Progress = ref new AsyncActionProgressHandler<double>([&](IAsyncActionWithProgress<double>^ action, double progressval)
            {
                progress_counter = progress_counter + 1;
            });

            op->Completed = ref new AsyncActionWithProgressCompletedHandler<double>([&](IAsyncActionWithProgress<double>^ action, AsyncStatus status)
            {				
                if (status != AsyncStatus::Canceled || action->Status != AsyncStatus::Canceled)
                    failed = true;

                ev.set();
            });

            op->Cancel();
            // Wait for the prime computation to complete
            auto ret = ev.wait(IASYNC_AWAIT_TEST_TIMEOUT);
            Assert::IsFalse(failed, L"IAsyncAction status != Cancelled.");
        }
    };
}