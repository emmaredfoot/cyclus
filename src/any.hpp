/*=============================================================================
    Copyright (c) 2007-2011 Hartmut Kaiser
    Copyright (c) Christopher Diggins 2005
    Copyright (c) Pablo Aguilar 2005
    Copyright (c) Kevlin Henney 2001

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    The class boost::spirit::hold_any is built based on the any class
    published here: http://www.codeproject.com/cpp/dynamic_typing.asp. It adds
    support for std streaming operator<<() and operator>>().
==============================================================================
Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
==============================================================================
WARNING: this file was modified by Robert Carlsen for Cyclus from the
original Boost 1.54.0
(http://www.boost.org/doc/libs/1_54_0/boost/spirit/home/support/detail/hold_any.hpp).
==============================================================================*/
#if !defined(BOOST_SPIRIT_HOLD_ANY_MAY_02_2007_0857AM)
#define BOOST_SPIRIT_HOLD_ANY_MAY_02_2007_0857AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/throw_exception.hpp>
#include <boost/static_assert.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/assert.hpp>
#include <boost/detail/sp_typeinfo.hpp>

#include <stdexcept>
#include <typeinfo>
#include <algorithm>
#include <iosfwd>
#include <string>

///////////////////////////////////////////////////////////////////////////////
#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
# pragma warning(push)
# pragma warning(disable: 4100)   // 'x': unreferenced formal parameter
# pragma warning(disable: 4127)   // conditional expression is constant
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit
{
    struct bad_any_cast
      : std::bad_cast
    {
        bad_any_cast(boost::detail::sp_typeinfo const& src, boost::detail::sp_typeinfo const& dest)
          : from(src.name()), to(dest.name())
        {}

        virtual const char* what() const throw() { return "bad any cast"; }

        const char* from;
        const char* to;
    };

    namespace detail
    {
        // function pointer table
        template <typename Char>
        struct fxn_ptr_table
        {
            boost::detail::sp_typeinfo const& (*get_type)();
            void (*static_delete)(void**);
            void (*destruct)(void**);
            void (*clone)(void* const*, void**);
            void (*move)(void* const*, void**);
        };

        // static functions for small value-types
        template <typename Small>
        struct fxns;

        template <>
        struct fxns<mpl::true_>
        {
            template<typename T, typename Char>
            struct type
            {
                static boost::detail::sp_typeinfo const& get_type()
                {
                    return BOOST_SP_TYPEID(T);
                }
                static void static_delete(void** x)
                {
                    reinterpret_cast<T*>(x)->~T();
                }
                static void destruct(void** x)
                {
                    reinterpret_cast<T*>(x)->~T();
                }
                static void clone(void* const* src, void** dest)
                {
                    new (dest) T(*reinterpret_cast<T const*>(src));
                }
                static void move(void* const* src, void** dest)
                {
                    *reinterpret_cast<T*>(dest) =
                        *reinterpret_cast<T const*>(src);
                }
            };
        };

        // static functions for big value-types (bigger than a void*)
        template <>
        struct fxns<mpl::false_>
        {
            template<typename T, typename Char>
            struct type
            {
                static boost::detail::sp_typeinfo const& get_type()
                {
                    return BOOST_SP_TYPEID(T);
                }
                static void static_delete(void** x)
                {
                    // destruct and free memory
                    delete (*reinterpret_cast<T**>(x));
                }
                static void destruct(void** x)
                {
                    // destruct only, we'll reuse memory
                    (*reinterpret_cast<T**>(x))->~T();
                }
                static void clone(void* const* src, void** dest)
                {
                    *dest = new T(**reinterpret_cast<T* const*>(src));
                }
                static void move(void* const* src, void** dest)
                {
                    **reinterpret_cast<T**>(dest) =
                        **reinterpret_cast<T* const*>(src);
                }
            };
        };

        template <typename T>
        struct get_table
        {
            typedef mpl::bool_<(sizeof(T) <= sizeof(void*))> is_small;

            template <typename Char>
            static fxn_ptr_table<Char>* get()
            {
                static fxn_ptr_table<Char> static_table =
                {
                    fxns<is_small>::template type<T, Char>::get_type,
                    fxns<is_small>::template type<T, Char>::static_delete,
                    fxns<is_small>::template type<T, Char>::destruct,
                    fxns<is_small>::template type<T, Char>::clone,
                    fxns<is_small>::template type<T, Char>::move,
                };
                return &static_table;
            }
        };

        ///////////////////////////////////////////////////////////////////////
        struct empty {};
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Char>
    class basic_hold_any
    {
    public:
        // constructors
        template <typename T>
        basic_hold_any(T const& x)
          : table(spirit::detail::get_table<T>::template get<Char>()), object(0)
        {
            if (spirit::detail::get_table<T>::is_small::value)
                new (&object) T(x);
            else
                object = new T(x);
        }

        basic_hold_any(const char* x)
          : table(spirit::detail::get_table< ::std::string>::template get<Char>()), object(0)
        {
            new (&object) ::std::string(x);
        }

        basic_hold_any()
          : table(spirit::detail::get_table<spirit::detail::empty>::template get<Char>()),
            object(0)
        {
        }

        basic_hold_any(basic_hold_any const& x)
          : table(spirit::detail::get_table<spirit::detail::empty>::template get<Char>()),
            object(0)
        {
            assign(x);
        }

        ~basic_hold_any()
        {
            table->static_delete(&object);
        }

        // assignment
        basic_hold_any& assign(basic_hold_any const& x)
        {
            if (&x != this) {
                // are we copying between the same type?
                if (table == x.table) {
                    // if so, we can avoid reallocation
                    table->move(&x.object, &object);
                }
                else {
                    reset();
                    x.table->clone(&x.object, &object);
                    table = x.table;
                }
            }
            return *this;
        }

        template <typename T>
        basic_hold_any& assign(T const& x)
        {
            // are we copying between the same type?
            spirit::detail::fxn_ptr_table<Char>* x_table =
                spirit::detail::get_table<T>::template get<Char>();
            if (table == x_table) {
            // if so, we can avoid deallocating and re-use memory
                table->destruct(&object);    // first destruct the old content
                if (spirit::detail::get_table<T>::is_small::value) {
                    // create copy on-top of object pointer itself
                    new (&object) T(x);
                }
                else {
                    // create copy on-top of old version
                    new (object) T(x);
                }
            }
            else {
                if (spirit::detail::get_table<T>::is_small::value) {
                    // create copy on-top of object pointer itself
                    table->destruct(&object); // first destruct the old content
                    new (&object) T(x);
                }
                else {
                    reset();                  // first delete the old content
                    object = new T(x);
                }
                table = x_table;      // update table pointer
            }
            return *this;
        }

        // assignment operator
        template <typename T>
        basic_hold_any& operator=(T const& x)
        {
            return assign(x);
        }

        // assignment operator
        basic_hold_any& operator=(basic_hold_any const& x)
        {
            return assign(x);
        }

        // utility functions
        basic_hold_any& swap(basic_hold_any& x)
        {
            std::swap(table, x.table);
            std::swap(object, x.object);
            return *this;
        }

        boost::detail::sp_typeinfo const& type() const
        {
            return table->get_type();
        }

        template <typename T>
        T const& cast() const
        {
            if (type() != BOOST_SP_TYPEID(T))
              throw bad_any_cast(type(), BOOST_SP_TYPEID(T));

            return spirit::detail::get_table<T>::is_small::value ?
                *reinterpret_cast<T const*>(&object) :
                *reinterpret_cast<T const*>(object);
        }

        const void* castsmallvoid() const
        {
          return &object;
        }

// implicit casting is disabled by default for compatibility with boost::any
#ifdef BOOST_SPIRIT_ANY_IMPLICIT_CASTING
        // automatic casting operator
        template <typename T>
        operator T const& () const { return cast<T>(); }
#endif // implicit casting

        bool empty() const
        {
            return table == spirit::detail::get_table<spirit::detail::empty>::template get<Char>();
        }

        void reset()
        {
            if (!empty())
            {
                table->static_delete(&object);
                table = spirit::detail::get_table<spirit::detail::empty>::template get<Char>();
                object = 0;
            }
        }

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
    private: // types
        template <typename T, typename Char_>
        friend T* any_cast(basic_hold_any<Char_> *);
#else
    public: // types (public so any_cast can be non-friend)
#endif
        // fields
        spirit::detail::fxn_ptr_table<Char>* table;
        void* object;
    };

    // boost::any-like casting
    template <typename T, typename Char>
    inline T* any_cast (basic_hold_any<Char>* operand)
    {
        if (operand && operand->type() == BOOST_SP_TYPEID(T)) {
            return spirit::detail::get_table<T>::is_small::value ?
                reinterpret_cast<T*>(&operand->object) :
                reinterpret_cast<T*>(operand->object);
        }
        return 0;
    }

    template <typename T, typename Char>
    inline T const* any_cast(basic_hold_any<Char> const* operand)
    {
        return any_cast<T>(const_cast<basic_hold_any<Char>*>(operand));
    }

    template <typename T, typename Char>
    T any_cast(basic_hold_any<Char>& operand)
    {
        typedef BOOST_DEDUCED_TYPENAME remove_reference<T>::type nonref;

#ifdef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
        // If 'nonref' is still reference type, it means the user has not
        // specialized 'remove_reference'.

        // Please use BOOST_BROKEN_COMPILER_TYPE_TRAITS_SPECIALIZATION macro
        // to generate specialization of remove_reference for your class
        // See type traits library documentation for details
        BOOST_STATIC_ASSERT(!is_reference<nonref>::value);
#endif

        nonref* result = any_cast<nonref>(&operand);
        if(!result)
            boost::throw_exception(bad_any_cast(operand.type(), BOOST_SP_TYPEID(T)));
        return *result;
    }

    template <typename T, typename Char>
    T const& any_cast(basic_hold_any<Char> const& operand)
    {
        typedef BOOST_DEDUCED_TYPENAME remove_reference<T>::type nonref;

#ifdef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
        // The comment in the above version of 'any_cast' explains when this
        // assert is fired and what to do.
        BOOST_STATIC_ASSERT(!is_reference<nonref>::value);
#endif

        return any_cast<nonref const&>(const_cast<basic_hold_any<Char> &>(operand));
    }

    ///////////////////////////////////////////////////////////////////////////////
    // backwards compatibility
    typedef basic_hold_any<char> hold_any;
    typedef basic_hold_any<wchar_t> whold_any;

    namespace traits
    {
        template <typename T>
        struct is_hold_any : mpl::false_ {};

        template <typename Char>
        struct is_hold_any<basic_hold_any<Char> > : mpl::true_ {};
    }

}}    // namespace boost::spirit

///////////////////////////////////////////////////////////////////////////////
#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
# pragma warning(pop)
#endif

#endif