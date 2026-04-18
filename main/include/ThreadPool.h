/****************************** Module Header ******************************\
* 2025: Some modifications by Vu Tung Lam for this to work with C++17.
*
* ***************************************************************************
*s
* Module Name:  ThreadPool.h
* Project:      CppWindowsService
* Copyright (c) Microsoft Corporation.
* 
* The class was designed by Kenny Kerr. It provides the ability to queue 
* simple member functions of a class to the Windows thread pool.
* 
* Using the thread pool is simple and feels natural in C++. 
* 
* class CSampleService
* {
* public:
* 
*     void AsyncRun()
*     {
*         CThreadPool::QueueUserWorkItem(&Service::Run, this);
*     }
* 
*     void Run()
*     {
*         // Some lengthy operation
*     }
* };
* 
* Kenny Kerr spends most of his time designing and building distributed 
* applications for the Microsoft Windows platform. He also has a particular 
* passion for C++ and security programming. Reach Kenny at 
* http://weblogs.asp.net/kennykerr/ or visit his Web site: 
* http://www.kennyandkarin.com/Kenny/.
* 
* This source is subject to the Microsoft Public License.
* See http://www.microsoft.com/en-us/openness/resources/licenses.aspx#MPL.
* All other rights reserved.
* 
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, 
* EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED 
* WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#pragma once

#include <memory>
#include <windows.h>
#include <threadpoolapiset.h>

class CThreadPool
{
public:

    template <typename T>
    static void QueueUserWorkItem(void (T::*function)(void), 
        T *object, ULONG flags = WT_EXECUTELONGFUNCTION)
    {
        typedef std::pair<void (T::*)(), T *> CallbackType;
        CallbackType* p = new CallbackType(function, object);

        if (0 == ::QueueUserWorkItem(ThreadProc<T>, p, flags))
        {
            delete p;
            throw GetLastError();
        }
    }

private:

    template <typename T>
    static DWORD WINAPI ThreadProc(PVOID context)
    {
        typedef std::pair<void (T::*)(), T *> CallbackType;

        std::unique_ptr<CallbackType> p(static_cast<CallbackType *>(context));

        (p->second->*p->first)();
        return 0;
    }
};