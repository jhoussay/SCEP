// Copyright 2008, Google Inc.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Google Inc. nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <assert.h>
#include <exdispid.h>
#include "gears/base/ie/browser_listener.h"

// Some macros defined here to keep changes from original source to a low roar.
// See //depot/googleclient/bar/common/(debugbase|macros).h for original
// definitions.
#define ASSERT(expr) assert(expr);

// Checks for HRESULT and if it fails returns. The macro will ASSERT in debug.
#define CHK(cmd)              \
    do                        \
    {                         \
        HRESULT __hr = (cmd); \
        if (FAILED(__hr))     \
        {                     \
            ASSERT(false);    \
            return __hr;      \
        }                     \
    } while (0);

// Verify an operation succeeded and ASSERT on failure. Only asserts in DEBUG.
#ifdef DEBUG
#define VERIFY(expr) ASSERT(expr)
#define VERIFYHR(expr) ASSERT(SUCCEEDED(expr))
#else
#define VERIFY(expr) expr
#define VERIFYHR(expr) expr
#endif

#undef ATLTRACE
#define ATLTRACE __noop

// Attach listener to a browser
void BrowserListener::Init(IWebBrowser2 *browser)
{
    browser_ = browser;
    ASSERT(!invoker_);
    invoker_ = new DispatchInvoke<BrowserListener>;
    VERIFY(invoker_);
    if (!invoker_)
        return;
    invoker_->Init(this, &BrowserListener::Invoke);
    VERIFY(SUCCEEDED(invoker_->Connect(browser, DIID_DWebBrowserEvents2)));
}

// Detach listener from a browser
void BrowserListener::Teardown()
{
    if (invoker_)
    {
        invoker_->Disconnect();
        invoker_->Release();
        invoker_ = NULL;
    }
}

// Helper method to get WebBrowser Busy property.
bool BrowserListener::BrowserBusy()
{
    VARIANT_BOOL busy;
    if (browser_ == NULL || FAILED(browser_->get_Busy(&busy)))
    {
        ASSERT(!"browser_->get_Busy(&busy) failed");
        return false;
    }
    return !!busy;
}

// Return true if we're inside a OnPageDownloadBegin/OnPageDownloadComplete pair
bool BrowserListener::IsDownloading()
{
    return download_depth_ > 0 || BrowserBusy();
}

// Dispatcher for IE disp events
HRESULT __stdcall BrowserListener::Invoke(DISPID event, const IID &riid,
                                          LCID /*lcid*/, WORD /*wFlags*/, DISPPARAMS *params, VARIANT * /*pVarResult*/,
                                          EXCEPINFO * /*pExcepInfo*/, UINT * /*puArgErr*/)
{
    if (IID_NULL != riid)
        return DISP_E_UNKNOWNINTERFACE;

    if (!params)
        return DISP_E_PARAMNOTOPTIONAL;

    VARIANTARG *args = params->rgvarg;

    switch (event)
    {
    case DISPID_BEFORENAVIGATE2:
    {
        // The parameters for this DISPID are as follows:
        // [0]: Cancel flag  - VT_BYREF|VT_BOOL
        // [1]: HTTP headers - VT_BYREF|VT_VARIANT
        // [2]: Address of HTTP POST data  - VT_BYREF|VT_VARIANT
        // [3]: Target frame name - VT_BYREF|VT_VARIANT
        // [4]: Option flags - VT_BYREF|VT_VARIANT
        // [5]: URL to navigate to - VT_BYREF|VT_VARIANT
        // [6]: An object that evaluates to the top-level or frame
        //      WebBrowser object corresponding to the event.
        CComPtr<IDispatch> window_disp = args[6].pdispVal;
        CComPtr<IWebBrowser2> window;
        window_disp.QueryInterface(&window);
        CString url = args[5].pvarVal->bstrVal;
        bool cancel = false;

        ATLTRACE(
            _T("BrowserListener:DISPID_BEFORENAVIGATE2:Url: %s  Depth: %d\n"),
            url, download_depth_);

        OnBeforeNavigate2(window, url, &cancel);
        assert(!cancel);
        // TODO(michaeln): We're seeing some crashes at this assignment, since
        // gears never attempts to cancel navigations and the default is to not
        // cancel, we can skip this assignment. The crash may be related to
        // non-thread safe GURL initialization which is also addressed in this CL.
        // *(args[0].pboolVal) = cancel ? VARIANT_TRUE : VARIANT_FALSE;

        if (!IsDownloading())
            OnPageDownloadBegin(url);
        download_depth_++;
        break;
    }

    case DISPID_DOCUMENTCOMPLETE:
    {
        // Increment the number of navigations if we're browsing to a new page.
        CComBSTR current_url;
        VERIFYHR(browser_->get_LocationURL(&current_url));
        ATLTRACE(
            _T("BrowserListener:DISPID_DOCUMENTCOMPLETE:Url: %s\n"),
            current_url.m_str);
        // [0]: URL - VT_BYREF|VT_VARIANT
        // [1]: IDispatch interface of browser/frame
        CComPtr<IDispatch> window_disp = args[1].pdispVal;
        CComPtr<IWebBrowser2> window;
        window_disp.QueryInterface(&window);
        CString url = args[0].pvarVal->bstrVal;
        OnDocumentComplete(window, url);

        // download_depth_ may already be 0 if we received the ProgressMax == 0
        // event before this one.
        if (download_depth_ > 0)
        {
            --download_depth_;
            if (!IsDownloading())
                OnPageDownloadComplete();
        }
        break;
    }

    case DISPID_DOWNLOADBEGIN:
        ATLTRACE(
            _T("BrowserListener:DISPID_DOWNLOADBEGIN\n"));
        OnDownloadBegin();
        if (!download_depth_)
        {
            const CString url_empty;
            OnPageDownloadBegin(url_empty);
        }
        break;

    case DISPID_DOWNLOADCOMPLETE:
        ATLTRACE(
            _T("BrowserListener:DISPID_DOWNLOADCOMPLETE\n"));
        OnDownloadComplete();
        if (!download_depth_)
            OnPageDownloadComplete();
        break;

    case DISPID_NAVIGATECOMPLETE2:
    {
        // [0]: URL - VT_BYREF|VT_VARIANT
        // [1]: IDispatch interface of browser/frame
        CComPtr<IDispatch> window_disp = args[1].pdispVal;
        CComPtr<IWebBrowser2> window;
        window_disp.QueryInterface(&window);
        CString url = args[0].pvarVal->bstrVal;
        ATLTRACE(
            _T("BrowserListener:DISPID_NAVIGATECOMPLETE2:Url: %s\n"), url);
        OnNavigateComplete2(window, url);
        break;
    }

    case DISPID_PROGRESSCHANGE:
    {
        LONG progress = args[1].lVal;
        LONG progress_max = args[0].lVal;
        ATLTRACE(
            _T("BrowserListener:DISPID_PROGRESSCHANGE: Progress: %d   Max: %d\n"),
            progress, progress_max);
        OnProgressChange(progress, progress_max);
        if (progress == 0 && progress_max == 0 && download_depth_ > 0)
        {
            // When we receive a ProgressChange event with progressMax == 0,
            // a navigation is complete and download_depth_ is usually already 0.
            // If it isn't, we reset the navigation count to 0 here; we haven't
            // received a DownloadComplete event and we might not receive one
            // at all, e.g. if the user presses Stop during a navigation.
            download_depth_ = 0;
            if (!BrowserBusy())
                OnPageDownloadComplete();
        }
        break;
    }
    }
    return S_OK;
}
