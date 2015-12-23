//
// NowUpdater
//
// nu_xml.cpp
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#include "nu_xml.h"
//-----------------------------------------------------------------------------------
#include "tidy.h"
#include "buffio.h"

#if defined(_DEBUG)
#define TIDY_LIB_SUFFIX "_d.lib"
#else
#define TIDY_LIB_SUFFIX ".lib"
#endif

#pragma comment(lib, "libtidy" TIDY_LIB_SUFFIX)
//-----------------------------------------------------------------------------------
std::string tidy_html(std::string html_str)
{
	if(html_str.empty())
		return html_str;

	std::string res;

	//std::transform(html_str.begin(), html_str.end(), html_str.begin(), ::tolower);

	//// strip all modern scripting shit from HEAD:
	//std::string real_html_start_tag = "<body";
	//std::string real_html_end_tag   = "</body>";
	//size_t s = html_str.find(real_html_start_tag);
	//size_t e = html_str.find(real_html_end_tag, s);
	//html_str = "<html>" + html_str.substr(s, e - s + real_html_end_tag.size()) + "</html>";

	TidyBuffer output = {0};
	TidyBuffer errbuf = {0};
	int rc = -1;
	Bool ok;

	TidyDoc tdoc = tidyCreate();                     // Initialize "document"
	//std::cout << "Tidying:\t" << htmlstr << std::endl;

	ok = tidyOptSetBool( tdoc, TidyXhtmlOut, yes );  // Convert to XHTML
	ok = (Bool) (ok && tidyOptSetBool( tdoc, TidyEscapeCdata, yes ));
	if ( ok )
		rc = tidySetErrorBuffer( tdoc, &errbuf );      // Capture diagnostics
	if ( rc >= 0 )
		rc = tidyParseString( tdoc, html_str.c_str() );           // Parse the input
	if ( rc >= 0 )
		rc = tidyCleanAndRepair( tdoc );               // Tidy it up!
	if ( rc >= 0 )
		rc = tidyRunDiagnostics( tdoc );               // Kvetch
	if ( rc > 1 )                                    // If error, force output.
		rc = ( tidyOptSetBool(tdoc, TidyForceOutput, yes) ? rc : -1 );
	if ( rc >= 0 )
		rc = tidySaveBuffer( tdoc, &output );          // Pretty Print

	if ( rc >= 0 )
	{
		//if ( rc > 0 )
		//	std::cout << "\nDiagnostics:\n" << errbuf.bp << std::endl;
		////std::cout << "\nAnd here is the result:\n" << output.bp << std::endl;
		res = (char *)output.bp;
	}
	else
		std::cout << "A severe error (" << rc << ") occurred."  << std::endl;

	tidyBufFree( &output );
	tidyBufFree( &errbuf );
	tidyRelease( tdoc );

	//str_replace(res, "\\/", "/");

	return res;
}

bool check_xml_parse_result(pugi::xml_parse_result &result, const std::string &where_str)
{
	if(result)
	{
#ifdef _DEBUG
		std::cout << "Load result: " << result.description() << std::endl;
#endif

		return true;
	}
	else
	{
#ifdef _DEBUG
		std::cout << "Error description: " << result.description() << std::endl;
		std::cout << "Error offset: " << result.offset << where_str << std::endl;
#endif

		return false;
	}
}

bool load_xml(pugi::xml_document &doc, const std::string &xml_str)
{
	pugi::xml_parse_result result = doc.load(xml_str.c_str());

	return check_xml_parse_result(result, " (error at [..." + xml_str.substr(result.offset, std::min<int>(1024, xml_str.size() - result.offset)) + "]");
}

bool load_xhtml(pugi::xml_document &doc, const std::string &xhtml)
{
	if(!load_xml(doc, tidy_html(xhtml)))
		return false;

	return true;
}

