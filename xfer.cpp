#include <string>
#include <string_view>

// Replace every "needle" with "needle+suffix", skipping ones already suffixed.
static inline void replace_in(std::string& s,
                              std::string_view needle,
                              std::string_view suffix)
{
    const std::string repl = std::string(needle) + std::string(suffix);
    size_t pos = 0;
    while ((pos = s.find(needle, pos)) != std::string::npos) {
        if (pos + needle.size() <= s.size() &&
            s.compare(pos + needle.size(), suffix.size(), suffix) == 0)
        {
            pos += needle.size() + suffix.size(); // already "...-IC"
            continue;
        }
        s.replace(pos, needle.size(), repl);
        pos += repl.size();
    }
}

static inline bool patch_node_text(rapidxml_sv::xml_document<>& doc,
                                   rapidxml_sv::xml_node<>* n,
                                   std::string_view needle,
                                   std::string_view suffix)
{
    if (!n) return false;
    const auto len = static_cast<size_t>(n->value_size());
    if (len == 0) return false;

    // Build a view safely (works whether value() is char* or string_view)
    std::string_view v{ n->value(), len };
    if (v.find(needle) == std::string::npos) return false;

    std::string s(v);
    replace_in(s, needle, suffix);
    if (s.size() == v.size() && std::equal(s.begin(), s.end(), v.begin())) return false;

    // Allocate from doc and set BOTH pointer and size:
    char* mem = doc.allocate_string(s.data(), s.size());
    n->value(mem, s.size());
    return true;
}

static inline bool patch_attr_text(rapidxml_sv::xml_document<>& doc,
                                   rapidxml_sv::xml_attribute<>* a,
                                   std::string_view needle,
                                   std::string_view suffix)
{
    if (!a) return false;
    const auto len = static_cast<size_t>(a->value_size());
    if (len == 0) return false;

    std::string_view v{ a->value(), len };
    if (v.find(needle) == std::string::npos) return false;

    std::string s(v);
    replace_in(s, needle, suffix);
    if (s.size() == v.size() && std::equal(s.begin(), s.end(), v.begin())) return false;

    char* mem = doc.allocate_string(s.data(), s.size());
    a->value(mem, s.size());
    return true;
}

static void replaceAllModelName_IC(rapidxml_sv::xml_document<>& doc,
                                   std::string_view needle = "MyModelName",
                                   std::string_view suffix = "-IC")
{
    std::function<void(rapidxml_sv::xml_node<>*)> walk =
        [&](rapidxml_sv::xml_node<>* n) {
            if (!n) return;

            // Element's own text
            patch_node_text(doc, n, needle, suffix);

            // Attributes
            for (auto* a = n->first_attribute(); a; a = a->next_attribute())
                patch_attr_text(doc, a, needle, suffix);

            // Data/CDATA children
            for (auto* c = n->first_node(); c; c = c->next_sibling()) {
                if (c->type() == rapidxml_sv::node_data ||
                    c->type() == rapidxml_sv::node_cdata)
                {
                    patch_node_text(doc, c, needle, suffix);
                }
            }

            // Recurse into element children
            for (auto* c = n->first_node(); c; c = c->next_sibling())
                if (c->type() == rapidxml_sv::node_element) walk(c);
        };

    walk(doc.first_node());
}
