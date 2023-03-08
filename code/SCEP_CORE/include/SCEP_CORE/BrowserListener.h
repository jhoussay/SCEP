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



// /!\ Note
// Modifications from jhoussay on 01/03/2023 in order to get rid of atl headers



#include <string>

#include <exdisp.h>
#include <shldisp.h>

#include <SCEP_CORE/SCEP_CORE.h>

#ifndef GEARS_BASE_IE_BROWSER_LISTENER_H__
#define GEARS_BASE_IE_BROWSER_LISTENER_H__

// Use this calss to capture all the IDispatch invokes.
template <class T>
class DispatchInvoke : public IDispatch
{
public:
    //
    // typedef for the function to call whenever
    // DispatchInvoke::Invoke gets called
    //
    typedef HRESULT (_stdcall T::*InvokeMethodPtr)(
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS *pDispParams,
        /* [out] */ VARIANT *pVarResult,
        /* [out] */ EXCEPINFO *pExcepInfo,
        /* [out] */ UINT *puArgErr);

    typedef HRESULT (_stdcall T::*GetIDSMethodPtr)(
        /* [in] */ REFIID riid,
        /* [size_is][in] */ LPOLESTR *rgszNames,
        /* [in] */ UINT cNames,
        /* [in] */ LCID lcid,
        /* [size_is][out] */ DISPID *rgDispId);

    // Constructor takes an object pointer to invoke and the
    // method to invoke
    DispatchInvoke() : owner_(NULL),
                       invoke_(NULL),
                       getids_(NULL),
                       point_(NULL),
                       reference_count_(1)
    {
    }

    // getids can be NULL, if dispatch interface doesn't implement it
    void Init(T *owner, InvokeMethodPtr invoke, GetIDSMethodPtr getids = NULL)
    {
        owner_ = owner;
        invoke_ = invoke;
        getids_ = getids;
    }

    ~DispatchInvoke()
    {
        invoke_ = NULL;
        owner_ = NULL;
        getids_ = NULL;
    }

    //
    // IUnknown
    //

    HRESULT _stdcall QueryInterface(REFIID riid, void **ppv)
    {
        if (IsEqualIID(riid, IID_IUnknown) ||
            IsEqualIID(riid, IID_IDispatch))
        {
            *ppv = static_cast<IDispatch *>(this);
        }
        else
        {
            *ppv = 0;
            return E_NOINTERFACE;
        }
        this->AddRef();
        return S_OK;
    }

    ULONG _stdcall AddRef()
    {
        return ++reference_count_;
    }

    ULONG _stdcall Release()
    {
        ULONG count = --reference_count_;
        if (count == 0)
            delete this;
        return count;
    }

    //
    // IDispatch (cut and paste from system include file OAIdl.h)
    //

    virtual HRESULT _stdcall GetTypeInfoCount(UINT *pctinfo)
    {
        *pctinfo = 0;
        return S_OK;
    }

    virtual HRESULT _stdcall GetTypeInfo(UINT /*iTInfo*/, LCID /*lcid*/,
                                         ITypeInfo **ppTInfo)
    {
        *ppTInfo = 0;
        return E_NOTIMPL;
    }

    virtual HRESULT _stdcall GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames,
                                           UINT cNames, LCID lcid, DISPID *rgDispId)
    {
        if (getids_ == NULL)
        {
            *rgDispId = NULL;
            return E_NOTIMPL;
        }
        return (owner_->*getids_)(riid, rgszNames, cNames, lcid, rgDispId);
    }

    virtual HRESULT _stdcall Invoke(
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS *pDispParams,
        /* [out] */ VARIANT *pVarResult,
        /* [out] */ EXCEPINFO *pExcepInfo,
        /* [out] */ UINT *puArgErr)
    {
        return (owner_->*invoke_)(dispIdMember, riid, lcid, wFlags, pDispParams,
                                  pVarResult, pExcepInfo, puArgErr);
    }

    virtual HRESULT Connect(IUnknown *generic_interface, REFIID riid)
    {
        IConnectionPointContainer *container = NULL;
        IConnectionPoint *point = NULL;

        if (!SUCCEEDED(generic_interface->QueryInterface(
                IID_IConnectionPointContainer,
                reinterpret_cast<void **>(&container))))
        {
            return E_FAIL;
        }

        if (SUCCEEDED(container->FindConnectionPoint(riid, &point)))
        {
            if (!SUCCEEDED(point->Advise(static_cast<IDispatch *>(this), &cookie_)))
            {
                point->Release();
                point = 0;
            }
        }

        point_ = point;
        container->Release();
        return point ? S_OK : E_FAIL;
    }

    bool Connected() const { return point_; }

    virtual void Disconnect()
    {
        if (point_)
        {
            point_->Unadvise(cookie_);
            point_->Release();
            point_ = 0;
        }
    }

protected:
    ULONG reference_count_;
    InvokeMethodPtr invoke_;
    GetIDSMethodPtr getids_;
    T *owner_;
    DWORD cookie_;
    IConnectionPoint *point_;
};

