#pragma once 

#include <streams.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

 

// {9F2A6627-669B-46F4-8AD8-42808EDCFE46}
static const GUID IID_IBoutechKLVDecoder =
{ 0x9f2a6627, 0x669b, 0x46f4, { 0x8a, 0xd8, 0x42, 0x80, 0x8e, 0xdc, 0xfe, 0x46 } };


typedef void(__stdcall * KLVCallback)(int id, uint8_t *values, int Size, uint8_t *keyBuffer, int keySize);

DECLARE_INTERFACE_(IBoutechKLVDecoder, IUnknown)
{	
	 	
	STDMETHOD(SetKLVDecoderCallback)(int id, KLVCallback klvCallback) PURE;		
	
};