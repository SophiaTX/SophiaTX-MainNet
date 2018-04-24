#include <stdint.h>
#include <steem/utilities/git_revision.hpp>

#define STEEM_GIT_REVISION_SHA "5e3450e93bca4fc3b1a947802dfe0cc33dfc3f3c"
#define STEEM_GIT_REVISION_UNIX_TIMESTAMP 1524501370
#define STEEM_GIT_REVISION_DESCRIPTION "unknown"

namespace steem { namespace utilities {

const char* const git_revision_sha = STEEM_GIT_REVISION_SHA;
const uint32_t git_revision_unix_timestamp = STEEM_GIT_REVISION_UNIX_TIMESTAMP;
const char* const git_revision_description = STEEM_GIT_REVISION_DESCRIPTION;

} } // end namespace steem::utilities
