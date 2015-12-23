//
// NowUpdater
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#ifndef WINWMI_H
#define WINWMI_H
//-----------------------------------------------------------------------------------
#define _WIN32_DCOM
//-----------------------------------------------------------------------------------
#include <comdef.h>
//-----------------------------------------------------------------------------------
bool ExecWMIQuery(const bstr_t &query, const wchar_t *propname, VARIANT *ppropvar);
//-----------------------------------------------------------------------------------
template <class ST, class CT> ST GetWMIVariable(VARIANT &propvar)
{
	ST res;

	bstr_t resbstr = bstr_t(propvar.bstrVal);
	CT *resc = (CT *) resbstr;
	if(resc)
		res.assign(resc);

	return res;
}
//-----------------------------------------------------------------------------------
template <class ST, class CT> ST ExecWMIQuery(const bstr_t &query, const wchar_t *propname)
{
	ST res;

	//VARIANT ProcessId;
	VARIANT propvar;
	if(ExecWMIQuery(query, propname, &propvar))
	{
		// access the properties
		//hr = result->Get(L"ProcessId", 0, &ProcessId, 0, 0);
		//hr = result->Get(L"CommandLine", 0, &CommandLine, 0, 0);            
		//if (!(CommandLine.vt==VT_NULL))
		//	wprintf(L"%u  %s \r\n", ProcessId.uintVal, CommandLine.bstrVal);VARIANT vtProp;

		// Get the value of the propname property
		if(!(propvar.vt == VT_NULL))
		{
			res = GetWMIVariable<ST, CT>(propvar);
		}
		VariantClear(&propvar);
	}

	return res;
}
//-----------------------------------------------------------------------------------
template <class ST, class CT> ST ExecWMIQuery(const CT *query, const wchar_t *propname)
{
	return ExecWMIQuery(query, propname);
}
//-----------------------------------------------------------------------------------
#endif
