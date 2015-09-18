/********************************************************************
 * Copyright (c) 2014, Pivotal Inc.
 * All rights reserved.
 *
 * Author: Zhanwei Wang
 ********************************************************************/
#include "KerberosName.h"

#include "Exception.h"
#include "ExceptionInternal.h"

#include <regex.h>
#include <string.h>
#include <vector>

namespace Yarn {
namespace Internal {

static void HandleRegError(int rc, regex_t * comp) {
    std::vector<char> buffer;
    size_t size = regerror(rc, comp, NULL, 0);
    buffer.resize(size + 1);
    regerror(rc, comp, &buffer[0], buffer.size());
    THROW(YarnIOException, "KerberosName: Failed to parse Kerberos principal.");
}

KerberosName::KerberosName() {
}

KerberosName::KerberosName(const std::string & principal) {
    parse(principal);
}

void KerberosName::parse(const std::string & principal) {
    int rc;
    static const char * pattern = "([^/@]*)(/([^/@]*))?@([^/@]*)";
    regex_t comp;
    regmatch_t pmatch[5];

    if (principal.empty()) {
        return;
    }

    memset(&comp, 0, sizeof(regex_t));
    rc = regcomp(&comp, pattern, REG_EXTENDED);

    if (rc) {
        HandleRegError(rc, &comp);
    }

    try {
        memset(pmatch, 0, sizeof(pmatch));
        rc = regexec(&comp, principal.c_str(),
                     sizeof(pmatch) / sizeof(pmatch[1]), pmatch, 0);

        if (rc && rc != REG_NOMATCH) {
            HandleRegError(rc, &comp);
        }

        if (rc == REG_NOMATCH) {
            if (principal.find('@') != principal.npos) {
                THROW(YarnIOException,
                      "KerberosName: Malformed Kerberos name: %s",
                      principal.c_str());
            } else {
                name = principal;
            }
        } else {
            if (pmatch[1].rm_so != -1) {
                name = principal.substr(pmatch[1].rm_so,
                                        pmatch[1].rm_eo - pmatch[1].rm_so);
            }

            if (pmatch[3].rm_so != -1) {
                host = principal.substr(pmatch[3].rm_so,
                                        pmatch[3].rm_eo - pmatch[3].rm_so);
            }

            if (pmatch[4].rm_so != -1) {
                realm = principal.substr(pmatch[4].rm_so,
                                         pmatch[4].rm_eo - pmatch[4].rm_so);
            }
        }
    } catch (...) {
        regfree(&comp);
        throw;
    }

    regfree(&comp);
}

size_t KerberosName::hash_value() const {
    size_t values[] = { StringHasher(name), StringHasher(host), StringHasher(
                            realm)
                      };
    return CombineHasher(values, sizeof(values) / sizeof(values[0]));
}

}
}