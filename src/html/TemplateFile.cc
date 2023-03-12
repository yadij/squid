/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "debug/Stream.h"
#include "errorpage.h"
#include "fde.h"
#include "fs_io.h"
#include "html/TemplateFile.h"
#include "HttpRequest.h"
#include "SquidConfig.h"

void
Html::TemplateFile::loadDefault()
{
    if (loaded()) // already loaded?
        return;

    /** test error_directory configured location */
    if (Config.errorDirectory) {
        char path[MAXPATHLEN];
        snprintf(path, sizeof(path), "%s/%s", Config.errorDirectory, templateName.termedBuf());
        loadFromFile(path);
    }

#if USE_ERR_LOCALES
    /** test error_default_language location */
    if (!loaded() && Config.errorDefaultLanguage) {
        if (!tryLoadTemplate(Config.errorDefaultLanguage)) {
            debugs(1, (templateCode < TCP_RESET ? DBG_CRITICAL : 3), "ERROR: Unable to load default error language files. Reset to backups.");
        }
    }
#endif

    /* test default location if failed (templates == English translation base templates) */
    if (!loaded()) {
        tryLoadTemplate("templates");
    }

    /* giving up if failed */
    if (!loaded()) {
        debugs(1, (templateCode < TCP_RESET ? DBG_CRITICAL : 3), "WARNING: failed to find or read error text file " << templateName);
        template_.clear();
        setDefault();
        wasLoaded = true;
    }
}

bool
Html::TemplateFile::tryLoadTemplate(const char *lang)
{
    assert(lang);

    char path[MAXPATHLEN];
    /* TODO: prep the directory path string to prevent snprintf ... */
    snprintf(path, sizeof(path), DEFAULT_SQUID_DATA_DIR "/errors/%s/%s",
             lang, templateName.termedBuf());
    path[MAXPATHLEN-1] = '\0';

    if (loadFromFile(path))
        return true;

#if HAVE_GLOB
    if (strlen(lang) == 2) {
        /* TODO glob the error directory for sub-dirs matching: <tag> '-*'   */
        /* use first result. */
        debugs(4,2, "wildcard fallback errors not coded yet.");
    }
#endif

    return false;
}

bool
Html::TemplateFile::loadFromFile(const char *path)
{
    int fd;
    char buf[4096];
    ssize_t len;

    if (loaded()) // already loaded?
        return true;

    fd = file_open(path, O_RDONLY | O_TEXT);

    if (fd < 0) {
        /* with dynamic locale negotiation we may see some failures before a success. */
        if (!silent && templateCode < TCP_RESET) {
            int xerrno = errno;
            debugs(4, DBG_CRITICAL, "ERROR: loading file '" << path << "': " << xstrerr(xerrno));
        }
        wasLoaded = false;
        return wasLoaded;
    }

    template_.clear();
    while ((len = FD_READ_METHOD(fd, buf, sizeof(buf))) > 0) {
        template_.append(buf, len);
    }

    if (len < 0) {
        int xerrno = errno;
        file_close(fd);
        debugs(4, DBG_CRITICAL, MYNAME << "ERROR: failed to fully read: '" << path << "': " << xstrerr(xerrno));
        wasLoaded = false;
        return false;
    }

    file_close(fd);

    filename = SBuf(path);

    if (!parse()) {
        debugs(4, DBG_CRITICAL, "ERROR: parsing error in template file: " << path);
        wasLoaded = false;
        return false;
    }

    wasLoaded = true;
    return wasLoaded;
}

bool
Html::TemplateFile::loadFor(const HttpRequest *request)
{
    String hdr;

#if USE_ERR_LOCALES
    if (loaded()) // already loaded?
        return true;

    if (!request || !request->header.getList(Http::HdrType::ACCEPT_LANGUAGE, &hdr))
        return false;

    char lang[256];
    size_t pos = 0; // current parsing position in header string

    debugs(4, 6, "Testing Header: '" << hdr << "'");

    while (strHdrAcptLangGetItem(hdr, lang, 256, pos)) {

        /* wildcard uses the configured default language */
        if (lang[0] == '*' && lang[1] == '\0') {
            debugs(4, 6, "Found language '" << lang << "'. Using configured default.");
            return false;
        }

        debugs(4, 6, "Found language '" << lang << "', testing for available template");

        if (tryLoadTemplate(lang)) {
            /* store the language we found for the Content-Language reply header */
            errLanguage = lang;
            break;
        } else if (Config.errorLogMissingLanguages) {
            debugs(4, DBG_IMPORTANT, "WARNING: Error Pages Missing Language: " << lang);
        }
    }
#else
    (void)request;
#endif

    return loaded();
}
