#include <stdint.h>
#include <steem/utilities/git_revision.hpp>

#define STEEM_GIT_REVISION_SHA "148f4e29e7017b93ab93130449d0ee7c34805e36"
#define STEEM_GIT_REVISION_UNIX_TIMESTAMP 1525258127
#define STEEM_GIT_REVISION_DESCRIPTION "unknown"

namespace steem { namespace utilities {

const char* const git_revision_sha = STEEM_GIT_REVISION_SHA;
const uint32_t git_revision_unix_timestamp = STEEM_GIT_REVISION_UNIX_TIMESTAMP;
const char* const git_revision_description = STEEM_GIT_REVISION_DESCRIPTION;

} } // end namespace steem::utilities
