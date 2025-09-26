// drop-in
void replaceOccurancesInTags(rapidxml_sv::xml_document<>& doc,
                             const std::vector<std::string>& tags,
                             const char* from, const char* to)
{
    std::unordered_set<std::string> tagset(tags.begin(), tags.end());
    const auto repl = [&](std::string& s)->bool{
        bool changed=false;
        size_t pos=0, flen=std::strlen(from), tlen=std::strlen(to);
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, flen, to);
            pos += tlen;
            changed = true;
        }
        return changed;
    };
    std::function<void(rapidxml_sv::xml_node<>*)> dfs =
    [&](rapidxml_sv::xml_node<>* n){
        if (!n) return;
        if (tagset.count(std::string(n->name()))) {
            // node value
            {
                std::string oldv = n->value();
                std::string newv = oldv;
                if (repl(newv) && newv != oldv) {
                    std::cout << "node <" << n->name() << ">: \"" << oldv
                              << "\" -> \"" << newv << "\"\n";
                    char* nv = doc.allocate_string(newv.c_str());
                    n->value(nv);
                }
            }
            // attributes
            for (auto* a = n->first_attribute(); a; a = a->next_attribute()) {
                std::string olda = a->value();
                std::string newa = olda;
                if (repl(newa) && newa != olda) {
                    std::cout << "attr " << a->name() << " on <" << n->name()
                              << ">: \"" << olda << "\" -> \"" << newa << "\"\n";
                    char* nv = doc.allocate_string(newa.c_str());
                    a->value(nv);
                }
            }
        }
        for (auto* c = n->first_node(); c; c = c->next_sibling()) dfs(c);
    };
    dfs(doc.first_node());
}
