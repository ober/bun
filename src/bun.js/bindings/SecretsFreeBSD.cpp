#include "root.h"

#if defined(__FreeBSD__)

#include "Secrets.h"
#include <wtf/text/WTFString.h>

namespace Bun {
namespace Secrets {

using namespace WTF;

Error setPassword(const CString& service, const CString& name, CString&& password, bool allowUnrestrictedAccess)
{
    Error err;
    err.type = ErrorType::PlatformError;
    err.message = WTF::String::fromUTF8("Keychain is not supported on FreeBSD");
    return err;
}

std::optional<WTF::Vector<uint8_t>> getPassword(const CString& service, const CString& name, Error& err)
{
    err.type = ErrorType::PlatformError;
    err.message = WTF::String::fromUTF8("Keychain is not supported on FreeBSD");
    return std::nullopt;
}

bool deletePassword(const CString& service, const CString& name, Error& err)
{
    err.type = ErrorType::PlatformError;
    err.message = WTF::String::fromUTF8("Keychain is not supported on FreeBSD");
    return false;
}

} // namespace Secrets
} // namespace Bun

#endif // defined(__FreeBSD__)
