
/*
(c) Yanikov '_Winnie' Ivan 2005 , woowoo@list.ru 
Use, modification and distribution is subject to the Boost Software
License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt) 
*/

#ifndef FOR_EACH_MACRO_HEADER_
#define FOR_EACH_MACRO_HEADER_

/** \cond */

namespace FOR_EACH_namespace
{

namespace Detail
{

struct WrapBase
{
	virtual ~WrapBase() {} //disable IC++ warning(444): destructor for base class "WrapBase" is not virtual
};

template <class Iterator>
struct WrapIter: public WrapBase 
{
  WrapIter(const Iterator &it): it(it) {}
  mutable Iterator it;
};

template <class Iterator>
WrapIter<Iterator> MakeWrapIter(const Iterator &it)
{
  return WrapIter<Iterator>(it);
}

//Unwrap for STL containers
template <class Container>
typename Container::iterator &Unwrap(const WrapBase &iterator_wrap, Container &)
{
  return static_cast<const WrapIter<typename Container::iterator> &>(iterator_wrap).it;
}

template <class Container>
typename Container::const_iterator &Unwrap(const WrapBase &iterator_wrap, const Container &)
{
  return static_cast<const WrapIter<typename Container::const_iterator> &>(iterator_wrap).it;
}

//Unwrap for arrays (we do not need in const T specialization, cv-qualifers already are in T)
template <class T, int size>
T* &Unwrap(const WrapBase &iterator_wrap, T (&)[size])
{
  return static_cast<const WrapIter<T*> &>(iterator_wrap).it;
}

//Begin/End for STL containers
template <class Container>
typename Container::iterator Begin(Container &c) { return c.begin(); }

template <class Container>
typename Container::const_iterator Begin(const Container &c) { return c.begin(); }


template <class Container>
typename Container::iterator End(Container &c) { return c.end(); }

template <class Container>
typename Container::const_iterator End(const Container &c) { return c.end(); }

//Begin/End for arrays
template <class T, int size>
T *Begin(T (&c)[size]) { return c; }

template <class T, int size>
T *End(T (&c)[size]) { return c+size; }


//дл€ предотвращени€ "warning C4706: assignment within conditional expression"
inline bool Invert(bool &b) 
{
	return b = !b;
}

} //namespace FOR_EACH_namespace
} //namespace Detail

/** \endcond */

/*

Be carefull, do not use expressions as second argument `container` (it often repeated in
this macro).

const FOR_EACH_namespace::Detail::WrapBase\
        &iterator_ = ...

.) non-const reference can't be initialized by temporary, so I use `const`
.) temporary, binded to reference live while reference is alive
.) `WrapBase iterator_` is const, but need in incrementing, => `mutable`
.) I can't write type of `container` ( like `typeof(container)`), so i need `static_cast` 
   trick and second parameter of `Unwrap(iterator_, container)`

if (int _fe_once_= 0) ;else \
  for ( \

.) do not allow something like FOR_EACH(...) { ... } else {} 
.) initialize counter as 0 for second `for` 
   ( C++ don't allow `for (int i=0, double d=0; ...`
.) initialize `declaration` by (*iterator)
.) internal `for` used only for `declaration` initialization. It execute body only once.
.) IC++ 8.0, MSVC 7.1,  MSVC 6.0 can remove redudant jumps
*/


/** 
»тераци€ по элементам контейнера. “ребует от контейнера наличи€ вложенных типов iterator/const_iterator 
удовлетвор€ющих требовани€м STL-forward итераторов, 
методов begin/end возвращающих эти итераторы.\n
“о, что передаетс€ в параметре declaration иницализируетс€ выражением \a (*CurrentIterator), таким образом если declaration
это ссылка, то она инициализируетс€ ссылкой на элемент внутри контейнера. Ѕудте осторожны, не забывайте ее, иначе вы будете
иметь дело не с объектом, а с его копией, что может повлечь накладные расходы или неожиданное поведение.\n
ѕримеры использовани€:
\code
    string strings[] = { "one", "two", "three" };
    FOR_EACH(std::string &str, strings)
      std::cout <<str <<std::endl;
\endcode
\code

    const std::vector<int> &v;
    ...
    int s=1;
    FOR_EACH(const int& i, v)
      s*= i;
\endcode
\code
    std::vector<int> &v;
    ...
    int s=1;
    FOR_EACH(int& i, v)
    {
        s*= i;
        i= 10;
    }
\endcode
	¬ отличие от std::for_each, FOR_EACH позвол€ет удал€ть элементы из контейнера, если это не инвалидирует итератор на следующий элемент.
\code
	CIntrusiveList<T> l;
	FOR_EACH(T &x, l)
		if (x.m_member > 0)
			x.Unlink();
\endcode

ћожно делать break:
\code
	
	std::list<Object *> Container;
	Object *pFoundObject = NULL;
	FOR_EACH(CObject *pObject, Container)
	{
		if (pObject->GetName() == "Winnie")
		{
			pFoundObject = pObject;
			break;
		}
	}
	if (pFoundObject)
	{
	...
	}
\endcode
*/

#if 1

#define FOR_EACH(declaration, container)   \
    if (bool fe_once_ = 0) ; else\
    for (\
      const FOR_EACH_namespace::Detail::WrapBase\
        &iterator_ = FOR_EACH_namespace::Detail::MakeWrapIter(FOR_EACH_namespace::Detail::Begin(container)),\
        &iterator_end_ = FOR_EACH_namespace::Detail::MakeWrapIter(FOR_EACH_namespace::Detail::End(container));\
\
      FOR_EACH_namespace::Detail::Unwrap(iterator_, (container)) != \
      FOR_EACH_namespace::Detail::Unwrap(iterator_end_, (container));\
    ) \
    if (fe_once_) break; else \
      for ( \
        declaration (*FOR_EACH_namespace::Detail::Unwrap(iterator_, (container))); \
        (!fe_once_ && (++FOR_EACH_namespace::Detail::Unwrap(iterator_, (container)), true)), (FOR_EACH_namespace::Detail::Invert(fe_once_)); \
        )
#endif

#endif

