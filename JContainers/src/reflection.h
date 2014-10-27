#pragma once

#include <string>
#include <vector>
#include <assert.h>
#include <algorithm>
#include <stdint.h>

#include "meta.h"

class VMClassRegistry;

namespace reflection {

    struct function_parameter {
        std::string tes_type_name;
        std::string tes_arg_name;
    };

    inline function_parameter function_parameter_make(const char * type_name, const char * arg_name) {
        return { type_name ? type_name : "", arg_name ? arg_name : "" };
    }

    typedef function_parameter (*type_info_func)();

    typedef void* tes_api_function;
    typedef void* c_function;

    struct function_info {
        typedef std::string (*comment_generator)();
        typedef  void (*tes_function_binder)(VMClassRegistry* registry, const char* className, const char* funcName);
        typedef  std::vector<type_info_func> (*parameter_list_creator)();

        tes_function_binder registrator = nullptr;
        parameter_list_creator param_list_func = nullptr;
        tes_api_function tes_func = nullptr;
        c_function c_func = nullptr;
        const char *argument_names = nullptr;
        const char *name = nullptr;

        comment_generator _comment_func = nullptr;
        const char *_comment_str = nullptr;

        std::string comment() const {
            if (_comment_func) {
                return _comment_func();
            }

            return _comment_str ? _comment_str : "";
        }

        void setComment(comment_generator func) {
            _comment_func = func;
        }

        void setComment(nullptr_t) {
            _comment_func = nullptr;
            _comment_str = nullptr;
        }

        void setComment(const char * comment) {
            _comment_str = comment;
        }

        void bind(VMClassRegistry *registry, const char *className) const;
    };

    struct string_icomparison {
        bool operator () (const std::string& left, const std::string& right) const {
            return _strcmpi(left.c_str(), right.c_str()) < 0;
        }
    };

    inline bool string_iequal(const std::string& str, const std::string& other) {
        return _strcmpi(str.c_str(), other.c_str()) == 0;
    }

    inline bool string_iequal(const std::string& str, const char* other) {
        return other && _strcmpi(str.c_str(), other) == 0;
    }

    struct class_info {

        std::vector<function_info > methods;
        std::string _className;
        std::string extendsClass;
        std::string comment;
        uint32_t version = 0;

        class_info() {
            _className = "NoClassName";
        }

        bool initialized() const {
            return !_className.empty();
        }

        std::string className() const {
#   if 0
            return _className + '_' + (char)((uint32_t)'0' + version);
#   else
            return _className;
#   endif
        }

        const function_info * find_function(const char* func_name) const {
            auto itr = std::find_if(methods.begin(), methods.end(), [&](const function_info& fi) { return string_iequal(func_name, fi.name); });
            return itr != methods.end() ? &*itr : nullptr;
        }

        void addFunction(const function_info& info) {
            assert(find_function(info.name) == nullptr);
            methods.push_back(info);
        }

        void bind(VMClassRegistry* registry) const {
            assert(initialized());

            auto clsName = className();
            for (const auto& itm : methods) {
                itm.bind(registry, clsName.c_str());
            }
        }

        void merge_with_extension(const class_info& extension) {
            assert(initialized());
            assert(className() == extension.className());
            assert(extendsClass == extension.extendsClass);

            for (const auto& itm : extension.methods) {
                addFunction(itm);
            }
        }
    };

    // Produces script files using meta class information
    namespace code_producer {

        std::string produceClassCode(const class_info& self);
        void produceClassToFile(const class_info& self, const std::string& directoryPath);

    }

    namespace _detail {

        struct class_meta_mixin {
            class_info metaInfo;
            virtual void additionalSetup() {}
        };

        template<class T>
        struct class_meta_mixin_t : class_meta_mixin  {

            static class_info metaInfoFunc() {
                T t;
                t.additionalSetup();
                return t.metaInfo;
            }
            // special support for hack inside REGISTERF macro
            typedef T __Type;
        };
    }
    template <class T>
    using class_meta_mixin_t = _detail::class_meta_mixin_t<T>;

    typedef class_info (*class_info_creator)();

    const std::map<std::string, class_info, string_icomparison>& class_database();
    void* find_tes_function_of_class(const char * functionName, const char *className);

    template<class T>
    inline void foreach_metaInfo_do(T& func) {

        for (auto & pair : class_database()) {
            func(pair.second);
        }
    }

#   define TES_META_INFO(Class)    \
        static const ::meta<::reflection::class_info_creator > g_tesMetaFunc_##Class ( &Class::metaInfoFunc );

}
