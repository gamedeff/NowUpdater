//
// NowUpdater
//
// nu_xml.h
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#ifndef NU_XML_H
#define NU_XML_H
//-----------------------------------------------------------------------------------
#include "nu_types.h"

#include "Poco/Timestamp.h"

#include <sstream>
#include <locale>

#include "pugixml.hpp"

//#if defined(_DEBUG)
//#define PUGI_LIB_SUFFIX "_d.lib"
//#else
//#define PUGI_LIB_SUFFIX ".lib"
//#endif
//
//#pragma comment(lib, "pugixml" PUGI_LIB_SUFFIX)

#define foreach_xml_node(node, parent, name) \
	for(pugi::xml_node node = parent.child(name); node; node = node.next_sibling(name))
#define foreach_xml_xpath_node(node, parent, xpath_first, xpath_next) \
	for(pugi::xml_node node = parent.select_node(xpath_first).node(); node; node = parent.select_node(xpath_next).node())
//-----------------------------------------------------------------------------------
std::string tidy_html(std::string html_str);

bool check_xml_parse_result(pugi::xml_parse_result &result, const std::string &where_str);

bool load_xml(pugi::xml_document &doc, const std::string &xml_str);

bool load_xhtml(pugi::xml_document &doc, const std::string &xhtml);
//-----------------------------------------------------------------------------------
inline bool iscomma(char ch)
{
	return ch == ',';
}

inline int get_int_number(const pugi::char_t *parsestr)
{
	std::string s(parsestr);

	//std::replace_if(s.begin(), s.end(), iscomma, '');

	for(int i = 0; i < s.size(); ++i)
		if(iscomma(s[i]))
			s.erase(i, 1);

	return atoi(s.c_str());
}

inline int64_t get_int64_number(const pugi::char_t *parsestr)
{
	std::string s(parsestr);

	//std::replace_if(s.begin(), s.end(), iscomma, '');

	for(int i = 0; i < s.size(); ++i)
		if(iscomma(s[i]))
			s.erase(i, 1);

	return _atoi64(s.c_str());
}

inline uint32_t get_uint32_number(const pugi::char_t *parsestr)
{
	return (uint32_t) get_int_number(parsestr);
}

inline bool get_bool_number(const pugi::char_t *parsestr)
{
	return (bool) 0 != get_int_number(parsestr);
}

inline float get_float_number(const pugi::char_t *parsestr)
{
	//return (float) atof(parsestr);

	float float_number = 0.0f;

	std::basic_istringstream<pugi::char_t, std::char_traits<pugi::char_t>, std::allocator<pugi::char_t> > istr(parsestr);

	istr.imbue(std::locale("C"));
	istr >> float_number;

	return float_number;
}

namespace pugi
{
	namespace serialization
	{
		struct raw_text_walker: pugi::xml_tree_walker
		{
			pugi::string_t result;

			virtual bool for_each(pugi::xml_node& node)
			{
				if(node.type() == pugi::node_pcdata)
					result += node.value();
				else if(pugi::string_t(node.name()) == PUGIXML_TEXT("br"))
					result += PUGIXML_TEXT("\n");

				return true; // continue traversal
			}
		};

		struct xml_string_writer: pugi::xml_writer
		{
			pugi::string_t result;

			virtual void write(const void* data, size_t size)
			{
				result += pugi::string_t(static_cast<const pugi::char_t *>(data), size);
			}
		};

		inline pugi::string_t node_to_string(const pugi::xml_node &node)
		{
			xml_string_writer writer;
			node.print(writer);

			return writer.result;
		}

		inline pugi::string_t node_to_raw_string(pugi::xml_node &node)
		{
		//	std::basic_ostringstream<char, std::char_traits<pugi::char_t>, std::allocator<pugi::char_t> > oss;
		//	node.print(oss, PUGIXML_TEXT(""), pugi::format_raw);

		//	return oss.str();

			raw_text_walker walker;
			node.traverse(walker);

			return walker.result;
		}

