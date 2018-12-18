#pragma once
#include <string>

namespace fc {
std::string base64m_encode(unsigned char const* bytes_to_encode, unsigned int in_len);
inline std::string base64m_encode(char const* bytes_to_encode, unsigned int in_len) { return base64m_encode( (unsigned char const*)bytes_to_encode, in_len); }
std::string base64m_encode( const std::string& enc );
std::string base64m_decode( const std::string& encoded_string);
std::string normalize_to_base64m(const std::string& base64_string);
std::string normalize_to_base64(const std::string& base64m_string);
}  // namespace fc
