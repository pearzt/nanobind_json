/***************************************************************************
* Copyright (c) 2019, Martin Renou                                         *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#pragma once

#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "nanobind/nanobind.hpp"

namespace nb = nanobind;
namespace nl = nlohmann;

namespace pyjson
{
    inline nb::object from_json(const nl::json& j)
    {
        if (j.is_null())
        {
            return nb::none();
        }
        else if (j.is_boolean())
        {
            return nb::bool_(j.get<bool>());
        }
        else if (j.is_number_integer())
        {
            return nb::int_(j.get<long>());
        }
        else if (j.is_number_float())
        {
            return nb::float_(j.get<double>());
        }
        else if (j.is_string())
        {
            return nb::str(j.get<std::string>());
        }
        else if (j.is_array())
        {
            nb::list obj;
            for (const auto& el : j)
            {
                obj.append(from_json(el));
            }
            return std::move(obj);
        }
        else // Object
        {
            nb::dict obj;
            for (nl::json::const_iterator it = j.cbegin(); it != j.cend(); ++it)
            {
                obj[nb::str(it.key())] = from_json(it.value());
            }
            return std::move(obj);
        }
    }

    inline nl::json to_json(const nb::handle& obj)
    {
        if (obj.ptr() == nullptr || obj.is_none())
        {
            return nullptr;
        }
        if (nb::isinstance<nb::bool_>(obj))
        {
            return obj.cast<bool>();
        }
        if (nb::isinstance<nb::int_>(obj))
        {
            return obj.cast<long>();
        }
        if (nb::isinstance<nb::float_>(obj))
        {
            return obj.cast<double>();
        }
        if (nb::isinstance<nb::bytes>(obj))
        {
            nb::module base64 = nb::module::import("base64");
            return base64.attr("b64encode")(obj).attr("decode")("utf-8").cast<std::string>();
        }
        if (nb::isinstance<nb::str>(obj))
        {
            return obj.cast<std::string>();
        }
        if (nb::isinstance<nb::tuple>(obj) || nb::isinstance<nb::list>(obj))
        {
            auto out = nl::json::array();
            for (const nb::handle value : obj)
            {
                out.push_back(to_json(value));
            }
            return out;
        }
        if (nb::isinstance<nb::dict>(obj))
        {
            auto out = nl::json::object();
            for (const nb::handle key : obj)
            {
                out[nb::str(key).cast<std::string>()] = to_json(obj[key]);
            }
            return out;
        }
        throw std::runtime_error("to_json not implemented for this type of object: " + nb::repr(obj).cast<std::string>());
    }
}

// nlohmann_json serializers
namespace nlohmann
{
    #define MAKE_NLJSON_SERIALIZER_DESERIALIZER(T)         \
    template <>                                            \
    struct adl_serializer<T>                               \
    {                                                      \
        inline static void to_json(json& j, const T& obj)  \
        {                                                  \
            j = pyjson::to_json(obj);                      \
        }                                                  \
                                                           \
        inline static T from_json(const json& j)           \
        {                                                  \
            return pyjson::from_json(j);                   \
        }                                                  \
    };

    #define MAKE_NLJSON_SERIALIZER_ONLY(T)                 \
    template <>                                            \
    struct adl_serializer<T>                               \
    {                                                      \
        inline static void to_json(json& j, const T& obj)  \
        {                                                  \
            j = pyjson::to_json(obj);                      \
        }                                                  \
    };

    MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::object);

    MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::bool_);
    MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::int_);
    MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::float_);
    MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::str);

    MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::list);
    MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::tuple);
    MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::dict);

    MAKE_NLJSON_SERIALIZER_ONLY(nb::handle);
    MAKE_NLJSON_SERIALIZER_ONLY(nb::detail::item_accessor);
    MAKE_NLJSON_SERIALIZER_ONLY(nb::detail::list_accessor);
    MAKE_NLJSON_SERIALIZER_ONLY(nb::detail::tuple_accessor);
    MAKE_NLJSON_SERIALIZER_ONLY(nb::detail::sequence_accessor);
    MAKE_NLJSON_SERIALIZER_ONLY(nb::detail::str_attr_accessor);
    MAKE_NLJSON_SERIALIZER_ONLY(nb::detail::obj_attr_accessor);

    #undef MAKE_NLJSON_SERIALIZER
    #undef MAKE_NLJSON_SERIALIZER_ONLY
}

// nanobind caster
namespace nanobind
{
    namespace detail
    {
        template <> struct type_caster<nl::json>
        {
        public:
            NANOBIND_TYPE_CASTER(nl::json, _("json"));

            bool load(handle src, bool)
            {
                try {
                    value = pyjson::to_json(src);
                    return true;
                }
                catch (...)
                {
                    return false;
                }
            }

            static handle cast(nl::json src, return_value_policy /* policy */, handle /* parent */)
            {
                object obj = pyjson::from_json(src);
                return obj.release();
            }
        };
    }
}