		template<class T> inline void read(const pugi::xml_attribute &attr, T &x)
		{
			x = attr.as_string();
		}
		template<> inline void read(const pugi::xml_attribute &attr, bool &x)
		{
			x = attr.as_bool();
		}
		template<> inline void read(const pugi::xml_attribute &attr, int &x)
		{
			x = attr.as_int();
		}
		template<> inline void read(const pugi::xml_attribute &attr, int64_t &x)
		{
			x = attr.as_llong();
		}
		template<> inline void read(const pugi::xml_attribute &attr, uint32_t &x)
		{
			x = attr.as_uint();
		}
		template<> inline void read(const pugi::xml_attribute &attr, float &x)
		{
			x = attr.as_float();
		}
		template<> inline void read(const pugi::xml_attribute &attr, Poco::Timestamp &x)
		{
			int64_t epoch_time = 0;
			read(attr, epoch_time);

			x = Poco::Timestamp::fromEpochTime(epoch_time);
		}
		template<class T> inline void read(const pugi::xml_node &node, T &x)
		{
			const pugi::char_t *node_value = (node.type() != pugi::node_pcdata) ? node.child_value() : node.value();

			x = node_value;
		}
		template<> inline void read(const pugi::xml_node &node, bool &x)
		{
			const pugi::char_t *node_value = (node.type() != pugi::node_pcdata) ? node.child_value() : node.value();

			x = get_bool_number(node_value);
		}
		template<> inline void read(const pugi::xml_node &node, int &x)
		{
			const pugi::char_t *node_value = (node.type() != pugi::node_pcdata) ? node.child_value() : node.value();

			x = get_int_number(node_value);
		}
		template<> inline void read(const pugi::xml_node &node, int64_t &x)
		{
			const pugi::char_t *node_value = (node.type() != pugi::node_pcdata) ? node.child_value() : node.value();

			x = get_int64_number(node_value);
		}
		template<> inline void read(const pugi::xml_node &node, uint32_t &x)
		{
			const pugi::char_t *node_value = (node.type() != pugi::node_pcdata) ? node.child_value() : node.value();

			x = get_uint32_number(node_value);
		}
		template<> inline void read(const pugi::xml_node &node, float &x)
		{
			const pugi::char_t *node_value = (node.type() != pugi::node_pcdata) ? node.child_value() : node.value();

			x = get_float_number(node_value);
		}
		template<> inline void read(const pugi::xml_node &node, Poco::Timestamp &x)
		{
			int64_t epoch_time = 0;
			read(node, epoch_time);

			x = Poco::Timestamp::fromEpochTime(epoch_time);
		}

		template<class W, class T> inline bool read(pugi::xml_node &node, T &x, const pugi::char_t *x_name, W (xml_node::*first_what)() const, W (W::*next_what)() const)
		{
			bool res = false;

			W wut = (node.*first_what)();
			while(wut)
			{
				const pugi::char_t *wut_name = wut.name();

				do
				{
					//if(!_tcscmp(attr_name, x_name))
					if(!strcmp(wut_name, x_name))
					{
						read(wut, x);
						res = true;
						break;
					}
				}
				while(0);

				wut = (wut.*next_what)();
			}

			return res;
		}
		//template<class W, class T, class U> inline bool read(pugi::xml_node &node, std::map<T, U> &x, const pugi::char_t *x_name, W (xml_node::*first_what)() const, W (W::*next_what)() const)
		//{
		//	W wut = (node.*first_what)();
		//	while(wut)
		//	{
		//		T x_name_val;

		//		if(!read(wut, x_name_val, x_name, first_what, next_what))
		//			return false;

		//		U x_value_val;

		//		if(!read(wut, x_value_val, x_name, first_what, next_what))
		//			return false;

		//		x[x_name_val] = x_value_val;

		//		wut = (wut.*next_what)();
		//	}

