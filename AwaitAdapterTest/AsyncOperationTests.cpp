#include "pch.h"
#include "awaitadaptertesthelper.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Platform;
using namespace Platform::Collections;
using namespace Concurrency;
using namespace std;

namespace AwaitAdapterTest
{
    IAsyncOperation<IVector<int>^>^ GetPrimesAsyncAwait(int first, int last, bool& done)
    {
        IVector<int>^ ret = co_await AwaitAdapterTestHelper::GetPrimesAsyncTask(first, last);
        done = true;
        return ret;
    }

    TEST_CLASS(AsyncOperationAwaitTest)
    {
    public:

        TEST_METHOD(AwaitInIAsyncOperation)
        {
            bool done = false;
            bool failed = false;
            event ev;
            int first = 1, last = 100;

            auto vecExpected = AwaitAdapterTestHelper::GetPrimesAsyncTask(first, last).get();
            IAsyncOperation<IVector<int>^>^ op = GetPrimesAsyncAwait(first, last, done);
            op->Completed = ref new AsyncOperationCompletedHandler<IVector<int>^>([&](IAsyncOperation<IVector<int>^>^ op, AsyncStatus status)
            {
                if (status != AsyncStatus::Completed)
                    failed = true;

                auto vecActual = op->GetResults();
                failed = !std::equal(begin(vecActual), end(vecActual), begin(vecExpected));
                
                ev.set();
            });

            ev.wait();
            Assert::IsTrue(done == true, L"await in IAsyncOperation did not complete.");
            Assert::IsTrue(failed == false, L"IAsyncOperation did not complete successfully.");
        }

        TEST_METHOD(ThrowIAsyncOperation)
        {
            bool done = false;
            bool failed = false;
            event ev;

            IAsyncOperation<IVector<int>^>^ op = GetPrimesAsyncAwait(-1, -1, done);
            op->Completed = ref new AsyncOperationCompletedHandler<IVector<int>^>([&](IAsyncOperation<IVector<int>^>^ op, AsyncStatus status)
            {
                if (status == AsyncStatus::Error && op->Status == AsyncStatus::Error && FAILED(op->ErrorCode.Value))
                    failed = true;

                ev.set();
            });

            auto ret = ev.wait(IASYNC_AWAIT_TEST_TIMEOUT);
            Assert::AreEqual<int>(0, ret, L"IAsyncOperation Completed handler not called.");
            Assert::IsFalse(done, L"await in IAsyncOperation completed, exception did not propagate and cancel the await.");
            Assert::IsTrue(failed == true, L"IAsyncOperation error did not propagate.");
        }

        TEST_METHOD(CancelIAsyncOperation)
        {
            bool done = false;
            bool failed = false; // Completed Handler sets this variable to notify if the test failed.
            event ev;
            int first = 1, last = 100;

            IAsyncOperation<IVector<int>^>^ op = GetPrimesAsyncAwait(first, last, done);
            op->Completed = ref new AsyncOperationCompletedHandler<IVector<int>^>([&](IAsyncOperation<IVector<int>^>^ op, AsyncStatus status)
            {
                if (status != AsyncStatus::Canceled || op->Status != AsyncStatus::Canceled)
                    failed = true;

                ev.set();
            });

            op->Cancel();
            // Wait for the prime computation to complete
            auto ret = ev.wait(IASYNC_AWAIT_TEST_TIMEOUT);
            Assert::IsFalse(failed, L"IAsyncOperation status != Cancelled.");
        }
    };
}