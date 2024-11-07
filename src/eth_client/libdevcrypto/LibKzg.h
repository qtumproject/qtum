#pragma once

#include <libdevcore/Common.h>

namespace dev
{
namespace crypto
{

std::pair<bool, bytes> point_evaluation_execute(bytesConstRef _in);

}
}