		//	return true;
		//}
		template<class W, class U> inline bool read(pugi::xml_node &node, std::map<std::string, U> &x, const pugi::char_t *x_name, W (xml_node::*first_what)() const, W (W::*next_what)() const)
		{
			bool res = false;

			W wut1 = (node.*first_what)();
			while(wut1)
			{
				const pugi::char_t *wut_name = wut1.name();

				do
				{
					//if(!_tcscmp(attr_name, x_name))
					if(!strcmp(wut_name, x_name))
					{
						W wut = (wut1.*first_what)();
						while(wut)
						{
							const pugi::char_t *wut_name = wut.name();

							U x_val;

							read(wut, x_val);

							//x.push_back(x_val);
							x[wut_name] = x_val;

							wut = (wut.*next_what)();
						}

						res = true;
						break;
					}
				}
				while(0);

				wut1 = (wut1.*next_what)();
			}

			return res;
		}
		template<class W, class T> inline bool read(pugi::xml_node &node, std::vector<T> &x, const pugi::char_t *x_name, W (xml_node::*first_what)() const, W (W::*next_what)() const)
		{
			W wut = (node.*first_what)();
			while(wut)
			{
				T x_val;

				if(!read(wut, x_val, x_name, first_what, next_what))
					return false;

				x.push_back(x_val);

				wut = (wut.*next_what)();
			}

			return true;
		}
		template<class W, class T> inline bool read(pugi::xml_node &node, std::vector<T> &x, const pugi::char_t *x_name, W (xml_node::*first_what)() const, W (W::*next_what)() const, bool (T::*read_method)(W &w))
		{
			bool res = false;

			W wut = (node.*first_what)();
			while(wut)
			{
				const pugi::char_t *wut_name = wut.name();

				do
				{
					//if(!_tcscmp(attr_name, x_name))
					if(!strcmp(wut_name, x_name))
					{
						T x_val;

						if(!(x_val.*read_method)(wut))
							return false;

						x.push_back(x_val);

						res = true;
						break;
					}
				}
				while(0);

				wut = (wut.*next_what)();
			}

			return res;
		}
		template<class T> inline bool read_child(pugi::xml_node &node, T &x, const pugi::char_t *x_name)
		{
			return read<xml_node, T>(node, x, x_name, &xml_node::first_child, &xml_node::next_sibling);
		}
		template<class T> inline bool read_attribute(pugi::xml_node &node, T &x, const pugi::char_t *x_name)
		{
			return read<xml_attribute, T>(node, x, x_name, &xml_node::first_attribute, &xml_attribute::next_attribute);
		}
		template<class T, class U> inline bool read_child(pugi::xml_node &node, std::map<T, U> &x, const pugi::char_t *x_name)
		{
			return read<xml_node, T, U>(node, x, x_name, &xml_node::first_child, &xml_node::next_sibling);
		}
		template<class T, class U> inline bool read_attribute(pugi::xml_node &node, std::map<T, U> &x, const pugi::char_t *x_name)
		{
			return read<xml_attribute, T, U>(node, x, x_name, &xml_node::first_attribute, &xml_attribute::next_attribute);
		}
		template<class U> inline bool read_child(pugi::xml_node &node, std::map<std::string, U> &x, const pugi::char_t *x_name)
		{
			return read<xml_node, U>(node, x, x_name, &xml_node::first_child, &xml_node::next_sibling);
		}
		template<class U> inline bool read_attribute(pugi::xml_node &node, std::map<std::string, U> &x, const pugi::char_t *x_name)
		{
			return read<xml_attribute, U>(node, x, x_name, &xml_node::first_attribute, &xml_attribute::next_attribute);
		}
		template<class T> inline bool read_child(pugi::xml_node &node, std::vector<T> &x, const pugi::char_t *x_name)
		{
			return read<xml_node, T>(node, x, x_name, &xml_node::first_child, &xml_node::next_sibling);
		}
		template<class T> inline bool read_attribute(pugi::xml_node &node, std::vector<T> &x, const pugi::char_t *x_name)
		{
			return read<xml_attribute, T>(node, x, x_name, &xml_node::first_attribute, &xml_attribute::next_attribute);
		}
		template<class T> inline bool read_child_array(pugi::xml_node &node, std::vector<T> &x, const pugi::char_t *x_name, bool (T::*read_method)(pugi::xml_node &node) = &T::read)
		{
			return read<xml_node, T>(node, x, x_name, &xml_node::first_child, &xml_node::next_sibling, read_method);
		}

