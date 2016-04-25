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
    // Compute prime using await
    IAsyncAction^ ComputePrimesAsyncAwait(int first, int last, bool& done, unsigned int& primeCount)
    {
        IVector<int>^ primevec = co_await AwaitAdapterTestHelper::GetPrimesAsyncTask(first, last);
        primeCount = primevec->Size;
        done = true;
    }
 
    TEST_CLASS(AsyncActionAwaitTest)
    {
    public:

        TEST_METHOD(AwaitInIAsyncAction)
        {
            bool done = false;
            bool failed = false;
            Concurrency::event ev;
            unsigned int primeCountActual = 0;

            int first = 1, last = 100;

            IAsyncAction^ op = ComputePrimesAsyncAwait(first, last, done, primeCountActual);
            op->Completed = ref new AsyncActionCompletedHandler([&](IAsyncAction^ action, AsyncStatus status)
            {
                if (status != AsyncStatus::Completed)
                    failed = true;

                ev.set();
            });

            ev.wait();
            auto primesCountExpected = AwaitAdapterTestHelper::GetPrimesAsyncTask(first, last).get()->Size;

            Assert::IsTrue(done == true, L"await in IAsyncAction did not complete.");
            Assert::IsTrue(failed == false, L"IAsyncAction did not complete.");
            Assert::IsTrue(primeCountActual == primesCountExpected, L"Primes count does not match.");
        }

        TEST_METHOD(ThrowIAsyncAction)
        {
            bool done = false;
            bool failed = false; 
            event ev;
            unsigned int primeCountActual = 0;
            int first = -1, last = -1;

            IAsyncAction^ op = ComputePrimesAsyncAwait(first, last, done, primeCountActual);
            op->Completed = ref new AsyncActionCompletedHandler([&](IAsyncAction^ action, AsyncStatus status)
            {
                if (status == AsyncStatus::Error && action->Status == AsyncStatus::Error && FAILED(action->ErrorCode.Value))
                    failed = true;

                ev.set();
            });

            // Wait for the prime computation to complete
            auto ret = ev.wait(IASYNC_AWAIT_TEST_TIMEOUT);
            Assert::AreEqual<int>(0, ret, L"IAsyncAction Completed handler not called.");
            Assert::IsFalse(done, L"await in IAsyncAction completed, exception did not propagate and cancel the await.");
            Assert::IsTrue(failed, L"IAsyncAction error did not propagate.");
        }

        TEST_METHOD(CancelIAsyncAction)
        {
            bool done = false;
            unsigned int primeCountActual = 0;
            bool failed = false; // Completed Handler sets this variable to notify if the test failed.
            event ev;
            int first = 1, last = 1000;

            IAsyncAction^ op = ComputePrimesAsyncAwait(first, last, done, primeCountActual);
            op->Completed = ref new AsyncActionCompletedHandler([&](IAsyncAction^ action, AsyncStatus status)
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