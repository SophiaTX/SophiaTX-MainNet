#include <stdint.h>
#include <steem/utilities/git_revision.hpp>

#define STEEM_GIT_REVISION_SHA "9af0627f132b82882dca8084fe9731bf023b9ff6"
#define STEEM_GIT_REVISION_UNIX_TIMESTAMP 1523545597
#define STEEM_GIT_REVISION_DESCRIPTION "unknown"

namespace steem { namespace utilities {

const char* const git_revision_sha = STEEM_GIT_REVISION_SHA;
const uint32_t git_revision_unix_timestamp = STEEM_GIT_REVISION_UNIX_TIMESTAMP;
const char* const git_revision_description = STEEM_GIT_REVISION_DESCRIPTION;

} } // end namespace steem::utilities
