#include <windows.h>
#include <commdlg.h>
#include <streams.h>
#include <initguid.h>
#include <strsafe.h>
#include "KLVDecoderuids.h"
#include "KLVDecoder.h"
#include "myInterface.h"
#include <time.h>
#include <vector>
#include <stdint.h>
#include "KlvParser.hpp"

 
// Setup data

const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
    &MEDIATYPE_NULL,            // Major type
    &MEDIASUBTYPE_NULL          // Minor type
};

const AMOVIESETUP_PIN sudPins =
{
    L"Input",                   // Pin string name
    FALSE,                      // Is it rendered
    FALSE,                      // Is it an output
    FALSE,                      // Allowed none
    FALSE,                      // Likewise many
    &CLSID_NULL,                // Connects to filter
    L"Output",                  // Connects to pin
    1,                          // Number of types
    &sudPinTypes                // Pin information
};

const AMOVIESETUP_FILTER sudDump =
{
	&CLSID_KLVDecoder,                // Filter CLSID
    L"KLV Decoder (MISB ST 0601.8)",                    // String name
    MERIT_DO_NOT_USE,           // Filter merit
    1,                          // Number pins
    &sudPins                    // Pin details
};


//
//  Object creation stuff
//
CFactoryTemplate g_Templates[]= {
	L"KLV Decoder (MISB ST 0601.8)", &CLSID_KLVDecoder, CDump::CreateInstance, NULL, &sudDump
};
int g_cTemplates = 1;


// Constructor

CDumpFilter::CDumpFilter(CDump *pDump,
                         LPUNKNOWN pUnk,
                         CCritSec *pLock,
                         HRESULT *phr) :
						 CBaseFilter(NAME("CDumpFilter"), pUnk, pLock, CLSID_KLVDecoder),
    m_pDump(pDump)
{
}


//
// GetPin
//
CBasePin * CDumpFilter::GetPin(int n)
{
    if (n == 0) {
        return m_pDump->m_pPin;
    } else {
        return NULL;
    }
}


//
// GetPinCount
//
int CDumpFilter::GetPinCount()
{
    return 1;
}
 
STDMETHODIMP CDumpFilter::Stop()
{
    CAutoLock cObjectLock(m_pLock);
	m_pDump->m_running = 0;
	 
    return CBaseFilter::Stop();
}
 
STDMETHODIMP CDumpFilter::Pause()
{
    CAutoLock cObjectLock(m_pLock);
	 

    return CBaseFilter::Pause();
}
  
STDMETHODIMP CDumpFilter::Run(REFERENCE_TIME tStart)
{
    CAutoLock cObjectLock(m_pLock);
  
	m_pDump->m_running = 1;
  
    return CBaseFilter::Run(tStart);
}


//
//  Definition of CDumpInputPin
//
CDumpInputPin::CDumpInputPin(CDump *pDump,
                             LPUNKNOWN pUnk,
                             CBaseFilter *pFilter,
                             CCritSec *pLock,
                             CCritSec *pReceiveLock,
                             HRESULT *phr) :

    CRenderedInputPin(NAME("CDumpInputPin"),
                  pFilter,                   // Filter
                  pLock,                     // Locking
                  phr,                       // Return code
                  L"Input"),                 // Pin name
    m_pReceiveLock(pReceiveLock),
    m_pDump(pDump),
    m_tLast(0)
{
	 
}


//
// CheckMediaType
//
// Check if the pin can support this specific proposed type and format
//
HRESULT CDumpInputPin::CheckMediaType(const CMediaType *)
{
    return S_OK;
}


//
// BreakConnect
//
// Break a connection
//
HRESULT CDumpInputPin::BreakConnect()
{
    if (m_pDump->m_pPosition != NULL) {
        m_pDump->m_pPosition->ForceRefresh();
    }

    return CRenderedInputPin::BreakConnect();
}


//
// ReceiveCanBlock
//
// We don't hold up source threads on Receive
//
STDMETHODIMP CDumpInputPin::ReceiveCanBlock()
{
    return S_FALSE;
}

 