// A BrowserListener listens to events defined by the DWebBrowserEvents2
// dispinterface and converts IDispatch calls to convenient C++ method calls.
// It also reports its own events OnUpdateBegin() and OnUpdateComplete().
// We define an update to be any navigation or refresh, either of the entire
// page or of any frame in it.  OnUpdateBegin() fires once before each update;
// OnUpdateComplete() fires once after an update has completed.
//
// How to implement OnUpdateBegin()/OnUpdateComplete()?  Let's start by
// considering just OnUpdateComplete().  There are a number of different events
// we can possibly use; see browser_events.html. These events include
//
// - DocumentComplete
// - DownloadComplete
// - ReadyState == READYSTATE_COMPLETE (the WebBrowser's IPropertyNotifySink
//   outgoing interface can notify us when this property changes)
// - ProgressChange with Progress == -1
// - ProgressChange with Progress == ProgressMax == 0
//
// Unfortunately none of these events in isolation will give us the behavior
// we want.  Here's why:
//
// - DocumentComplete isn't fired when the user reloads a page or frame.
//
// - DownloadComplete will always occur at least once during a navigation
// sequence, but it occurs a bit too often; for example, an extra
// DownloadBegin/DownloadComplete pair occurs at the beginning of
// every navigation.
//
// - When the user refreshes a frame, the ReadyState property doesn't seem
// to change either on the top-level WebBrowser object or on the frame's
// WebBrowser object.  And we don't seem to receive ReadyState events at all
// during certain navigations in Internet Explorer 5, e.g. when reloading
// a page.
//
// - ProgressChange with Progress == -1 occurs just once during most
// navigations, but occurs twice occasionally, e.g. when a page contains an
// onload event which redirects; see browser_events.html.
// And unfortunately on pages with frames this event sometimes occurs before
// all inline frames have finished loading.
//
// - ProgressChange with Progress == ProgressMax == 0 occurs at the
// end of every navigation.  It also occurs twice once in a while, as
// when a reload times out.  Unfortunately this event generally seems to occur
// only after a noticeable delay has elapsed after a page has finished loading;
// when we tried to use this event to decide when to look for autofillable
// fields in the Toolbar, there was a visible delay after loading a page
// before fields became highlighted.
//
// So we need to use a combination of events.  DownloadBegin and
// DownloadComplete events occur during every refresh or navigation, & are never
// nested. Each BeforeNavigate2 (almost) always has a corresponding
// DocumentComplete event; there may be several BeforeNavigate2/DocumentComplete
// pairs active simulaneously, as when a page has frames.  So we keep a count of
// nested BeforeNavigate2() events; we fire OnUpdateComplete() whenever the
// navigate count is zero and we're not inside a DownloadBegin/DownloadComplete.
// One catch is that sometimes a DocumentComplete event will fail to arrive,
// as when the user presses Stop during a navigation.  So whenever we see a
// ProgressChange event with ProgressMax == 0, which occurs at the end of
// every navigation, we reset the navigate count to zero if it's not zero
// already.
//
// Similarly, we fire OnUpdateBegin() whenever we see a BeforeNavigate2 or
// DownloadBegin event and we're not already navigating or downloading.
// Note:
// OnUpdateBegin() and OnUpdateComplete() are called OnPageDownloadBegin()
// and OnPageDownloadComplete() respectively in the code below.
class SCEP_CORE_DLL BrowserListener
{
public:
    BrowserListener() : browserInvoker_(NULL), viewInvoker_(NULL), download_depth_(0) /*, num_navigations_(0)*/ {}
    virtual ~BrowserListener() { Teardown(); }

    // Attach/detach listener to a browser and its view.
    void InitBrowser(IWebBrowser2 *browser); // first
    void InitView(IShellFolderViewDual *view); // second
    void Teardown();

    // Returns true if inside a OnPageDownloadBegin/OnPageDownloadComplete pair.
    bool IsDownloading();

    // Accessors
    IWebBrowser2 *browser() const { return browser_; }
    IShellFolderViewDual *view() const { return view_; }

protected:
    // Helper method to get WebBrowser Busy property.
    bool BrowserBusy();

    // Event handler methods.
    virtual void OnBeforeNavigate2(IWebBrowser2 * /*window*/, const std::wstring & /*url*/,
                                   bool * /*cancel*/) {}
    virtual void OnDocumentComplete(IWebBrowser2 * /*window*/, const std::wstring & /*url*/) {}
    virtual void OnDownloadBegin() {}
    virtual void OnDownloadComplete() {}
    virtual void OnNavigateComplete2(IWebBrowser2 * /*window*/, const std::wstring & /*url*/) {}
    virtual void OnProgressChange(LONG /*progress*/, LONG /*progressMax*/) {}

    virtual void OnSelectionChanged() {}

    // Derivative events build on top of IE events.
    virtual void OnPageDownloadBegin(const std::wstring & /*url*/) {}
    virtual void OnPageDownloadComplete() {}

private:
    HRESULT __stdcall Invoke(DISPID event, const IID &riid,
                             LCID lcid, WORD wFlags, DISPPARAMS *params, VARIANT *pVarResult,
                             EXCEPINFO *pExcepInfo, UINT *puArgErr);

    IWebBrowser2* browser_;                           // The browser this object is attached to
    DispatchInvoke<BrowserListener> *browserInvoker_; // Helper event dispatcher for the browser
    int download_depth_;                              // The depth of downloading request.

    IShellFolderViewDual* view_;                      // The view this object is attached to
    DispatchInvoke<BrowserListener> *viewInvoker_;    // Helper event dispatcher for the view.
};

#endif // GEARS_BASE_IE_BROWSER_LISTENER_H__
