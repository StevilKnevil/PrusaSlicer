#include <set>
#include <mutex>
#include <memory>

#include "SL1.hpp"
#include "SL1_SVG.hpp"
#include "AnycubicSLA.hpp"

#include "SLAArchiveFormatRegistry.hpp"

namespace Slic3r {

static std::mutex arch_mtx;

class Registry {
    static std::unique_ptr<Registry> registry;

    std::set<ArchiveEntry> entries;
public:

    Registry ()
    {
        entries = {
            {
                "SL1",                      // id
                L("SL1 archive format"),    // description
                "sl1",                      // main extension
                {"sl1s", "zip"},            // extension aliases

                // Writer factory
                [] (const auto &cfg) { return std::make_unique<SL1Archive>(cfg); },

                // Reader factory
                [] (const std::string &fname, SLAImportQuality quality, const ProgrFn &progr) {
                    return std::make_unique<SL1Reader>(fname, quality, progr);
                }
            },
            {
                "SL1SVG",
                L("SL1SVG archive files"),
                "sl1_svg",
                {},
                [] (const auto &cfg) { return std::make_unique<SL1_SVGArchive>(cfg); },
                [] (const std::string &fname, SLAImportQuality quality, const ProgrFn &progr) {
                    return std::make_unique<SL1_SVGReader>(fname, quality, progr);
                }
            },
            {
                "SL2",
                "",
                "sl1_svg",
                {},
                [] (const auto &cfg) { return std::make_unique<SL1_SVGArchive>(cfg); },
                nullptr
            },
            anycubic_sla_format("pwmo", "Photon Mono"),
            anycubic_sla_format("pwmx", "Photon Mono X"),
            anycubic_sla_format("pwms", "Photon Mono SE"),

            /**
                // Supports only ANYCUBIC_SLA_VERSION_1
                anycubic_sla_format_versioned("pws", "Photon / Photon S", ANYCUBIC_SLA_VERSION_1),
                anycubic_sla_format_versioned("pw0", "Photon Zero", ANYCUBIC_SLA_VERSION_1),
                anycubic_sla_format_versioned("pwx", "Photon X", ANYCUBIC_SLA_VERSION_1),

                // Supports ANYCUBIC_SLA_VERSION_1 and ANYCUBIC_SLA_VERSION_515
                anycubic_sla_format_versioned("pwmo", "Photon Mono", ANYCUBIC_SLA_VERSION_1),
                anycubic_sla_format_versioned("pwms", "Photon Mono SE", ANYCUBIC_SLA_VERSION_1),
                anycubic_sla_format_versioned("dlp", "Photon Ultra", ANYCUBIC_SLA_VERSION_1),
                anycubic_sla_format_versioned("pwmx", "Photon Mono X", ANYCUBIC_SLA_VERSION_1),
                anycubic_sla_format_versioned("pmsq", "Photon Mono SQ", ANYCUBIC_SLA_VERSION_1),

                // Supports ANYCUBIC_SLA_VERSION_515 and ANYCUBIC_SLA_VERSION_516
                anycubic_sla_format_versioned("pwma", "Photon Mono 4K", ANYCUBIC_SLA_VERSION_515),
                anycubic_sla_format_versioned("pm3",  "Photon M3", ANYCUBIC_SLA_VERSION_515),
                anycubic_sla_format_versioned("pm3m", "Photon M3 Max", ANYCUBIC_SLA_VERSION_515),

                // Supports NYCUBIC_SLA_VERSION_515 and ANYCUBIC_SLA_VERSION_516 and ANYCUBIC_SLA_VERSION_517
                anycubic_sla_format_versioned("pwmb", "Photon Mono X 6K / Photon M3 Plus", ANYCUBIC_SLA_VERSION_515),
                anycubic_sla_format_versioned("dl2p", "Photon Photon D2", ANYCUBIC_SLA_VERSION_515),
                anycubic_sla_format_versioned("pmx2", "Photon Mono X2", ANYCUBIC_SLA_VERSION_515),
                anycubic_sla_format_versioned("pm3r", "Photon M3 Premium", ANYCUBIC_SLA_VERSION_515),
            */
        };
    }

    static Registry& get_instance()
    {
        if (!registry)
            registry = std::make_unique<Registry>();

        return *registry;
    }

    static std::set<ArchiveEntry>& get()
    {
        return get_instance().entries;
    }

    std::set<ArchiveEntry>& get_entries() { return entries; }
};

std::unique_ptr<Registry> Registry::registry = nullptr;

std::set<ArchiveEntry> registered_sla_archives()
{
    std::lock_guard lk{arch_mtx};

    return Registry::get();
}

std::vector<std::string> get_extensions(const ArchiveEntry &entry)
{
    auto ret = reserve_vector<std::string>(entry.ext_aliases.size() + 1);

    ret.emplace_back(entry.ext);
    for (const char *alias : entry.ext_aliases)
        ret.emplace_back(alias);

    return ret;
}

ArchiveWriterFactory get_writer_factory(const char *formatid)
{
    std::lock_guard lk{arch_mtx};

    ArchiveWriterFactory ret;
    auto entry = Registry::get().find(ArchiveEntry{formatid});
    if (entry != Registry::get().end())
        ret = entry->wrfactoryfn;

    return ret;
}

ArchiveReaderFactory get_reader_factory(const char *formatid)
{
    std::lock_guard lk{arch_mtx};

    ArchiveReaderFactory ret;
    auto entry = Registry::get().find(ArchiveEntry{formatid});
    if (entry != Registry::get().end())
        ret = entry->rdfactoryfn;

    return ret;
}

} // namespace Slic3r::sla
