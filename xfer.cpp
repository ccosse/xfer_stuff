#include <rapidxml.hpp>
#include <rapidxml_print.hpp>
#include <string>
#include <functional>
using namespace rapidxml;

static void replaceAllModelName(xml_document<>& doc,
                                const char* needle_cstr = "MyModelName",
                                const char* suffix_cstr = "-IC")
{
    const std::string needle(needle_cstr);
    const std::string replacement = needle + suffix_cstr;

    auto replace_in = [&](std::string& s) {
        size_t pos = 0;
        while ((pos = s.find(needle, pos)) != std::string::npos) {
            // If it’s already "MyModelName-IC", skip this occurrence
            if (pos + needle.size() + 3 <= s.size() &&
                s[pos + needle.size()] == '-' &&
                s[pos + needle.size() + 1] == 'I' &&
                s[pos + needle.size() + 2] == 'C') {
                pos += needle.size() + 3;
                continue;
            }
            s.replace(pos, needle.size(), replacement);
            pos += replacement.size();
        }
    };

    std::function<void(xml_node<>*)> walk = [&](xml_node<>* n) {
        if (!n) return;

        // Element’s own text (if any)
        if (n->value() && *n->value()) {
            std::string v = n->value();
            std::string before = v;
            replace_in(v);
            if (v != before) n->value(doc.allocate_string(v.c_str()));
        }

        // Attributes
        for (auto* a = n->first_attribute(); a; a = a->next_attribute()) {
            std::string v = a->value();
            std::string before = v;
            replace_in(v);
            if (v != before) a->value(doc.allocate_string(v.c_str()));
        }

        // Data/CDATA children (text nodes)
        for (auto* c = n->first_node(); c; c = c->next_sibling()) {
            if (c->type() == node_data || c->type() == node_cdata) {
                std::string v = c->value();
                std::string before = v;
                replace_in(v);
                if (v != before) c->value(doc.allocate_string(v.c_str()));
            }
        }

        // Recurse into element children
        for (auto* c = n->first_node(); c; c = c->next_sibling())
            if (c->type() == node_element) walk(c);
    };

    walk(doc.first_node());
}
