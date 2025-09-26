#include <rapidxml.hpp>
#include <rapidxml_print.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <functional>

using namespace rapidxml;

template <class T>
static inline std::string_view get_view(T* base) {
    // Works whether value() returns char*, std::string, or std::string_view
    auto v_any = base->value();
    using V = std::decay_t<decltype(v_any)>;

    if constexpr (std::is_pointer_v<V>) {                 // char*
        return std::string_view(v_any,
                                static_cast<size_t>(base->value_size()));
    } else if constexpr (std::is_same_v<V, std::string>) { // std::string
        return std::string_view(v_any.data(), v_any.size());
    } else {                                               // std::string_view
        return v_any;
    }
}

static inline void replace_substr_inplace(std::string& s,
                                          std::string_view needle,
                                          std::string_view suffix)
{
    const std::string repl = std::string(needle) + std::string(suffix);
    size_t pos = 0;
    while ((pos = s.find(needle, pos)) != std::string::npos) {
        // Skip if already suffixed
        if (pos + needle.size() + suffix.size() <= s.size() &&
            s.compare(pos + needle.size(), suffix.size(), suffix) == 0) {
            pos += needle.size() + suffix.size();
            continue;
        }
        s.replace(pos, needle.size(), repl);
        pos += repl.size();
    }
}

template <class Base>  // xml_node<>* or xml_attribute<>*
static inline bool patch_value(xml_document<>& doc, Base* base,
                               std::string_view needle, std::string_view suffix)
{
    std::string_view v = get_view(base);
    if (v.empty() || v.find(needle) == std::string_view::npos) return false;

    std::string s(v);
    replace_substr_inplace(s, needle, suffix);
    if (s == v) return false;

    base->value(doc.allocate_string(s.c_str())); // write back
    return true;
}

static void replaceAllModelName(xml_document<>& doc,
                                std::string_view needle = "MyModelName",
                                std::string_view suffix = "-IC")
{
    std::function<void(xml_node<>*)> walk = [&](xml_node<>* n) {
        if (!n) return;

        // Element text
        if (n->value_size() > 0) {
            if (patch_value(doc, n, needle, suffix)) {
                auto now = get_view(n);
                // DEBUG (re-read after modifying!)
                // std::cerr << "Element text -> " << std::string(now) << "\n";
            }
        }

        // Attributes
        for (auto* a = n->first_attribute(); a; a = a->next_attribute()) {
            if (a->value_size() > 0) {
                if (patch_value(doc, a, needle, suffix)) {
                    auto now = get_view(a);
                    // std::cerr << "Attr " << a->name() << " -> " << std::string(now) << "\n";
                }
            }
        }

        // Data/CDATA
        for (auto* c = n->first_node(); c; c = c->next_sibling()) {
            if ((c->type() == node_data || c->type() == node_cdata) &&
                 c->value_size() > 0) {
                if (patch_value(doc, c, needle, suffix)) {
                    auto now = get_view(c);
                    // std::cerr << "CDATA -> " << std::string(now) << "\n";
                }
            }
        }

        // Recurse elements
        for (auto* c = n->first_node(); c; c = c->next_sibling())
            if (c->type() == node_element) walk(c);
    };

    walk(doc.first_node());
}
