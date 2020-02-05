// dllmain.cpp : DLL アプリケーションのエントリ ポイントを定義します。
#include "pch.h"

#include <neko.h>

#define HEAP_SIZE 0x10000

static CRITICAL_SECTION csMem;
static HANDLE hHeap;


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		InitializeCriticalSection(&csMem);

		hHeap = HeapCreate(HEAP_CREATE_ALIGN_16 | HEAP_NO_SERIALIZE, HEAP_SIZE, HEAP_SIZE);

		break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
		break;
    case DLL_PROCESS_DETACH:
		EnterCriticalSection(&csMem);

		HeapDestroy(hHeap);

		hHeap = NULL;

		LeaveCriticalSection(&csMem);
		DeleteCriticalSection(&csMem);

		break;
    }
    return TRUE;
}


extern "C" {
	static void get_size_f(value v, field f, void *p)
	{
		int *q = (int*)p;

		++*q;
	}

	EXPORT value get_size(value v)
	{
		int r = 0;

		if (val_is_null(v)) r = 0;

		if (val_is_bool(v)) r = 1;

		if (val_is_array(v)) r = val_array_size(v);

		if (val_is_string(v)) r = val_strlen(v);

		if (val_is_object(v)) val_iter_fields(v, get_size_f, &r);

		if (val_is_function(v)) r = val_fun_nargs(v);

		if (val_is_number(v)) r = 1;

		return alloc_int(r);
	}


	EXPORT value point_to_string(value x, value y)
	{
		buffer buf = alloc_buffer("");

		buffer_append_sub(buf, "{x: X, y: Y}", 3);
		val_buffer(buf, x);
		buffer_append(buf, ", y: ");
		val_buffer(buf, y);
		buffer_append_char(buf, '}');

		return buffer_to_string(buf);
	}


	/* abstruct kind types */

	DEFINE_KIND(k_ByteArray);


	EXPORT void ByteArray_delete(value v);

	EXPORT value ByteArray_create(value n)
	{
		val_check(n,int);

		int i = val_int(n);

		if (i < 0) neko_error();

		unsigned char *bytes = NULL;
		
		if (hHeap != NULL)
		{
			EnterCriticalSection(&csMem);

			bytes = (unsigned char*)HeapAlloc(hHeap, HEAP_NO_SERIALIZE, i);

			LeaveCriticalSection(&csMem);
		}

		if (bytes == NULL) return val_null;

		value r = alloc_abstract(k_ByteArray, bytes);

		/* register to gc (optional) */
		val_gc(r,ByteArray_delete);

		return r;
	}

	EXPORT void ByteArray_delete(value v)
	{
		if (!val_is_abstract(v)
			|| !val_is_kind(v, k_ByteArray)) return;

		unsigned char *bytes = (unsigned char*)val_data(v);

		if (hHeap != NULL)
		{
			EnterCriticalSection(&csMem);

			HeapFree(hHeap, HEAP_NO_SERIALIZE, bytes);

			LeaveCriticalSection(&csMem);
		}

		val_kind(v) = NULL;
	}

	EXPORT value ByteArray_get(value b, value n)
	{
		val_check_kind(b, k_ByteArray);
		val_check(n, int);

		unsigned char *bytes = (unsigned char*)val_data(b);
		int i = val_int(n);

		if (i < 0 || i > HEAP_SIZE - 1) neko_error();

		return alloc_int(bytes[i]);
	}

	EXPORT value ByteArray_set(value b, value n, value v)
	{
		val_check_kind(b, k_ByteArray);
		val_check(n, int);
		val_check(v, int);

		unsigned char *bytes = (unsigned char*)val_data(b);
		int i = val_int(n);

		if (i < 0 || i > HEAP_SIZE - 1) neko_error();

		bytes[i] = val_int(v);

		return b;
	}

	DEFINE_PRIM(get_size, 1);
	DEFINE_PRIM(point_to_string, 2);
	DEFINE_PRIM(ByteArray_create, 1);
	DEFINE_PRIM(ByteArray_delete, 1);
	DEFINE_PRIM(ByteArray_get, 2);
	DEFINE_PRIM(ByteArray_set, 3);
}

