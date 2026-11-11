#include "xochitl.h"

#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <fstream>
#include <sstream>
#include <unistd.h>

namespace xochitl {

namespace {

std::string read_link(const char *path) {
  char buf[256];
  const ssize_t n = readlink(path, buf, sizeof(buf) - 1);
  if (n <= 0) {
    return {};
  }
  buf[n] = '\0';
  return buf;
}

std::string read_release_version() {
  std::ifstream file{"/usr/share/remarkable/update.conf"};
  std::string line;
  while (std::getline(file, line)) {
    if (line.rfind("REMARKABLE_RELEASE_VERSION=", 0) == 0) {
      std::istringstream ls{line.substr(27)};
      std::string v;
      ls >> v;
      return v;
    }
  }
  return {};
}

// Load bias = (mapping start of file offset 0) - (first PT_LOAD vaddr).
// For ET_EXEC binaries this is 0; for PIE binaries it is the relocation
// delta that must be added to every link time address.
std::uintptr_t read_load_bias(const std::string &path) {
  std::uintptr_t map_start = 0;
  {
    std::ifstream maps{"/proc/self/maps"};
    std::string line;
    while (std::getline(maps, line)) {
      const auto p = line.rfind(' ');
      if (p == std::string::npos || line.substr(p + 1) != path) {
        continue;
      }
      std::uintptr_t start = 0;
      unsigned int start_lo = 0, end_lo = 0, offset = 0;
      char perms[5] = {0};
      if (std::sscanf(line.c_str(), "%x-%x %4s %x", &start_lo, &end_lo,
                      perms, &offset) != 4) {
        continue;
      }
      start = start_lo;
      (void)end_lo;
      if (offset == 0) {
        map_start = start;
        break;
      }
    }
  }
  if (map_start == 0) {
    return 0;
  }

  std::uintptr_t link_base = 0;
  {
    std::ifstream bin{path, std::ios::binary};
    unsigned char ehdr[64];
    bin.read(reinterpret_cast<char *>(ehdr), sizeof(ehdr));
    if (!bin || ehdr[0] != 0x7f || ehdr[1] != 'E' || ehdr[2] != 'L' ||
        ehdr[3] != 'F' || ehdr[4] != 1 || ehdr[5] != 1) {
      return 0;
    }
    const auto phoff = *reinterpret_cast<std::uint32_t *>(ehdr + 28);
    const auto phentsize = *reinterpret_cast<std::uint16_t *>(ehdr + 42);
    const auto phnum = *reinterpret_cast<std::uint16_t *>(ehdr + 44);
    bin.seekg(phoff);
    for (int i = 0; i < phnum; ++i) {
      unsigned char phdr[32];
      bin.read(reinterpret_cast<char *>(phdr), phentsize < 32 ? phentsize : 32);
      if (!bin) {
        return 0;
      }
      if (*reinterpret_cast<std::uint32_t *>(phdr) == 1 /* PT_LOAD */) {
        link_base = *reinterpret_cast<std::uint32_t *>(phdr + 8);
        break;
      }
    }
  }
  return map_start - link_base;
}

// Parses "key addr 0x..." lines for the running version's block from a
// single conf file. Never lowers an already found value.
static void parse_addr_from(const std::string &path,
                            const std::string &version,
                            const std::string &key, std::uintptr_t &value) {
  std::ifstream f{path};
  if (!f) {
    return;
  }
  std::string line;
  bool version_matches = false;
  while (std::getline(f, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    if (line[0] == '!') {
      std::istringstream ls{line.substr(1)};
      std::string v;
      ls >> v;
      version_matches = (v == version);
    } else if (version_matches) {
      std::istringstream ls{line};
      std::string k, type;
      ls >> k >> type;
      if (k == key && type == "addr") {
        std::uintptr_t addr = 0;
        ls.unsetf(std::ios::basefield);
        ls >> addr;
        if (addr != 0) {
          value = addr;
        }
      }
    }
  }
}

// Directory containing this shared library, so the conf can travel with
// the lib regardless of the working directory xochitl was started from.
static std::string lib_dir() {
  Dl_info info;
  if (dladdr(reinterpret_cast<void *>(&lib_dir), &info) && info.dli_fname) {
    const std::string p = info.dli_fname;
    const auto slash = p.rfind('/');
    if (slash != std::string::npos) {
      return p.substr(0, slash);
    }
  }
  return ".";
}

// rm2fb.conf style parsing: only the block for the running version is read.
// Paths mirror rm2fb so both projects can share a single conf file.
std::uintptr_t conf_offset(const std::string &version, const std::string &key) {
  if (const char *env = getenv("RMHACKS_CONF")) {
    const std::string p = env;
    std::uintptr_t v = 0;
    parse_addr_from(p, version, key, v);
    if (v != 0) {
      return v;
    }
  }
  const std::string beside_lib = lib_dir() + "/rmhacks.conf";
  const char *paths[] = {
      "/etc/rmhacks.conf",
      "/opt/etc/rmhacks.conf",
      "/usr/share/rmhacks.conf",
      "/opt/share/rmhacks.conf",
      // fall back to the rm2fb conf which may already carry the keys
      "/etc/rm2fb.conf",
      "/opt/etc/rm2fb.conf",
  };
  std::uintptr_t value = 0;
  for (const char *p : paths) {
    parse_addr_from(p, version, key, value);
    if (value != 0) {
      return value;
    }
  }
  parse_addr_from(beside_lib, version, key, value);
  return value;
}

bool env_flag(const char *name) {
  const char *v = getenv(name);
  return v != nullptr && v[0] != '\0' && v[0] != '0';
}

} // namespace

bool debug_enabled() { return env_flag("RMHACKS_DEBUG"); }

const Runtime &runtime() {
  static Runtime rt = [] {
    Runtime r;
    r.version = read_release_version();
    const std::string exe = read_link("/proc/self/exe");
    r.is_xochitl = (exe == "/usr/bin/xochitl");
    if (!r.is_xochitl || env_flag("RMHACKS_DISABLE")) {
      return r;
    }
    r.load_bias = read_load_bias(exe);

    const std::uintptr_t update = conf_offset(r.version, "update");
    const std::uintptr_t instance = conf_offset(r.version, "getInstance");
    if (update == 0 || instance == 0) {
      std::fprintf(stderr,
                   "rmhacks: no addresses for xochitl %s, disabled\n"
                   "  put rmhacks.conf next to the library, in "
                   "/etc/rmhacks.conf,\n"
                   "  or set RMHACKS_CONF=/path/to/rmhacks.conf\n",
                   r.version.c_str());
      return r;
    }

    r.update_request =
        reinterpret_cast<UpdateRequest>(r.load_bias + update);
    r.get_instance =
        reinterpret_cast<GetInstance>(r.load_bias + instance);
    // Qt symbols come from the hosting process (Qt 6 ABI on device)
    r.region_ctor = reinterpret_cast<QRegionCtor>(
        dlsym(RTLD_DEFAULT, "_ZN7QRegionC1EiiiiNS_10RegionTypeE"));
    r.region_dtor = reinterpret_cast<QRegionDtor>(
        dlsym(RTLD_DEFAULT, "_ZN7QRegionD1Ev"));

    if (r.region_ctor == nullptr) {
      std::fprintf(stderr, "rmhacks: QRegion ctor not found, disabled\n");
      return r;
    }

    r.supported = true;
    if (debug_enabled()) {
      std::fprintf(stderr,
                   "rmhacks: active, xochitl %s, bias 0x%lx, update %p, "
                   "instance %p\n",
                   r.version.c_str(), static_cast<unsigned long>(r.load_bias),
                   reinterpret_cast<void *>(r.update_request),
                   reinterpret_cast<void *>(r.get_instance));
    }
    return r;
  }();
  return rt;
}

} // namespace xochitl
