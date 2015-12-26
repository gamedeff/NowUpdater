//
// NowUpdater
//
// nu_site_parser.h
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#ifndef NU_SITE_PARSER_H
#define NU_SITE_PARSER_H
//-----------------------------------------------------------------------------------
#include <cctype>

#include <regex>

#include "nu_xml.h"
//-----------------------------------------------------------------------------------
struct parser_entity_t
{
	std::string xpath, regexp;

	parser_entity_t(std::string xpath = "", std::string regexp = "") : xpath(xpath), regexp(regexp) {}
};
//-----------------------------------------------------------------------------------

//void str_replace(std::string &s, const std::string &search, const std::string &replace)
//{
//	for( size_t pos = 0; ; pos += replace.length() ) 
//	{
//		pos = s.find( search, pos );
//		if( pos == std::string::npos ) break;
//
//		s.erase( pos, search.length() );
//		s.insert( pos, replace );
//	}
//}

template <class T> T do_nothing_just_pass_value(const pugi::char_t *value)
{
	return value;
}

//template <class T> T select_from(pugi::xml_document &doc, const std::string &xpath, T (*f)(const pugi::char_t*))
//{
//	pugi::xpath_node node = doc.select_node(xpath.c_str());
//
//	return f(node.node() ? node.node().value() : node.attribute().value());
//}
//
//std::string select_from(pugi::xml_document &doc, const std::string &xpath)
//{
//	return select_from(doc, xpath, do_nothing_just_pass_value<const pugi::char_t*>);
//}

template <class T> bool parse_f(const pugi::char_t *node_str, const parser_entity_t &entity, T &result_value, T (*convert_f)(const pugi::char_t *))
{
	if(!node_str)
		return false;

	if(strlen(node_str) > 0 && !entity.regexp.empty())
	{
		const std::regex pattern(entity.regexp/*, std::regex::nosubs*/);

		// object that will contain the sequence of sub-matches
		std::match_results<const pugi::char_t *> result;

		// match the node text with the regular expression
		bool valid = std::regex_search(node_str, result, pattern/*, std::regex_constants::match_continuous*/);

		// if the node text matched the regex, then return the first part
		if(valid && !result.empty())
		{
			result_value = convert_f(result.str(result.size() - 1).c_str());
			return true;
		}
		else
			return false;
	}

	result_value = convert_f(node_str);
	return true;
}

template <class T> bool parse_f(pugi::xml_node &node, const parser_entity_t &entity, T &result_value, T (*convert_f)(const pugi::char_t *))
{
	if(entity.xpath.empty())
	{
		pugi::string_t node_str = pugi::serialization::node_to_raw_string(node);
		//pugi::string_t node_str;

		////pugi::xpath_node_set ns = node.select_nodes(PUGIXML_TEXT("text()"));

		////for(size_t i = 0; i < ns.size(); ++i)
		////	node_str += ns[i].node().value();

		//for(pugi::xml_node child = node.first_child(); child; child = child.next_sibling())
		//	if(child.type() == pugi::node_pcdata)
		//		node_str += child.value();

		return parse_f(node_str.c_str(), entity, result_value, convert_f);
	}
	else
	{
		pugi::xpath_node node_selected = node.select_node(entity.xpath.c_str());
		if(!node_selected)
			return false;
 
		const pugi::char_t *node_str = node_selected.node() ? node_selected.node().value() : node_selected.attribute().value();

		return parse_f(node_str, entity, result_value, convert_f);
	}
}

inline bool parse(pugi::xml_node &node, const parser_entity_t &entity, std::string &result_value)
{
	return parse_f<std::string>(node, entity, result_value, do_nothing_just_pass_value);
}

inline bool parse(pugi::xml_node &node, const parser_entity_t &entity, uint32_t &result_value)
{
	return parse_f(node, entity, result_value, get_uint32_number);
}

inline bool parse(pugi::xml_node &node, const parser_entity_t &entity, float &result_value)
{
	return parse_f(node, entity, result_value, get_float_number);
}
//-----------------------------------------------------------------------------------
#endif