STDMETHODIMP CDumpInputPin::Receive(IMediaSample *pSample)
{
    CheckPointer(pSample,E_POINTER);

	if (m_pDump->m_running == false)
	{		 
		return S_OK;
	}

    CAutoLock lock(m_pReceiveLock);
    PBYTE pbData;
	  
    REFERENCE_TIME tStart, tStop;
    pSample->GetTime(&tStart, &tStop);
	 
   // pSample->GetActualDataLength()));

    m_tLast = tStart;
	 
    HRESULT hr = pSample->GetPointer(&pbData);
    if (FAILED(hr)) {
        return hr;
    }
	 
	int size = pSample->GetActualDataLength();
	 

	
	for (int i = 0; i < size; i++)
	{
		test_pkt.push_back(pbData[i]);
	}


	KlvParser parser({ KlvParser::KEY_ENCODING_16_BYTE });

	// for each byte in the test packet, pass it through the parser until we get a valid Klv object
	KLV* parsed_klv = NULL;
	int i;
	for (i = 0; i < test_pkt.size(); i++) {
		parsed_klv = parser.parseByte(test_pkt[i]);

		if (parsed_klv != NULL)
			break;
	}

	if (parsed_klv == NULL)
		return S_OK;


	std::vector<uint8_t> key = parsed_klv->getKey();
	std::vector<uint8_t> x = parsed_klv->getLenEncoded();
	std::vector<uint8_t> v = parsed_klv->getValue();

	 
	uint8_t* a = &v[0];
	uint8_t* keyArray = &key[0];
 
	if (m_pDump->pKlvCallback != -NULL)
		m_pDump->pKlvCallback(m_pDump->m_id, a, v.size(), keyArray, key.size());
 
	test_pkt.clear();
	   
	return S_OK; 
}
 
CDump::CDump(LPUNKNOWN pUnk, HRESULT *phr) :
    CUnknown(NAME("CDump"), pUnk),
    m_pFilter(NULL),
    m_pPin(NULL),
    m_pPosition(NULL),    
    m_pFileName(0) 
    
{
    ASSERT(phr);
    
    m_pFilter = new CDumpFilter(this, GetOwner(), &m_Lock, phr);
    if (m_pFilter == NULL) {
        if (phr)
            *phr = E_OUTOFMEMORY;
        return;
    }
	pKlvCallback = NULL;
	 
	m_running = 0;
	 
    m_pPin = new CDumpInputPin(this,GetOwner(),
                               m_pFilter,
                               &m_Lock,
                               &m_ReceiveLock,
                               phr);
    if (m_pPin == NULL) {
        if (phr)
            *phr = E_OUTOFMEMORY;
        return;
    }
}
 

CDump::~CDump()
{
	if (m_pPin != NULL)
		delete m_pPin;
		if (m_pFilter != NULL)
    delete m_pFilter;
    delete m_pPosition;
    delete m_pFileName;
}


//
// CreateInstance
//
// Provide the way for COM to create a dump filter
//
CUnknown * WINAPI CDump::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
    ASSERT(phr);
    
    CDump *pNewObject = new CDump(punk, phr);
    if (pNewObject == NULL) {
        if (phr)
            *phr = E_OUTOFMEMORY;
    }

    return pNewObject;

} // CreateInstance


//
// NonDelegatingQueryInterface
//
// Override this to say what interfaces we support where
//
STDMETHODIMP CDump::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    CheckPointer(ppv,E_POINTER);
    CAutoLock lock(&m_Lock);

    // Do we have this interface

	if (riid == IID_IBoutechKLVDecoder) {
		return GetInterface((IBoutechKLVDecoder *) this, ppv);
	}
	else
    if (riid == IID_IBaseFilter || riid == IID_IMediaFilter || riid == IID_IPersist) {
        return m_pFilter->NonDelegatingQueryInterface(riid, ppv);
    } 
    else if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) {
        if (m_pPosition == NULL) 
        {

            HRESULT hr = S_OK;
            m_pPosition = new CPosPassThru(NAME("Dump Pass Through"),
                                           (IUnknown *) GetOwner(),
                                           (HRESULT *) &hr, m_pPin);
            if (m_pPosition == NULL) 
                return E_OUTOFMEMORY;

            if (FAILED(hr)) 
            {
                delete m_pPosition;
                m_pPosition = NULL;
                return hr;
            }
        }

        return m_pPosition->NonDelegatingQueryInterface(riid, ppv);
    } 

    return CUnknown::NonDelegatingQueryInterface(riid, ppv);

} // NonDelegatingQueryInterface

  
//
// DllRegisterSever
//
// Handle the registration of this filter
//
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );

} // DllRegisterServer


//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );

} // DllUnregisterServer


//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
                      DWORD  dwReason, 
                      LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}
  
 
STDMETHODIMP CDump::SetKLVDecoderCallback(int id, KLVCallback p)
{
	pKlvCallback = p;
	m_id = id;
	return S_OK;
}