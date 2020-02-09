#ifndef RUNNER_SRC_INCLUDE_KOURT_RUNNER_ELF_H_
#define RUNNER_SRC_INCLUDE_KOURT_RUNNER_ELF_H_

#include <elf.h>
#include <cstdint>

#include <cstddef>
#include <string>
#include <memory>

#include <kourt/runner/defer.h>

struct ElfSection {
  std::string_view section_name;
  uint32_t section_type;
  uint64_t section_flags;
  std::uintptr_t virtual_address;
  const char *section_content;
  size_t section_size;

  uint64_t address_alignment;
  uint64_t entry_size;
};

struct ElfSymbol {
  enum class Bind {
    kLocal, kGlobal, kWeak, kUnknown
  };
  enum class Type {
    kNoType, kObject, kFunc, kSection, kFile, kUnknown
  };
  enum class Visibility {
    kDefault, kInternal, kHidden, kProtected, kUnknown
  };

  std::string_view symbol_name;
  Bind symbol_bind;
  Type symbol_type;
  Visibility symbol_visibility;
  const ElfSection *section;
  std::uintptr_t symbol_value;
  size_t symbol_size;
};

class ElfFile {
 public:
  ElfFile(const char *elf, size_t size) :
      elf_(elf),
      size_(size) {
    // nop
  }

  virtual ~ElfFile() {
    munmap(const_cast<char *>(elf_), size_);
  }

  [[nodiscard]] virtual const std::vector<ElfSection> &Sections() const = 0;

 protected:
  const char *elf_;
  const size_t size_;
};

std::unique_ptr<ElfFile> ParseElfFile(const std::string &file_name);

#endif //RUNNER_SRC_INCLUDE_KOURT_RUNNER_ELF_H_
