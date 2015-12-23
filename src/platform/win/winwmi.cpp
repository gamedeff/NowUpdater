//
// NowUpdater
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#include "winwmi.h"
//-----------------------------------------------------------------------------------
#include <Wbemidl.h>
//-----------------------------------------------------------------------------------
#pragma comment(lib, "wbemuuid.lib")
//-----------------------------------------------------------------------------------
bool ExecWMIQuery(const bstr_t &query, const wchar_t *propname, VARIANT *ppropvar)
{
	HRESULT hr = 0;
	IWbemLocator         *WbemLocator  = NULL;
	IWbemServices        *WbemServices = NULL;
	IEnumWbemClassObject *EnumWbem  = NULL;

	// Step 1: --------------------------------------------------
	// Initialize COM. ------------------------------------------
	hr = CoInitializeEx(0, COINIT_MULTITHREADED);

	// Step 2: --------------------------------------------------
	// Set general COM security levels --------------------------
	// Note: If you are using Windows 2000, you need to specify -
	// the default authentication credentials for a user by using
	// a SOLE_AUTHENTICATION_LIST structure in the pAuthList ----
	// parameter of CoInitializeSecurity ------------------------
	hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);

	// Step 3: ---------------------------------------------------
	// Obtain the initial locator to WMI -------------------------
	hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &WbemLocator);

	// Step 4: -----------------------------------------------------
	// Connect to WMI through the IWbemLocator::ConnectServer method
	hr = WbemLocator->ConnectServer(L"ROOT\\CIMV2", NULL, NULL, NULL, 0, NULL, NULL, &WbemServices);   

	// Step 5: --------------------------------------------------
	// Set security levels on the proxy -------------------------
	hr = CoSetProxyBlanket(WbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

	// Step 6: --------------------------------------------------
	// Use the IWbemServices pointer to make requests of WMI ----
	hr = WbemServices->ExecQuery(bstr_t("WQL"), query, WBEM_FLAG_FORWARD_ONLY, NULL, &EnumWbem);

	bool res = false;

	// Step 7: -------------------------------------------------
	// Get the data from the query in step 6 -------------------
	if(EnumWbem != NULL)
	{
		IWbemClassObject *result = NULL;
		ULONG returnedCount = 0;

		while((hr = EnumWbem->Next(WBEM_INFINITE, 1, &result, &returnedCount)) == S_OK)
		{
			// Get the value of the propname property
			if((hr = result->Get(propname, 0, ppropvar, 0, 0)) == S_OK)
			{
				res = true;
			}

			result->Release();
		}

		EnumWbem->Release();
	}

	// Cleanup
	// ========
	WbemServices->Release();
	WbemLocator->Release();

	CoUninitialize();

	return res;
}
//-----------------------------------------------------------------------------------

