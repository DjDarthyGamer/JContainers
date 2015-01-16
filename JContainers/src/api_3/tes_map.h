namespace tes_api_3 {

    using namespace collections;


    template<class Key, class Cnt>
    class tes_map_t : public class_meta< tes_map_t<Key, Cnt> > {
    public:

        using map_functions = map_functions_templ < Cnt >;
        typedef typename Cnt* ref;

        tes_map_t() {
            metaInfo.comment = "Associative key-value container.\n"
                "Inherits JValue functionality";
        }

        REGISTERF(tes_object::object<Cnt>, "object", "", kCommentObject);

        template<class T>
        static T getItem(Cnt *obj, Key key, T def = T(0)) {
            map_functions::doReadOp(obj, key, [&](Item& itm) { def = itm.readAs<T>(); });
            return def;
        }
        REGISTERF(getItem<SInt32>, "getInt", "object key default=0", "returns value associated with key");
        REGISTERF(getItem<Float32>, "getFlt", "object key default=0.0", "");
        REGISTERF(getItem<BSFixedString>, "getStr", "object key default=\"\"", "");
        REGISTERF(getItem<object_base*>, "getObj", "object key default=0", "");
        REGISTERF(getItem<TESForm*>, "getForm", "object key default=None", "");

        template<class T>
        static void setItem(Cnt *obj, Key key, T item) {
            map_functions::doWriteOp(obj, key, [&](Item& itm) { itm = item; });
        }
        REGISTERF(setItem<SInt32>, "setInt", "* key value", "creates key-value association. replaces existing value if any");
        REGISTERF(setItem<Float32>, "setFlt", "* key value", "");
        REGISTERF(setItem<const char *>, "setStr", "* key value", "");
        REGISTERF(setItem<object_base*>, "setObj", "* key container", "");
        REGISTERF(setItem<TESForm*>, "setForm", "* key value", "");

        static bool hasKey(ref obj, Key key) {
            return valueType(obj, key) != 0;
        }
        REGISTERF2(hasKey, "* key", "returns true, if something associated with key");

        static SInt32 valueType(ref obj, Key key) {
            auto type = item_type::no_item;
            map_functions::doReadOp(obj, key, [&](Item& itm) { type = itm.type(); });
            return (SInt32)type;
        }
        REGISTERF2(valueType, "* key", "returns type of the value associated with key.\n"VALUE_TYPE_COMMENT);

        static object_base* allKeys(Cnt* obj) {
            if (!obj) {
                return nullptr;
            }

            return &array::objectWithInitializer([&](array &arr) {
                object_lock g(obj);

                arr._array.reserve(obj->u_count());
                for each(auto& pair in obj->u_container()) {
                    arr.u_container().push_back(Item(pair.first));
                }
            },
                tes_context::instance());
        }
        REGISTERF(allKeys, "allKeys", "*", "returns new array containing all keys");

        static object_base* allValues(Cnt *obj) {
            if (!obj) {
                return nullptr;
            }

            return &array::objectWithInitializer([&](array &arr) {
                object_lock g(obj);

                arr._array.reserve(obj->u_count());
                for each(auto& pair in obj->u_container()) {
                    arr._array.push_back(pair.second);
                }
            },
                tes_context::instance());
        }
        REGISTERF(allValues, "allValues", "*", "returns new array containing all values");

        static bool removeKey(Cnt *obj, Key key) {
            if (obj && map_key_checker::check(key)) {
                return obj->erase(key);
            }
            return false;
        }
        REGISTERF(removeKey, "removeKey", "* key", "destroys key-value association");

        static SInt32 count(ref obj) {
            if (!obj) {
                return 0;
            }

            return obj->s_count();
        }
        REGISTERF2(count, "*", "returns count of items/associations");

        static void clear(ref obj) {
            if (!obj) {
                return;
            }

            obj->s_clear();
        }
        REGISTERF2(clear, "*", "removes all items from container");

        static void addPairs(ref obj, const ref source, bool overrideDuplicates) {
            if (!obj || !source || source == obj) {
                return;
            }

            object_lock g(obj);
            object_lock c(source);

            if (overrideDuplicates) {
                for (const auto& pair : source->u_container()) {
                    obj->u_container()[pair.first] = pair.second;
                }
            }
            else {
                obj->u_container().insert(source->u_container().begin(), source->u_container().end());
            }
        }
        REGISTERF2(addPairs, "* source overrideDuplicates", "inserts key-value pairs from the source map");

        void additionalSetup();
    };

    typedef tes_map_t<const char*, map > tes_map;
    typedef tes_map_t<FormId, form_map> tes_form_map;

    void tes_map::additionalSetup() {
        metaInfo._className = "JMap";
    }

    void tes_form_map::additionalSetup() {
        metaInfo._className = "JFormMap";
    }

    TES_META_INFO(tes_map);
    TES_META_INFO(tes_form_map);

    const char *tes_map_nextKey_comment =
        "Simplifies iteration over container's contents.\nIncrements and returns previous key, pass defaulf parameter to begin iteration. Usage:\n"
        "string key = JMap.nextKey(map)\n"
        "while key\n"
        "  <retrieve values here>\n"
        "  key = JMap.nextKey(map, key)\n"
        "endwhile";

    struct tes_map_ext : class_meta < tes_map_ext > {
        REGISTER_TES_NAME("JMap");
        template<class Key>
        static Key nextKey(map* obj, const char* previousKey = "", const char * endKey = "") {
            Key str(endKey);
            map_functions::nextKey(obj, previousKey, [&](const std::string& key) { str = key.c_str(); });
            return str;
        }
        REGISTERF(nextKey<BSFixedString>, "nextKey", STR(* previousKey="" endKey=""), tes_map_nextKey_comment);

        static const char * getNthKey_comment() { return "Retrieves N-th key. "NEGATIVE_IDX_COMMENT "\nWorst complexity is O(n/2)"; }

        template<class Key>
        static Key getNthKey(map* obj, SInt32 keyIndex) {
            Key ith;
            map_functions::getNthKey(obj, keyIndex, [&](const std::string& key) { ith = key.c_str(); });
            return ith;
        }
        REGISTERF(getNthKey<BSFixedString>, "getNthKey", "* keyIndex", getNthKey_comment());
    };

    struct tes_form_map_ext : class_meta < tes_form_map_ext > {
        REGISTER_TES_NAME("JFormMap");
        static FormId nextKey(form_map* obj, FormId previousKey = FormZero, FormId endKey = FormZero) {
            FormId k = endKey;
            formmap_functions::nextKey(obj, previousKey, [&](const FormId& key) { k = key; });
            return k;
        }
        REGISTERF(nextKey, "nextKey", STR(* previousKey=None endKey=None), tes_map_nextKey_comment);

        static FormId getNthKey(form_map* obj, SInt32 keyIndex) {
            FormId ith = FormZero;
            formmap_functions::getNthKey(obj, keyIndex, [&](const FormId& key) { ith = key; });
            return ith;
        }
        REGISTERF(getNthKey, "getNthKey", "* keyIndex", tes_map_ext::getNthKey_comment());
    };

    TES_META_INFO(tes_map_ext);
    TES_META_INFO(tes_form_map_ext);
}
