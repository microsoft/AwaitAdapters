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

namespace winrt_await_adapters
{
#ifdef __cplusplus_winrt

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
        _AsyncInfoPromiseBase():
            m_status(Windows::Foundation::AsyncStatus::Started)
        {
            m_id = GetNextAsyncId();
            m_errorcode.Value = S_OK;
        }

        void set_error(Platform::Exception^ ex)
        {
            Status = Windows::Foundation::AsyncStatus::Error;
            m_errorcode.Value = ex->HResult;
        }

        static unsigned int GetNextAsyncId()
        {
            static long s_asyncId = 0;
            return static_cast<unsigned int>(_InterlockedIncrement(&s_asyncId));
        }

        Windows::Foundation::HResult m_errorcode;
        Windows::Foundation::AsyncStatus m_status;
        unsigned int m_id;
    };

    template <typename _Attributes>
    ref class _AsyncActionPromise :public _AsyncInfoPromiseBase, public _Attributes::_AsyncBaseType
    {
    public:
        virtual void GetResults()
        {
        }

        // Get/set the method that handles the action completed notification.
        virtual property typename _Attributes::_CompletionDelegateType^ Completed
        {
            typename _Attributes::_CompletionDelegateType^ get()
            {
                return m_completedhandler;
            }

            void set(typename _Attributes::_CompletionDelegateType^ _CompleteHandler)
            {
                m_completedhandler = _CompleteHandler;
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

    internal:
        // Called by promise_type::return_void for AsyncActionPromise coroutine_traits
        void invoke_completion()
        {
            // If Status is already set to Canceled/Error, retain the state.
            if (Status == Windows::Foundation::AsyncStatus::Started)
                Status = Windows::Foundation::AsyncStatus::Completed;

            if (m_completedhandler)
            {
                m_completedhandler->Invoke(this, Status);
            }
        }

        // Called by coroutine_traits::promise_type::set_exception to propagate the error.
        // There is no way to propagate the Exception message.
        void set_error(Platform::Exception^ ex)
        {
            _AsyncInfoPromiseBase::set_error(ex);
            invoke_completion();
        }

        typename _Attributes::_CompletionDelegateType^  volatile  m_completedhandler;
    };

    template <typename _Attributes>
    ref class _AsyncActionWithProgressPromise: public _AsyncInfoPromiseBase, public _Attributes::_AsyncBaseType
    {
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
            virtual unsigned int get() override
            {
                return _AsyncInfoPromiseBase::Id;
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

        virtual property typename _Attributes::_ProgressDelegateType^ Progress
        {
            typename _Attributes::_ProgressDelegateType^ get()
            {
                return m_progresshandler;
            }

            void set(typename _Attributes::_ProgressDelegateType^ progresshandler)
            {
                m_progresshandler = progresshandler;
            }
        }

        // Get/set the method that handles the action/operation completed notification.
        virtual property typename _Attributes::_CompletionDelegateType^ Completed
        {
            typename _Attributes::_CompletionDelegateType^ get()
            {
                return m_completedhandler;
            }

            void set(typename _Attributes::_CompletionDelegateType^ _CompleteHandler)
            {
                m_completedhandler = _CompleteHandler;
            }
        }

    internal:
        void invoke_progress_handler(typename _Attributes::_ProgressType progress)
        {
            if (m_progresshandler)
            {
                m_progresshandler->Invoke(this, progress);
            }
        }

        // Called by promise_type::return_void for AsyncAction/AsyncActionWithProgress coroutine_traits
        void invoke_completion()
        {
            // If m_status is already set to Canceled/Error, retain the state.
            if (Status == Windows::Foundation::AsyncStatus::Started)
                Status = Windows::Foundation::AsyncStatus::Completed;

            if (m_completedhandler)
            {
                m_completedhandler->Invoke(this, Status);
            }
        }

        // Called by coroutine_traits::promise_type::set_exception to propagate the error.
        // There is no way to propagate the Exception message.
        void set_error(Platform::Exception^ ex)
        {
            _AsyncInfoPromiseBase::set_error(ex);
            invoke_completion();
        }

        typename _Attributes::_ProgressDelegateType^  volatile  m_progresshandler;
        typename _Attributes::_CompletionDelegateType^  volatile  m_completedhandler;
    };

    template <typename _Attributes>
    ref class _AsyncOperationPromise :public _AsyncInfoPromiseBase, public _Attributes::_AsyncBaseType
    {
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
            virtual unsigned int get() override
            {
                return _AsyncInfoPromiseBase::Id;
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

        virtual typename _Attributes::_ReturnType GetResults()
        {
            return m_results;
        }

        // Get/set the method that handles the action/operation completed notification.
        virtual property typename _Attributes::_CompletionDelegateType^ Completed
        {
            typename _Attributes::_CompletionDelegateType^ get()
            {
                return m_completedhandler;
            }

            void set(typename _Attributes::_CompletionDelegateType^ _CompleteHandler)
            {
                m_completedhandler = _CompleteHandler;
            }
        }

    internal:
        // Called by promise_type::return_value for AsyncOperation/AsyncOperationWithProgress coroutine_traits
        void invoke_completion(typename _Attributes::_ReturnType result)
        {
            // If Status is already set to Canceled/Error, retain the state.
            if (Status == Windows::Foundation::AsyncStatus::Started)
                Status = Windows::Foundation::AsyncStatus::Completed;

            if (m_completedhandler)
            {
                m_results = result;
                m_completedhandler->Invoke(this, Status);
            }
        }

        // Called by coroutine_traits::promise_type::set_exception to propagate the error.
        // There is no way to propagate the Exception message.
        void set_error(Platform::Exception^ ex)
        {
            _AsyncInfoPromiseBase::set_error(ex);
            invoke_completion(nullptr);
        }

        typename _Attributes::_ReturnType m_results;
        typename _Attributes::_CompletionDelegateType^  volatile  m_completedhandler;
    };
    
    template <typename _Attributes>
    ref class _AsyncOperationWithProgressPromise :public _AsyncInfoPromiseBase, public _Attributes::_AsyncBaseType
    {
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
            virtual unsigned int get() override
            {
                return _AsyncInfoPromiseBase::Id;
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

        virtual typename _Attributes::_ReturnType GetResults()
        {
            return m_results;
        }

        // Get/set the method that handles the action/operation completed notification.
        virtual property typename _Attributes::_CompletionDelegateType^ Completed
        {
            typename _Attributes::_CompletionDelegateType^ get()
            {
                return m_completedhandler;
            }

            void set(typename _Attributes::_CompletionDelegateType^ _CompleteHandler)
            {
                m_completedhandler = _CompleteHandler;
            }
        }

        virtual property typename _Attributes::_ProgressDelegateType^ Progress
        {
            typename _Attributes::_ProgressDelegateType^ get()
            {
                return m_progresshandler;
            }

            void set(typename _Attributes::_ProgressDelegateType^ progresshandler)
            {
                m_progresshandler = progresshandler;
            }
        }

    internal:
        void invoke_progress_handler(typename _Attributes::_ProgressType progress)
        {
            if (m_progresshandler)
            {
                m_progresshandler->Invoke(this, progress);
            }
        }

        // Called by promise_type::return_value for AsyncOperation/AsyncOperationWithProgress coroutine_traits
        void invoke_completion(typename _Attributes::_ReturnType result)
        {
            // If m_status is already set to Canceled/Error, retain the state.
            if (Status == Windows::Foundation::AsyncStatus::Started)
                Status = Windows::Foundation::AsyncStatus::Completed;

            if (m_completedhandler)
            {
                m_results = result;
                m_completedhandler->Invoke(this, Status);
            }
        }

        // Called by coroutine_traits::promise_type::set_exception to propagate the error.
        // There is no way to propagate the Exception message.
        void set_error(Platform::Exception^ ex)
        {
            _AsyncInfoPromiseBase::set_error(ex);
            invoke_completion(nullptr);
        }

        typename _Attributes::_ReturnType m_results;
        typename _Attributes::_CompletionDelegateType^  volatile  m_completedhandler;
        typename _Attributes::_ProgressDelegateType^  volatile  m_progresshandler;
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
    class await_progress_reporter
    {
    public:
        await_progress_reporter(TPromise^ promise)
        {
            coroPromise = promise;
        }

        void report_progress(TProgress progressval)
        {
            ((TPromise<TProgress> ^)myProgress)->invoke_progress_handler(progressval);
        }

        TPromise^ coroPromise;
    };

    template <typename TProgress>
    class await_progress_reporter<Windows::Foundation::IAsyncActionWithProgress<TProgress>, TProgress>
    {
    public:
        await_progress_reporter(Windows::Foundation::IAsyncActionWithProgress<TProgress>^ promise)
        {
            coroPromise = promise;
        }
        void report_progress(TProgress progressval)
        {
            ((_AsyncActionWithProgressPromise<_AsyncAttributes<TProgress, void, true>> ^)coroPromise)->invoke_progress_handler(progressval);
        }

        Windows::Foundation::IAsyncActionWithProgress<TProgress>^ coroPromise;
    };


    template <typename TResult, typename TProgress>
    class await_progress_reporter<Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>, TProgress>
    {
    public:
        await_progress_reporter(Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>^ promise)
        {
            coroPromise = promise;
        }
        void report_progress(TProgress progressval)
        {
            ((_AsyncOperationWithProgressPromise<_AsyncAttributes<TProgress, TResult, true>> ^)coroPromise)->invoke_progress_handler(progressval);
        }

        Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>^ coroPromise;
    };

    //
    // await on get_progress_reporter<IAsyncType, ProgressType> to get a handle to the await_progress_reporter 
    // Applications can use await_progress_reporter::report_progress for progress reporting.
    // The await will obtain a handle to the coroutine s promise (which is the _AsyncActionWithProgressPromise or _AsyncOperationWithProgressPromise  
    // and can invoke the corresponding progress handler
    //
    template <typename TPromise, typename TProgress>
    struct get_progress_reporter {};

    template <typename TPromise, typename TProgress>
    auto operator await (get_progress_reporter<TPromise, TProgress>)
    {
        struct Awaiter
        {
            using Promise = typename std::experimental::coroutine_traits<TPromise^>::promise_type;
            Promise* promise;
            bool await_ready()
            {
                return false;
            }

            void await_suspend(std::experimental::coroutine_handle<Promise> h)
            {
                promise = &h.promise();
                h();
            }

            await_progress_reporter<TPromise, TProgress> await_resume()
            {
                return await_progress_reporter<TPromise, TProgress>(promise->result);
            };
        };

        return Awaiter{};
    }
}

namespace std
{
    namespace experimental
    {
        template<typename _Attributes>
        struct _coroutine_traits
        {
            struct promise_type
            {
                typename _Attributes::_AsyncPromiseType^ result;

                promise_type()
                {
                    result = ref new typename _Attributes::_AsyncPromiseType();
                }

                void return_void()
                {
                    result->invoke_completion();
                }

                bool initial_suspend() const 
                { 
                    return false; 
                }
             
                bool final_suspend() const 
                { 
                    return false; 
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
                        result->set_error(ex);
                    }
                    catch (...)
                    {
                        Platform::Exception^ ex = ref new Platform::Exception(E_FAIL);
                        result->set_error(ex);
                    }
                }
            };
        };

        template<typename _Attributes>
        struct _coroutine_traits_with_ret
        {
            struct promise_type
            {
                typename _Attributes::_AsyncPromiseType^ result;

                promise_type()
                {
                    result = ref new typename _Attributes::_AsyncPromiseType();
                }

                void return_value(const typename _Attributes::_ReturnType &_Val)
                {
                    result->invoke_completion(_Val);
                }

                bool initial_suspend() const 
                { 
                    return false; 
                }
                
                bool final_suspend() const 
                { 
                    return false; 
                }

                typename _Attributes::_AsyncBaseType^ get_return_object()
                {
                    return result;
                }

                void set_exception(const std::exception_ptr &eptr)
                {
                    // Handle eptr
                    try
                    {
                        if (eptr)
                        {
                            std::rethrow_exception(eptr);
                        }
                    }
                    catch (Platform::Exception^ ex)
                    {
                        result->set_error(ex);
                    }
                    catch (...)
                    {
                        Platform::Exception^ ex = ref new Platform::Exception(E_FAIL);
                        result->set_error(ex);
                    }
                }
            };
        };

        // Specialization of coroutine_traits for functions with IAsyncAction return type.
        template <typename... _TArgs>
        struct coroutine_traits<Windows::Foundation::IAsyncAction^, _TArgs...> :
            _coroutine_traits<winrt_await_adapters::_AsyncAttributes<void, void, false>>
        {
            using promise_type = typename _coroutine_traits::promise_type;
        };

        // Specialization of coroutine_traits for functions with IAsyncActionWithProgress return type.
        template <typename TProgress, typename... _TArgs>
        struct std::experimental::coroutine_traits<Windows::Foundation::IAsyncActionWithProgress<TProgress>^, _TArgs...> :
            _coroutine_traits<winrt_await_adapters::_AsyncAttributes<TProgress, void, true>>
        {
            using promise_type = typename _coroutine_traits::promise_type;
        };

        // Specialization of coroutine_traits for functions with IAsyncOperationWithProgress return type.
        template <typename TProgress, typename TResult, typename... _TArgs>
        struct std::experimental::coroutine_traits<Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>^, _TArgs...> :
            _coroutine_traits_with_ret<winrt_await_adapters::_AsyncAttributes<TProgress, TResult, true>>
        {
            using promise_type = typename _coroutine_traits_with_ret::promise_type;
        };

        // Specialization of coroutine_traits for functions with IAsyncOperation return type.
        template <typename TResult, typename... _TArgs>
        struct std::experimental::coroutine_traits<Windows::Foundation::IAsyncOperation<TResult>^, _TArgs...> :
            _coroutine_traits_with_ret<winrt_await_adapters::_AsyncAttributes<void, TResult, false>>
        {
            using promise_type = typename _coroutine_traits_with_ret::promise_type;
        };
    }

#endif
}

#endif