		template<class T> inline bool write(pugi::xml_node &node, const T &x)
		{
			std::string str = std::to_string(x);

			return write(node, str);
		}
		template<> inline bool write(pugi::xml_node &node, const std::string &x)
		{
			if(node.type() != pugi::node_pcdata)
			{
				node = node.append_child(pugi::node_pcdata);

				if(!node)
					return false;
			}

			return node.set_value(x.c_str());
		}
		template<> inline bool write(pugi::xml_node &node, const Poco::Timestamp &x)
		{
			int64_t epoch_time = x.epochTime();

			return write(node, epoch_time);
		}
		template<class T> inline bool write(pugi::xml_attribute &attr, const T &x)
		{
			return attr.set_value(x);
		}
		template<> inline bool write(pugi::xml_attribute &attr, const std::string &x)
		{
			return attr.set_value(x.c_str());
		}
		template<> inline bool write(pugi::xml_attribute &attr, const Poco::Timestamp &x)
		{
			int64_t epoch_time = x.epochTime();

			return write(attr, epoch_time);
		}

		template<class W, class T> inline bool write(pugi::xml_node &node, const T &x, const pugi::char_t *x_name, W (xml_node::*append_what)(const char_t* name))
		{
			W wut = (node.*append_what)(x_name);

			if(!wut)
				return false;

			if(!write(wut, x))
				return false;

			return true;
		}
		template<class W, class T, class U> inline bool write(pugi::xml_node &node, const std::map<T, U> &x, const pugi::char_t *x_name, W (xml_node::*append_what)(const char_t* name))
		{
			W wut = (node.*append_what)(x_name);

			if(!wut)
				return false;

			for(std::map<T, U>::const_iterator it = x.begin(); it != x.end(); ++it)
			{
				if(!write(wut, it->first, "first", append_what))
					return false;

				if(!write(wut, it->second, "second", append_what))
					return false;
			}

			return true;
		}
		template<class W, class U> inline bool write(pugi::xml_node &node, const std::map<std::string, U> &x, const pugi::char_t *x_name, W (xml_node::*append_what)(const char_t* name))
		{
			W wut = (node.*append_what)(x_name);

			if(!wut)
				return false;

			for(std::map<std::string, U>::const_iterator it = x.begin(); it != x.end(); ++it)
			{
				if(!write(wut, it->second, it->first.c_str(), append_what))
					return false;
			}

			return true;
		}
		template<class W, class T> inline bool write(pugi::xml_node &node, const std::vector<T> &x, const pugi::char_t *x_name, W (xml_node::*append_what)(const char_t* name))
		{
			for(std::vector<T>::const_iterator it = x.begin(); it != x.end(); ++it)
			{
				W wut = (node.*append_what)(x_name);

				if(!wut)
					return false;

				if(!write(wut, *it, x_name, append_what))
					return false;
			}

			return true;
		}
		template<class W, class T> inline bool write(pugi::xml_node &node, /*const*/ std::vector<T> &x, const pugi::char_t *x_name, W (xml_node::*append_what)(const char_t* name), bool (T::*write_method)(W &w))
		{
			for(std::vector<T>::/*const_*/iterator it = x.begin(); it != x.end(); ++it)
			{
				//std::string x_val_name = x_name + std::string("[") + std::to_string(it - x.begin()) + std::string("]");

				W wut = (node.*append_what)(x_name);

				if(!wut)
					return false;

				if(!((*it).*write_method)(wut))
					return false;
			}

			return true;
		}
		template<class T> inline bool write_child(pugi::xml_node &node, const T &x, const pugi::char_t *x_name)
		{
			return write(node, x, x_name, &xml_node::append_child);
		}
		template<class T> inline bool write_attribute(pugi::xml_node &node, const T &x, const pugi::char_t *x_name)
		{
			return write(node, x, x_name, &xml_node::append_attribute);
		}
		template<class T> inline bool write_child_array(pugi::xml_node &node, /*const*/ std::vector<T> &x, const pugi::char_t *x_name, bool (T::*write_method)(pugi::xml_node &node) = &T::write)
		{
			return write(node, x, x_name, &xml_node::append_child, write_method);
		}
		//template<class T> inline bool write_attribute(pugi::xml_node &node, const std::vector<T> &x, const pugi::char_t *x_name, bool (T::*write_method)(W &w) = 0)
		//{
		//	return write(node, x, x_name, &xml_node::append_attribute, write_method);
		//}

