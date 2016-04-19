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
    IAsyncOperation<IVector<int>^>^ GetPrimesAsyncAwait(int first, int last, std::shared_ptr<bool> spDone)
    {
        IVector<int>^ ret = co_await AwaitAdapterTestHelper::GetPrimesAsyncTask(first, last);
        *spDone = true;
        return ret;
    }

    TEST_CLASS(AsyncOperationAwaitTest)
    {
    public:

        TEST_METHOD(AwaitInIAsyncOperation)
        {
            std::shared_ptr<bool> spDone = std::make_shared<bool>(false);
            std::shared_ptr<bool> failed = std::make_shared<bool>(false);
            std::shared_ptr<Concurrency::event> ev = std::make_shared<Concurrency::event>();
            int first = 1, last = 100;

            auto vecExpected = AwaitAdapterTestHelper::GetPrimesAsyncTask(first, last).get();
            IAsyncOperation<IVector<int>^>^ op = GetPrimesAsyncAwait(first, last, spDone);
            op->Completed = ref new AsyncOperationCompletedHandler<IVector<int>^>([ev, failed, vecExpected](IAsyncOperation<IVector<int>^>^ op, AsyncStatus status)
            {
                if (status != AsyncStatus::Completed)
                    *failed = true;

                auto vecActual = op->GetResults();
                *failed = !std::equal(begin(vecActual), end(vecActual), begin(vecExpected));
                
                ev->set();
            });

            ev->wait();
            Assert::IsTrue(*spDone == true, L"await in IAsyncOperation did not complete.");
            Assert::IsTrue(*failed == false, L"IAsyncOperation did not complete successfully.");
        }

        TEST_METHOD(ThrowIAsyncOperation)
        {
            std::shared_ptr<bool> spDone = std::make_shared<bool>(false);
            std::shared_ptr<bool> spFailed = std::make_shared<bool>(false);
            std::shared_ptr<Concurrency::event> ev = std::make_shared<Concurrency::event>();

            IAsyncOperation<IVector<int>^>^ op = GetPrimesAsyncAwait(-1, -1, spDone);
            op->Completed = ref new AsyncOperationCompletedHandler<IVector<int>^>([ev, spFailed](IAsyncOperation<IVector<int>^>^ op, AsyncStatus status)
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

        TEST_METHOD(CancelIAsyncOperation)
        {
            std::shared_ptr<bool> spDone = std::make_shared<bool>(false);
            std::shared_ptr<bool> spFailed = std::make_shared<bool>(false); // Completed Handler sets this variable to notify if the test failed.
            std::shared_ptr<Concurrency::event> ev = std::make_shared<Concurrency::event>();
            int first = 1, last = 100;

            IAsyncOperation<IVector<int>^>^ op = GetPrimesAsyncAwait(first, last, spDone);
            op->Completed = ref new AsyncOperationCompletedHandler<IVector<int>^>([ev, spFailed](IAsyncOperation<IVector<int>^>^ op, AsyncStatus status)
            {
                if (status != AsyncStatus::Canceled || op->Status != AsyncStatus::Canceled)
                    *spFailed = true;

                ev->set();
            });

            op->Cancel();
            // Wait for the prime computation to complete
            auto ret = ev->wait(IASYNC_AWAIT_TEST_TIMEOUT);
            Assert::IsFalse(*spFailed, L"IAsyncOperation status != Cancelled.");
        }
    };
}