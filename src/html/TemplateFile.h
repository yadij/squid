/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID__SRC_HTML_TEMPLATEFILE_H
#define SQUID__SRC_HTML_TEMPLATEFILE_H

#include "error/forward.h"
#include "html/forward.h"
#include "http/forward.h"
#include "sbuf/SBuf.h"
#include "SquidString.h"

namespace Html
{

/**
 * loads text templates used for error pages and details;
 * supports translation of templates
 */
class TemplateFile
{
public:
    TemplateFile(const char *name, const err_type code):
        templateName(name),
        templateCode(code)
    {
        assert(name);
    }

    virtual ~TemplateFile() {}

    /// return true if the data loaded from disk without any problem
    bool loaded() const {return wasLoaded;}

    /**
     * Load the page_name template from a file which  probably exist at:
     *  (a) admin specified custom directory (error_directory)
     *  (b) default language translation directory (error_default_language)
     *  (c) English sub-directory where errors should ALWAYS exist
     * If all of the above fail, setDefault() is called.
     */
    void loadDefault();

    /**
     * Load an error template for a given HTTP request. This function examines the
     * Accept-Language header and select the first available template.
     * \retval false if default template selected (eg because of a "Accept-Language: *")
     * \retval false if not available template found
     */
    bool loadFor(const HttpRequest *);

    /**
     * Load the given file. It uses the parse() method.
     * \reval true On success, and sets wasLoaded
     */
    bool loadFromFile(const char *path);

    /// The language used for the template
    const char *language() {return errLanguage.termedBuf();}

    SBuf filename; ///< where the template was loaded from

    /// Whether to print error messages on cache.log file or not. It is user defined.
    bool silent = false;

protected:
    /// post-process the loaded template
    virtual bool parse() { return true; }

    /// recover from loadDefault() failure to load or parse() a template
    virtual void setDefault() {}

    /**
     * Try to load the "page_name" template for a given language "lang"
     * from squid errors directory
     * \return true on success false otherwise
     */
    bool tryLoadTemplate(const char *lang);

    SBuf template_; ///< raw template contents
    bool wasLoaded = false; ///< True if the template data read from disk without any problem
    String errLanguage; ///< The error language of the template.
    String templateName; ///< The name of the template
    err_type templateCode; ///< The internal code for this template.
};

} // namespace Html

#endif /* SQUID__SRC_HTML_TEMPLATEFILE_H */