		//template<class T> inline bool serialize(pugi::xml_node &node, bool is_write)
	}
}

//#define PUGI_READ_T(node, x)  { if(!pugi::serialization::read(node, x, GW_CODE2TXT(x))) return false; }
//#define PUGI_WRITE_T(node, x) { if(!pugi::serialization::write(node, x, GW_CODE2TXT(x))) return false; }

#define PUGI_READ(node, x)  { if(!pugi::serialization::read_child(node, x, GW_TOSTRING(x))) return false; }
#define PUGI_READ_ATTRIB(node, x)  { if(!pugi::serialization::read_attribute(node, x, GW_TOSTRING(x))) return false; }
#define PUGI_WRITE(node, x) { if(!pugi::serialization::write_child(node, x, GW_TOSTRING(x))) return false; }
#define PUGI_WRITE_ATTRIB(node, x) { if(!pugi::serialization::write_attribute(node, x, GW_TOSTRING(x))) return false; }
#define PUGI_WRITE_ARRAY(node, x, x_name) { if(!pugi::serialization::write_child_array(node, x, x_name)) return false; }
#define PUGI_READ_ARRAY(node, x, x_name)  { if(!pugi::serialization::read_child_array(node, x, x_name)) return false; }
//#define PUGI_WRITE_MAP(node, x, x_name) { if(!pugi::serialization::write_child_array(node, x, x_name)) return false; }
//#define PUGI_READ_MAP(node, x, x_name)  { if(!pugi::serialization::read_child_array(node, x, x_name)) return false; }

#define PUGI_SERIALIZE(x) { if(is_write) PUGI_WRITE(node, x) else PUGI_READ(node, x) }
#define PUGI_SERIALIZE_NR(x) (is_write ? pugi::serialization::write_child(node, x, GW_TOSTRING(x)) : pugi::serialization::read_child(node, x, GW_TOSTRING(x)))
#define PUGI_SERIALIZE_ATTRIB(x) { if(is_write) PUGI_WRITE_ATTRIB(node, x) else PUGI_READ_ATTRIB(node, x) }
#define PUGI_SERIALIZE_ARRAY(x, x_name) { if(is_write) PUGI_WRITE_ARRAY(node, x, x_name) else PUGI_READ_ARRAY(node, x, x_name) }
#define PUGI_SERIALIZE_ARRAY_NR(x, x_name) (is_write ? pugi::serialization::write_child_array(node, x, x_name) : pugi::serialization::read_child_array(node, x, x_name))
//#define PUGI_SERIALIZE_MAP(x, x_name) { if(is_write) PUGI_WRITE_MAP(node, x, x_name) else PUGI_READ_MAP(node, x, x_name) }

#define PUGI_SERIALIZATION_START bool serialize(pugi::xml_node &node, bool is_write)
#define PUGI_SERIALIZATION_END \
	bool read(pugi::xml_node &node) \
	{ \
		return serialize(node, false); \
	} \
	bool write(pugi::xml_node &node) \
	{ \
		return serialize(node, true); \
	}

#define GW_DECLARE_FUNC_COMPARE_BY_(x) \
	template<class T> struct compare_by_##x : public std::binary_function<T, T, bool> \
	{ \
		bool operator()(const T &a, const T &b) const \
		{ \
			return (a.##x == b.##x); \
		} \
	}

GW_DECLARE_FUNC_COMPARE_BY_(index);
GW_DECLARE_FUNC_COMPARE_BY_(name);
//-----------------------------------------------------------------------------------
#endif
