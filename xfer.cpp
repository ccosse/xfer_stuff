#include <rapidxml.hpp>
#include <rapidxml_print.hpp>
#include <string>
#include <string_view>
#include <functional>
using namespace rapidxml;

static void replaceAllModelName(xml_document<>& doc,
                                std::string_view needle = "MyModelName",
                                std::string_view suffix = "-IC")
{
    const std::string replacement = std::string(needle) + std::string(suffix);

    auto replace_in = [&](std::string& s) {
        size_t pos = 0;
        while ((pos = s.find(needle, pos)) != std::string::npos) {
            // Skip if already "...MyModelName-IC"
            if (pos + needle.size() + suffix.size() <= s.size() &&
                s.compare(pos + needle.size(), suffix.size(), suffix) == 0) {
                pos += needle.size() + suffix.size();
                continue;
            }
            s.replace(pos, needle.size(), replacement);
            pos += replacement.size();
        }
    };

    auto patch_value = [&](auto* base /* xml_node<>* or xml_attribute<>* */) {
        // Build a view from pointer + size; works regardless of return type quirks.
        std::string_view v{ base->value(), static_cast<size_t>(base->value_size()) };
        if (v.empty()) return;

        std::string s(v);         // owning buffer to mutate
        replace_in(s);
        if (s != v) base->value(doc.allocate_string(s.c_str()));
    };

    std::function<void(xml_node<>*)> walk = [&](xml_node<>* n) {
        if (!n) return;

        // Element text in the element itself
        if (n->value_size() > 0) patch_value(n);

        // Attributes
        for (auto* a = n->first_attribute(); a; a = a->next_attribute())
            if (a->value_size() > 0) patch_value(a);

        // Data/CDATA text nodes
        for (auto* c = n->first_node(); c; c = c->next_sibling())
            if ((c->type() == node_data || c->type() == node_cdata) && c->value_size() > 0)
                patch_value(c);

        // Recurse into element children
        for (auto* c = n->first_node(); c; c = c->next_sibling())
            if (c->type() == node_element) walk(c);
    };

    walk(doc.first_node());
}
