#pragma once

#include <libdevcore/Common.h>

namespace dev
{
namespace crypto
{

std::pair<bool, bytes> p256verify(bytesConstRef _in);

}
}
