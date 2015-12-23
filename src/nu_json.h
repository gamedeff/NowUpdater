//
// NowUpdater
//
// nu_json.h
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#ifndef NU_JSON_H
#define NU_JSON_H
//-----------------------------------------------------------------------------------
#include "nu_types.h"

#include "Poco/Dynamic/Var.h"
#include "Poco/JSON/ParseHandler.h"
#include "Poco/NumberFormatter.h"
//-----------------------------------------------------------------------------------
void make_json_request_str_start(Poco::JSON::ParseHandler &parse_handler)
{
	parse_handler.startObject();
		parse_handler.key("clientVersion");
		parse_handler.value(Poco::NumberFormatter::format(0.9));
		parse_handler.key("params");
		parse_handler.startArray();
}

std::string make_json_request_str_end(Poco::JSON::ParseHandler &parse_handler, const std::string &func)
{
		parse_handler.endArray();
		parse_handler.key("func");
		parse_handler.value(func);
	parse_handler.endObject();
	Poco::Dynamic::Var json_request = parse_handler.asVar();
	std::string json_request_str = json_request.toString();
	replace_whitespace(json_request_str);
	return json_request_str;
}

template<class A> std::string make_json_request_str(const std::string &func, A a)
{
	Poco::JSON::ParseHandler parse_handler(true);
	make_json_request_str_start(parse_handler);
		parse_handler.value(a);
	return make_json_request_str_end(parse_handler, func);
}

template<class A, class B> std::string make_json_request_str(const std::string &func, A a, B b)
{
	Poco::JSON::ParseHandler parse_handler(true);
	make_json_request_str_start(parse_handler);
		parse_handler.value(a);
		parse_handler.value(b);
	return make_json_request_str_end(parse_handler, func);
}

template<class A, class B, class C> std::string make_json_request_str(const std::string &func, A a, B b, C c)
{
	Poco::JSON::ParseHandler parse_handler(true);
	make_json_request_str_start(parse_handler);
		parse_handler.value(a);
		parse_handler.value(b);
		parse_handler.value(c);
	return make_json_request_str_end(parse_handler, func);
}
//-----------------------------------------------------------------------------------
#endif

