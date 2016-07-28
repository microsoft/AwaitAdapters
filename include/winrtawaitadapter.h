/***
* ==++==
*
* Copyright (c) Microsoft Corporation. All rights reserved.
*
* ==++==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* winrtawaitadapter.h
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

#pragma once

#ifndef _WINRT_AWAITADAPTER_H
#define _WINRT_AWAITADAPTER_H

#ifdef __cplusplus_winrt

#include <exception>
#include <winerror.h>
#include <intrin.h>
#include <experimental\coroutine>
namespace winrt_await_adapters
{
    //
    // Base class for the promise returned for asynchronous actions and operations.
    //
    ref class _AsyncInfoPromiseBase abstract : public Windows::Foundation::IAsyncInfo
    {
    public:

        // IAsyncInfo interface method: Cancels the asynchronous action/operation.
        virtual void Cancel()
        {
            m_status = Windows::Foundation::AsyncStatus::Canceled;
        }

        // IAsyncInfo interface method: Closes the asynchronous action/operation.
        virtual void Close()
        {
        }

        // IAsyncInfo property: Error condition of the asynchronous action/operation.
        virtual property Windows::Foundation::HResult ErrorCode
        {
            Windows::Foundation::HResult get()
            {
                return m_errorcode;
            }
        }

        // IAsyncInfo property: Handle of the asynchronous action/operation.
        virtual property unsigned int Id
        {
            unsigned int get()
            {
                return m_id;
            }
            void set(unsigned int id)
            {
                m_id = id;
            }
        }

        // IAsyncInfo property: value that indicates the status of the asynchronous action/operation.
        virtual property Windows::Foundation::AsyncStatus Status
        {
            Windows::Foundation::AsyncStatus get()
            {
                return m_status;
            }
            void set(Windows::Foundation::AsyncStatus status)
            {
                m_status = status;
            }
        }

    internal:
        _AsyncInfoPromiseBase() :
            m_status(Windows::Foundation::AsyncStatus::Started)
        {
            m_id = 1;
            m_errorcode.Value = S_OK;
        }

        void _SetError(Platform::Exception^ ex)
        {
            Status = Windows::Foundation::AsyncStatus::Error;
            m_errorcode.Value = ex->HResult;
        }

        Windows::Foundation::HResult m_errorcode;
        Windows::Foundation::AsyncStatus m_status;
        unsigned int m_id;
    };

    template <typename _Attributes>
    ref class _AsyncActionPromise : public _AsyncInfoPromiseBase, public _Attributes::_AsyncBaseType
    {
        using _CompletionDelegateType = typename _Attributes::_CompletionDelegateType;
    public:
        virtual void GetResults()
        {
        }

        // Get/set the method that handles the action completed notification.
        virtual property _CompletionDelegateType^ Completed
        {
            _CompletionDelegateType^ get()
            {
                return m_completedHandler;
            }

            void set(_CompletionDelegateType^ _CompletedHandler)
            {
                m_completedHandler = _CompletedHandler;
            }
        }

        void Cancel() override
        {
            _AsyncInfoPromiseBase::Cancel();
        }

        void Close() override
        {
            _AsyncInfoPromiseBase::Close();
        }

        property Windows::Foundation::HResult ErrorCode
        {
            Windows::Foundation::HResult get() override
            {
                return _AsyncInfoPromiseBase::ErrorCode;
            }
        }

        property unsigned int Id
        {
            unsigned int get() override
            {
                return _AsyncInfoPromiseBase::Id;
            }
            void set(unsigned int id) override
            {
                _AsyncInfoPromiseBase::Id = id;
            }
        }

        property Windows::Foundation::AsyncStatus Status
        {
            Windows::Foundation::AsyncStatus get() override
            {
                return _AsyncInfoPromiseBase::Status;
            }
            void set(Windows::Foundation::AsyncStatus status) override
            {
                _AsyncInfoPromiseBase::Status = status;
            }
        }

    internal:
        // Called by promise_type::return_void for AsyncActionPromise coroutine_traits
        void _InvokeCompletion()
        {
            // If Status is already set to Canceled/Error, retain the state.
            if (Status == Windows::Foundation::AsyncStatus::Started)
                Status = Windows::Foundation::AsyncStatus::Completed;

            if (m_completedHandler)
            {
                m_completedHandler->Invoke(this, Status);
            }
        }

        // Called by coroutine_traits::promise_type::set_exception to propagate the error.
        // There is no way to propagate the Exception message.
        void _SetError(Platform::Exception^ ex)
        {
            _AsyncInfoPromiseBase::_SetError(ex);
            _InvokeCompletion();
        }

        _CompletionDelegateType^  m_completedHandler;
    };

    template <typename _Attributes>
    ref class _AsyncActionWithProgressPromise : public _AsyncInfoPromiseBase, public _Attributes::_AsyncBaseType
    {
        using _CompletionDelegateType = typename _Attributes::_CompletionDelegateType;
        using _ProgressDelegateType = typename _Attributes::_ProgressDelegateType;
    public:
        void Close() override
        {
            _AsyncInfoPromiseBase::Close();
        }

        void Cancel() override
        {
            _AsyncInfoPromiseBase::Cancel();
        }

        virtual property Windows::Foundation::HResult ErrorCode
        {
            Windows::Foundation::HResult get() override
            {
                return _AsyncInfoPromiseBase::ErrorCode;
            }
        }

        virtual property unsigned int Id
        {
            unsigned int get() override
            {
                return _AsyncInfoPromiseBase::Id;
            }
            void set(unsigned int id) override
            {
                _AsyncInfoPromiseBase::Id = id;
            }
        }

        virtual property Windows::Foundation::AsyncStatus Status
        {
            Windows::Foundation::AsyncStatus get() override
            {
                return _AsyncInfoPromiseBase::Status;
            }
            void set(Windows::Foundation::AsyncStatus status) override
            {
                _AsyncInfoPromiseBase::Status = status;
            }
        }

        virtual void GetResults()
        {
        }

        virtual property _ProgressDelegateType^ Progress
        {
            _ProgressDelegateType^ get()
            {
                return m_progressHandler;
            }

            void set(_ProgressDelegateType^ progressHandler)
            {
                m_progressHandler = progressHandler;
            }
        }

        // Get/set the method that handles the action/operation completed notification.
        virtual property _CompletionDelegateType^ Completed
        {
            _CompletionDelegateType^ get()
            {
                return m_completedHandler;
            }

            void set(_CompletionDelegateType^ _CompletedHandler)
            {
                m_completedHandler = _CompletedHandler;
            }
        }

    internal:
        void _InvokeProgressHandler(typename _Attributes::_ProgressType progress)
        {
            if (m_progressHandler)
            {
                m_progressHandler->Invoke(this, progress);
            }
        }

        // Called by promise_type::return_void for AsyncAction/AsyncActionWithProgress coroutine_traits
        void _InvokeCompletion()
        {
            // If m_status is already set to Canceled/Error, retain the state.
            if (Status == Windows::Foundation::AsyncStatus::Started)
                Status = Windows::Foundation::AsyncStatus::Completed;

            if (m_completedHandler)
            {
                m_completedHandler->Invoke(this, Status);
            }
        }

        // Called by coroutine_traits::promise_type::set_exception to propagate the error.
        // There is no way to propagate the Exception message.
        void _SetError(Platform::Exception^ ex)
        {
            _AsyncInfoPromiseBase::_SetError(ex);
            _InvokeCompletion();
        }

        _ProgressDelegateType^ m_progressHandler;
        _CompletionDelegateType^ m_completedHandler;
    };

    template <typename _Attributes>
    ref class _AsyncOperationPromise :public _AsyncInfoPromiseBase, public _Attributes::_AsyncBaseType
    {
        using _CompletionDelegateType = typename _Attributes::_CompletionDelegateType;
        using _ReturnType = typename _Attributes::_ReturnType;

    public:
        void Close() override
        {
            _AsyncInfoPromiseBase::Close();
        }

        void Cancel() override
        {
            _AsyncInfoPromiseBase::Cancel();
        }

        virtual property Windows::Foundation::HResult ErrorCode
        {
            Windows::Foundation::HResult get() override
            {
                return _AsyncInfoPromiseBase::ErrorCode;
            }
        }

        virtual property unsigned int Id
        {
            unsigned int get() override
            {
                return _AsyncInfoPromiseBase::Id;
            }
            void set(unsigned int id) override
            {
                _AsyncInfoPromiseBase::Id = id;
            }
        }

        virtual property Windows::Foundation::AsyncStatus Status
        {
            Windows::Foundation::AsyncStatus get() override
            {
                return _AsyncInfoPromiseBase::Status;
            }
            void set(Windows::Foundation::AsyncStatus status) override
            {
                _AsyncInfoPromiseBase::Status = status;
            }
        }

        virtual _ReturnType GetResults()
        {
            return m_results;
        }

        // Get/set the method that handles the action/operation completed notification.
        virtual property _CompletionDelegateType^ Completed
        {
            _CompletionDelegateType^ get()
            {
                return m_completedHandler;
            }

            void set(_CompletionDelegateType^ _CompletedHandler)
            {
                m_completedHandler = _CompletedHandler;
            }
        }

    internal:
        // Called by promise_type::return_value for AsyncOperation/AsyncOperationWithProgress coroutine_traits
        void _InvokeCompletion(_ReturnType result)
        {
            // If Status is already set to Canceled/Error, retain the state.
            if (Status == Windows::Foundation::AsyncStatus::Started)
                Status = Windows::Foundation::AsyncStatus::Completed;

            if (m_completedHandler)
            {
                m_results = result;
                m_completedHandler->Invoke(this, Status);
            }
        }

        // Called by coroutine_traits::promise_type::set_exception to propagate the error.
        // There is no way to propagate the Exception message.
        void _SetError(Platform::Exception^ ex)
        {
            _AsyncInfoPromiseBase::_SetError(ex);
            _InvokeCompletion(nullptr);
        }

        _ReturnType m_results;
        _CompletionDelegateType^ m_completedHandler;
    };

    template <typename _Attributes>
    ref class _AsyncOperationWithProgressPromise :public _AsyncInfoPromiseBase, public _Attributes::_AsyncBaseType
    {
        using _CompletionDelegateType = typename _Attributes::_CompletionDelegateType;
        using _ProgressDelegateType = typename _Attributes::_ProgressDelegateType;
        using _ReturnType = typename _Attributes::_ReturnType;
    public:
        void Close() override
        {
            _AsyncInfoPromiseBase::Close();
        }

        void Cancel() override
        {
            _AsyncInfoPromiseBase::Cancel();
        }

        virtual property Windows::Foundation::HResult ErrorCode
        {
            Windows::Foundation::HResult get() override
            {
                return _AsyncInfoPromiseBase::ErrorCode;
            }
        }

        virtual property unsigned int Id
        {
            unsigned int get() override
            {
                return _AsyncInfoPromiseBase::Id;
            }
            void set(unsigned int id) override
            {
                _AsyncInfoPromiseBase::Id = id;
            }
        }

        virtual property Windows::Foundation::AsyncStatus Status
        {
            Windows::Foundation::AsyncStatus get() override
            {
                return _AsyncInfoPromiseBase::Status;
            }
            void set(Windows::Foundation::AsyncStatus status) override
            {
                _AsyncInfoPromiseBase::Status = status;
            }
        }

        virtual _ReturnType GetResults()
        {
            return m_results;
        }

        // Get/set the method that handles the action/operation completed notification.
        virtual property _CompletionDelegateType^ Completed
        {
            _CompletionDelegateType^ get()
            {
                return m_completedHandler;
            }

            void set(_CompletionDelegateType^ _CompletedHandler)
            {
                m_completedHandler = _CompletedHandler;
            }
        }

        virtual property _ProgressDelegateType^ Progress
        {
            _ProgressDelegateType^ get()
            {
                return m_progressHandler;
            }

            void set(_ProgressDelegateType^ progressHandler)
            {
                m_progressHandler = progressHandler;
            }
        }

    internal:
        void _InvokeProgressHandler(typename _Attributes::_ProgressType progress)
        {
            if (m_progressHandler)
            {
                m_progressHandler->Invoke(this, progress);
            }
        }

        // Called by promise_type::return_value for AsyncOperation/AsyncOperationWithProgress coroutine_traits
        void _InvokeCompletion(_ReturnType result)
        {
            // If m_status is already set to Canceled/Error, retain the state.
            if (Status == Windows::Foundation::AsyncStatus::Started)
                Status = Windows::Foundation::AsyncStatus::Completed;

            if (m_completedHandler)
            {
                m_results = result;
                m_completedHandler->Invoke(this, Status);
            }
        }

        // Called by coroutine_traits::promise_type::set_exception to propagate the error.
        // There is no way to propagate the Exception message.
        void _SetError(Platform::Exception^ ex)
        {
            _AsyncInfoPromiseBase::_SetError(ex);
            _InvokeCompletion(nullptr);
        }

        _ReturnType m_results;
        _CompletionDelegateType^ m_completedHandler;
        _ProgressDelegateType^ m_progressHandler;
    };

    template<typename _ProgressType, typename _ReturnType, bool _TakesProgress>
    struct _AsyncAttributes
    {
    };

    template<typename _ProgressType, typename _ReturnType>
    struct _AsyncAttributes<_ProgressType, _ReturnType, true>
    {
        typedef Windows::Foundation::IAsyncOperationWithProgress<_ReturnType, _ProgressType> _AsyncBaseType;
        typedef Windows::Foundation::AsyncOperationProgressHandler<_ReturnType, _ProgressType> _ProgressDelegateType;
        typedef Windows::Foundation::AsyncOperationWithProgressCompletedHandler<_ReturnType, _ProgressType> _CompletionDelegateType;
        typedef _ReturnType _ReturnType;
        typedef _ProgressType _ProgressType;
        typedef _AsyncOperationWithProgressPromise<_AsyncAttributes<_ProgressType, _ReturnType, true>> _AsyncPromiseType;
    };

    template<typename _ProgressType, typename _ReturnType>
    struct _AsyncAttributes<_ProgressType, _ReturnType, false>
    {
        typedef Windows::Foundation::IAsyncOperation<_ReturnType> _AsyncBaseType;
        typedef Windows::Foundation::AsyncOperationCompletedHandler<_ReturnType> _CompletionDelegateType;
        typedef _ReturnType _ReturnType;
        typedef _ProgressType _ProgressType;
        typedef _AsyncOperationPromise<_AsyncAttributes<_ProgressType, _ReturnType, false>> _AsyncPromiseType;
    };

    template<typename _ProgressType>
    struct _AsyncAttributes<_ProgressType, void, true>
    {
        typedef Windows::Foundation::IAsyncActionWithProgress<_ProgressType> _AsyncBaseType;
        typedef Windows::Foundation::AsyncActionProgressHandler<_ProgressType> _ProgressDelegateType;
        typedef Windows::Foundation::AsyncActionWithProgressCompletedHandler<_ProgressType> _CompletionDelegateType;
        typedef void _ReturnType;
        typedef _ProgressType _ProgressType;
        typedef _AsyncActionWithProgressPromise<_AsyncAttributes<_ProgressType, void, true>> _AsyncPromiseType;
    };

    template<typename _ProgressType>
    struct _AsyncAttributes<_ProgressType, void, false>
    {
        typedef Windows::Foundation::IAsyncAction _AsyncBaseType;
        typedef Windows::Foundation::AsyncActionCompletedHandler _CompletionDelegateType;
        typedef void _ReturnType;
        typedef _ProgressType _ProgressType;
        typedef _AsyncActionPromise<_AsyncAttributes<_ProgressType, void, false>> _AsyncPromiseType;
    };

    template <typename TPromise, typename TProgress>
    class await_progress_reporter;

    template <typename TProgress>
    class await_progress_reporter<Windows::Foundation::IAsyncActionWithProgress<TProgress>, TProgress>
    {
    public:
        await_progress_reporter(Windows::Foundation::IAsyncActionWithProgress<TProgress>^ promise)
        {
            m_promise = promise;
        }

        void report_progress(TProgress progressval)
        {
            static_cast<_AsyncActionWithProgressPromise<_AsyncAttributes<TProgress, void, true>> ^>(m_promise)->_InvokeProgressHandler(progressval);
        }

        bool await_ready()
        {
            return true;
        }

        void await_suspend(std::experimental::coroutine_handle<>)
        {
        }

        await_progress_reporter<Windows::Foundation::IAsyncActionWithProgress<TProgress>, TProgress> await_resume()
        {
            return *this;
        }
    private:
        Windows::Foundation::IAsyncActionWithProgress<TProgress>^ m_promise;
    };


    template <typename TResult, typename TProgress>
    class await_progress_reporter<Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>, TProgress>
    {
    public:
        await_progress_reporter(Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>^ promise)
        {
            m_promise = promise;
        }

        void report_progress(TProgress progressval)
        {
            static_cast<_AsyncOperationWithProgressPromise<_AsyncAttributes<TProgress, TResult, true>> ^>(m_promise)->_InvokeProgressHandler(progressval);
        }

        bool await_ready()
        {
            return true;
        }

        void await_suspend(std::experimental::coroutine_handle<>)
        {
        }

        await_progress_reporter<Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>, TProgress> await_resume()
        {
            return *this;
        }

    private:
        Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>^ m_promise;
    };

    //
    // await on get_progress_reporter to get a handle to the await_progress_reporter 
    // Applications can use await_progress_reporter::report_progress for progress reporting.
    // The await will obtain a handle to the coroutine s promise (which is the _AsyncActionWithProgressPromise or _AsyncOperationWithProgressPromise  
    // and can invoke the corresponding progress handler
    // Example:
    // auto pr = co_await get_progress_reporter<double>();
    // pr.report_progress(98.0);
    //
    struct get_progress_reporter 
    {
    };

    template<typename _Attributes>
    struct _coroutine_promise_base
    {
        typename _Attributes::_AsyncPromiseType^ result;

        _coroutine_promise_base()
        {
            result = ref new typename _Attributes::_AsyncPromiseType();
        }

        std::experimental::suspend_never initial_suspend() const
        {
            return {};
        }

        std::experimental::suspend_never final_suspend() const
        {
            return {};
        }

        typename _Attributes::_AsyncBaseType^ get_return_object()
        {
            return result;
        }

        void set_exception(const std::exception_ptr &eptr)
        {
            try
            {
                if (eptr)
                {
                    std::rethrow_exception(eptr);
                }
            }
            catch (Platform::Exception^ ex)
            {
                result->_SetError(ex);
            }
            catch (...)
            {
                Platform::Exception^ ex = ref new Platform::Exception(E_FAIL);
                result->_SetError(ex);
            }
        }
    };

    template<typename _Attributes>
    struct _coroutine_promise_void :_coroutine_promise_base<_Attributes>
    {
        void return_void()
        {
            result->_InvokeCompletion();
        }
    };

    template<typename _Attributes>
    struct _coroutine_promise_void_with_progress : _coroutine_promise_void<_Attributes>
    {
        await_progress_reporter<typename _Attributes::_AsyncBaseType, typename _Attributes::_ProgressType> await_transform(get_progress_reporter)
        {
            return{ result };
        }

        template <typename _Uty>
        _Uty && await_transform(_Uty &&_Whatever)
        {
            return _STD forward<_Uty>(_Whatever);
        }
    };

    template<typename _Attributes>
    struct _coroutine_promise_value :_coroutine_promise_base<_Attributes>
    {
        void return_value(const typename _Attributes::_ReturnType &_Val)
        {
            result->_InvokeCompletion(_Val);
        }
    };

    template<typename _Attributes>
    struct _coroutine_promise_value_with_progress : _coroutine_promise_value<_Attributes>
    {
        await_progress_reporter<typename _Attributes::_AsyncBaseType, typename _Attributes::_ProgressType> await_transform(get_progress_reporter)
        {
            return{ result };
        }

        template <typename _Uty>
        _Uty && await_transform(_Uty &&_Whatever)
        {
            return _STD forward<_Uty>(_Whatever);
        }
    };
}

template <typename T>
auto operator * (concurrency::cancellation_token ct, T^ async)
{
	ct.register_callback([=]()
    {
        async->Cancel();
    });
    return async;
}

namespace std
{
    namespace experimental
    {
        // Specialization of coroutine_traits for functions with IAsyncAction return type.
        template <typename... _TArgs>
        struct coroutine_traits<Windows::Foundation::IAsyncAction^, _TArgs...>
        {
            using promise_type = typename winrt_await_adapters::_coroutine_promise_void<winrt_await_adapters::_AsyncAttributes<void, void, false>>;
        };

        // Specialization of coroutine_traits for functions with IAsyncActionWithProgress return type.
        template <typename TProgress, typename... _TArgs>
        struct std::experimental::coroutine_traits<Windows::Foundation::IAsyncActionWithProgress<TProgress>^, _TArgs...>
        {
            using promise_type = typename winrt_await_adapters::_coroutine_promise_void_with_progress<winrt_await_adapters::_AsyncAttributes<TProgress, void, true>>;
        };

        // Specialization of coroutine_traits for functions with IAsyncOperationWithProgress return type.
        template <typename TProgress, typename TResult, typename... _TArgs>
        struct std::experimental::coroutine_traits<Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>^, _TArgs...>
        {
            using promise_type = typename winrt_await_adapters::_coroutine_promise_value_with_progress<winrt_await_adapters::_AsyncAttributes<TProgress, TResult, true>>;
        };

        // Specialization of coroutine_traits for functions with IAsyncOperation return type.
        template <typename TResult, typename... _TArgs>
        struct std::experimental::coroutine_traits<Windows::Foundation::IAsyncOperation<TResult>^, _TArgs...>
        {
            using promise_type = typename winrt_await_adapters::_coroutine_promise_value<winrt_await_adapters::_AsyncAttributes<void, TResult, false>>;
        };
    }
}
#endif
#endif
