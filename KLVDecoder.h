#pragma once 
#include "myInterface.h" 
#include <iostream>
#include <vector>
class CDumpInputPin;
class CDump;
class CDumpFilter;



// Main filter object

class CDumpFilter : public CBaseFilter
{
    CDump * const m_pDump;

public:

    // Constructor
    CDumpFilter(CDump *pDump,
                LPUNKNOWN pUnk,
                CCritSec *pLock,
                HRESULT *phr);

    // Pin enumeration
    CBasePin * GetPin(int n);
    int GetPinCount();

    // Open and close the file as necessary
    STDMETHODIMP Run(REFERENCE_TIME tStart);
    STDMETHODIMP Pause();
    STDMETHODIMP Stop();
	  
};


//  Pin object

class CDumpInputPin : public CRenderedInputPin
{
    CDump    * const m_pDump;           // Main renderer object
    CCritSec * const m_pReceiveLock;    // Sample critical section
    REFERENCE_TIME m_tLast;             // Last sample receive time

public:

    CDumpInputPin(CDump *pDump,
                  LPUNKNOWN pUnk,
                  CBaseFilter *pFilter,
                  CCritSec *pLock,
                  CCritSec *pReceiveLock,
                  HRESULT *phr);

    // Do something with this media sample
    STDMETHODIMP Receive(IMediaSample *pSample);
    
    STDMETHODIMP ReceiveCanBlock();

	 
    // Check if the pin can support this specific proposed type and format
    HRESULT CheckMediaType(const CMediaType *);

    // Break connection
    HRESULT BreakConnect();
	std::vector<uint8_t> test_pkt;
	 

   
};


//  CDump object which has filter and pin members

class CDump : public CUnknown, public IBoutechKLVDecoder
{
    friend class CDumpFilter;
    friend class CDumpInputPin;

    CDumpFilter   *m_pFilter;       // Methods for filter interfaces
    CDumpInputPin *m_pPin;          // A simple rendered input pin

    CCritSec m_Lock;                // Main renderer critical section
    CCritSec m_ReceiveLock;         // Sublock for received samples

    CPosPassThru *m_pPosition;      // Renderer position controls
    LPOLESTR m_pFileName;           // The filename where we dump


public:

    DECLARE_IUNKNOWN

    CDump(LPUNKNOWN pUnk, HRESULT *phr);
    ~CDump();

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

	int m_id;
    
    // Write raw data stream to a file
    HRESULT Write(PBYTE pbData, LONG lDataLength);

	 
	STDMETHODIMP SetKLVDecoderCallback(int id , KLVCallback klvCallback);


	KLVCallback pKlvCallback;
 
	int m_chunkCount;
	int m_running;
	 
private:

    // Overriden to say what interfaces we support where
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

 
};

