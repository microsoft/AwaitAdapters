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
    IAsyncAction^ ComputePrimesAsyncAwait(int first, int last, std::shared_ptr<bool> done, std::shared_ptr<unsigned int> primeCount)
    {
        IVector<int>^ primevec = co_await AwaitAdapterTestHelper::GetPrimesAsyncTask(first, last);
        *primeCount = primevec->Size;
        *done = true;
    }
 
    TEST_CLASS(AsyncActionAwaitTest)
    {
    public:

        TEST_METHOD(AwaitInIAsyncAction)
        {
            std::shared_ptr<bool> spDone = std::make_shared<bool>(false);
            std::shared_ptr<bool> spFailed = std::make_shared<bool>(false);
            std::shared_ptr<Concurrency::event> ev = std::make_shared<Concurrency::event>();
            std::shared_ptr<unsigned int> spPrimeCountActual = std::make_shared<unsigned int>(0);
            int first = 1, last = 100;

            IAsyncAction^ op = ComputePrimesAsyncAwait(first, last, spDone, spPrimeCountActual);
            op->Completed = ref new AsyncActionCompletedHandler([ev, spFailed](IAsyncAction^ action, AsyncStatus status)
            {
                if (status != AsyncStatus::Completed)
                    *spFailed = true;

                ev->set();
            });

            ev->wait();
            auto primesCountExpected = AwaitAdapterTestHelper::GetPrimesAsyncTask(first, last).get()->Size;

            Assert::IsTrue(*spDone == true, L"await in IAsyncAction did not complete.");
            Assert::IsTrue(*spFailed == false, L"IAsyncAction did not complete.");
            Assert::IsTrue(*spPrimeCountActual == primesCountExpected, L"Primes count does not match.");
        }

        TEST_METHOD(ThrowIAsyncAction)
        {
            std::shared_ptr<bool> spDone = std::make_shared<bool>(false);
            std::shared_ptr<bool> spFailed = std::make_shared<bool>(false); 
            std::shared_ptr<event> ev = std::make_shared<event>();
            std::shared_ptr<unsigned int> spPrimeCountActual = std::make_shared<unsigned int>(0);
            int first = -1, last = -1;

            IAsyncAction^ op = ComputePrimesAsyncAwait(first, last, spDone, spPrimeCountActual);
            op->Completed = ref new AsyncActionCompletedHandler([ev, spFailed](IAsyncAction^ action, AsyncStatus status)
            {
                if (status == AsyncStatus::Error && action->Status == AsyncStatus::Error && FAILED(action->ErrorCode.Value))
                    *spFailed = true;

                ev->set();
            });

            // Wait for the prime computation to complete
            auto ret = ev->wait(IASYNC_AWAIT_TEST_TIMEOUT);
            Assert::AreEqual<int>(0, ret, L"IAsyncAction Completed handler not called.");
            Assert::IsFalse(*spDone, L"await in IAsyncAction completed, exception did not propagate and cancel the await.");
            Assert::IsTrue(*spFailed, L"IAsyncAction error did not propagate.");
        }

        TEST_METHOD(CancelIAsyncAction)
        {
            std::shared_ptr<bool> spDone = std::make_shared<bool>(false);
            std::shared_ptr<unsigned int> spPrimeCountActual = std::make_shared<unsigned int>(0);
            std::shared_ptr<bool> spFailed = std::make_shared<bool>(false); // Completed Handler sets this variable to notify if the test failed.
            std::shared_ptr<Concurrency::event> ev = std::make_shared<Concurrency::event>();
            int first = 1, last = 1000;

            IAsyncAction^ op = ComputePrimesAsyncAwait(first, last, spDone, spPrimeCountActual);
            op->Completed = ref new AsyncActionCompletedHandler([ev, spFailed](IAsyncAction^ action, AsyncStatus status)
            {
                if (status != AsyncStatus::Canceled || action->Status != AsyncStatus::Canceled)
                    *spFailed = true;

                ev->set();
            });

            op->Cancel();
            // Wait for the prime computation to complete
            auto ret = ev->wait(IASYNC_AWAIT_TEST_TIMEOUT);
            Assert::IsFalse(*spFailed, L"IAsyncAction status != Cancelled.");
        }
    };
}