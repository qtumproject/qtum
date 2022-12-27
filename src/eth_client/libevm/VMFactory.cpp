// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "VMFactory.h"
#include "EVMC.h"
#include <evmc/loader.h>
#include <evmone/evmone.h>

namespace po = boost::program_options;

namespace dev
{
namespace eth
{
namespace
{

DEV_SIMPLE_EXCEPTION(VMKindNotSupported);

auto g_kind = VMKind::Evmone;

/// The list of EVMC options stored as pairs of (name, value).
std::vector<std::pair<std::string, std::string>> s_evmcOptions;

/// A helper type to build the tabled of VM implementations.
///
/// More readable than std::tuple.
/// Fields are not initialized to allow usage of construction with initializer lists {}.
struct VMKindTableEntry
{
    VMKind kind;
    const char* name;
};

/// The table of available VM implementations.
///
/// We don't use a map to avoid complex dynamic initialization. This list will never be long,
/// so linear search only to parse command line arguments is not a problem.
VMKindTableEntry vmKindsTable[] = {
    {VMKind::Evmone, "evmone"},
};

void setVMKind(const std::string& _name)
{
    for (auto& entry : vmKindsTable)
    {
        // Try to find a match in the table of VMs.
        if (_name == entry.name)
        {
            g_kind = entry.kind;
            return;
        }
    }

    BOOST_THROW_EXCEPTION(VMKindNotSupported() << errinfo_comment("VM " + _name + "not supported"));
}
}  // namespace

namespace
{
/// The name of the program option --evmc. The boost will trim the tailing
/// space and we can reuse this variable in exception message.
const char c_evmcPrefix[] = "evmc ";

/// The additional parser for EVMC options. The options should look like
/// `--evmc name=value` or `--evmc=name=value`. The boost pass the strings
/// of `name=value` here. This function splits the name and value or reports
/// the syntax error if the `=` character is missing.
void parseEvmcOptions(const std::vector<std::string>& _opts)
{
    for (auto& s : _opts)
    {
        auto separatorPos = s.find('=');
        if (separatorPos == s.npos)
            throw po::invalid_syntax{po::invalid_syntax::missing_parameter, c_evmcPrefix + s};
        auto name = s.substr(0, separatorPos);
        auto value = s.substr(separatorPos + 1);
        s_evmcOptions.emplace_back(std::move(name), std::move(value));
    }
}
}  // namespace

po::options_description vmProgramOptions(unsigned _lineLength)
{
    // It must be a static object because boost expects const char*.
    static const std::string description = [] {
        std::string names;
        for (auto& entry : vmKindsTable)
        {
            if (!names.empty())
                names += ", ";
            names += entry.name;
        }

        return "Select VM implementation. Available options are: " + names + ".";
    }();

    po::options_description opts("VM OPTIONS", _lineLength);
    auto add = opts.add_options();

    add("vm",
        po::value<std::string>()
            ->value_name("<name>|<path>")
            ->default_value("evmone")
            ->notifier(setVMKind),
        description.data());

    add(c_evmcPrefix,
        po::value<std::vector<std::string>>()
            ->multitoken()
            ->value_name("<option>=<value>")
            ->notifier(parseEvmcOptions),
        "EVMC option\n");

    return opts;
}


VMPtr VMFactory::create()
{
    return create(g_kind);
}

VMPtr VMFactory::create(VMKind _kind)
{
    static const auto default_delete = [](VMFace * _vm) noexcept { delete _vm; };

    switch (_kind)
    {
    case VMKind::Evmone:
        return {new EVMC{evmc_create_evmone(), s_evmcOptions}, default_delete};
    default:
        return {new EVMC{evmc_create_evmone(), s_evmcOptions}, default_delete};
    }
}
}  // namespace eth
}  // namespace dev
