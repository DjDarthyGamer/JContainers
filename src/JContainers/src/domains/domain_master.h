#pragma once

#include <string>
#include <set>
#include <map>
#include <iosfwd>
#include <memory>

#include "forms/form_observer.h"
#include "collections/context.h"
#include "util/istring.h"

namespace domain_master {

    using context = ::collections::tes_context;
    using form_observer = ::forms::form_observer;

    class master {
    public:

        master()
            : _default_domain(_form_watcher)
        {}

        std::set<util::istring> active_domain_names;

        context& get_or_create_domain_with_name(const util::istring& name);// or create if none
        context* get_domain_if_active(const util::istring& name);
        context& get_default_domain();

        using DomainsMap = std::map<util::istring, std::shared_ptr<context>>;

        DomainsMap& active_domains_map();

        form_observer& get_form_observer();


        static master& instance();

        void clear_state();
        void read_from_stream(std::istream&);
        void write_to_stream(std::ostream&);

        // save from stream / load from stream
        // drop (or not save?) loaded contexts if no appropriate config files found?

        // burden of C++ constness
        const form_observer& get_form_observer() const { return _form_watcher; }
        const context& get_default_domain() const { return _default_domain; }
        const DomainsMap& active_domains_map() const { return _domains; }

    private:
        // Since it's not a real implementation yet:
        form_observer _form_watcher;
        context _default_domain;
        DomainsMap _domains;
    };
}