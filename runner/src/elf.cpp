#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <elf.h>

#include <algorithm>
#include <stdexcept>
#include <functional>

#include <kourt/runner/assert.h>
#include <kourt/runner/elf.h>

class Elf64File : public ElfFile {
 public:
  Elf64File(const char *elf, size_t size) : ElfFile(elf, size) {
    ASSERT(
        ELFMAG0 == elf[EI_MAG0] && ELFMAG1 == elf[EI_MAG1] && ELFMAG2 == elf[EI_MAG2] && ELFMAG3 == elf[EI_MAG3],
        "invalid ELF magic"
    )
    ASSERT(ELFDATA2LSB == elf_[EI_DATA], "Only little-endian ELF files are supported");
    ASSERT(ET_EXEC == Header()->e_type, "Only executable ELF files are supported");

    LoadSections();
    LoadSymbols();
  }

  ~Elf64File() override {
    ElfFile::~ElfFile();
  }

  [[nodiscard]] const std::vector<ElfSection> &Sections() const override {
    return sections_;
  }
 private:
  void LoadSections() {
    auto section_headers = SectionHeaders();

    // navigate to
    auto section_names_table_index = section_headers[Header()->e_shstrndx].sh_offset;
    if (section_names_table_index == SHN_XINDEX) {
      section_names_table_index = section_headers[0].sh_link;
    }
    auto section_names_table = elf_ + section_names_table_index;
    for (size_t i = 0; i < SectionsCount(); i++) {
      auto raw_section = section_headers + i;
      sections_.push_back(ElfSection {
          .section_name = section_names_table + raw_section->sh_name,
          .section_type = raw_section->sh_type,
          .section_flags = raw_section->sh_flags,
          .virtual_address = raw_section->sh_addr,
          .section_content = elf_ + raw_section->sh_offset,
          .section_size = raw_section->sh_size,
          .address_alignment = raw_section->sh_addralign,
          .entry_size = raw_section->sh_entsize
      });
    }
  }
  void LoadSymbols() {
    ASSERT(!sections_.empty(), "Sections should be loaded before symbols");

    auto symbols_table_header = SectionOfName(".symtab");
    auto raw_symbols = (Elf64_Sym *) symbols_table_header.section_content;
    auto symbol_names_table = SectionOfName(".strtab").section_content;
    for (int i = 0; i < symbols_table_header.section_size / symbols_table_header.entry_size; i++) {
      auto raw_symbol = raw_symbols[i];
      symbols_.push_back(ElfSymbol{
          .symbol_name = symbol_names_table + raw_symbol.st_name,
          .symbol_bind = DetermineSymbolBind(raw_symbol.st_info),
          .symbol_type = DetermineSymbolType(raw_symbol.st_info),
          .symbol_visibility = DetermineSymbolVisibility(raw_symbol.st_other),
          .section = &Sections()[raw_symbol.st_shndx],
          .symbol_value = raw_symbol.st_value,
          .symbol_size = raw_symbol.st_size
      })
    }
  }

  [[nodiscard]] inline const Elf64_Ehdr *Header() const {
    return reinterpret_cast<const Elf64_Ehdr *>(elf_);
  }
  [[nodiscard]] inline const Elf64_Shdr *SectionHeaders() const {
    auto segments_offset = Header()->e_shoff;
    ASSERT(0 != segments_offset, "ELF file does not have segments");
    return reinterpret_cast<const Elf64_Shdr *>(elf_ + segments_offset);
  }
  [[nodiscard]] inline size_t SectionsCount() const {
    return Header()->e_shnum;
  }
  [[nodiscard]] inline const ElfSection& SectionOfName(std::string_view section_name) {
    const ElfSection *found_section;
    bool found = false;
    for (const auto &section : sections_) {
      if (section.section_name == section_name) {
        if (!found) {
          found_section = &section;
          found = true;
        } else {
          throw std::runtime_error("There are two different sections of name " + std::string(section_name));
        }
      }
    }
    throw std::runtime_error("There is no section of name " + std::string(section_name));
  }

  static ElfSymbol::Bind DetermineSymbolBind(unsigned char info) {
    switch (ELF64_ST_BIND(info)) {
      case STB_LOCAL: return ElfSymbol::Bind::kLocal;
      case STB_GLOBAL: return ElfSymbol::Bind::kGlobal;
      case STB_WEAK: return ElfSymbol::Bind::kWeak;
      default: return ElfSymbol::Bind::kUnknown;
    }
  }
  static ElfSymbol::Type DetermineSymbolType(unsigned char info) {
    switch (ELF64_ST_TYPE(info)) {
      case STT_NOTYPE: return ElfSymbol::Type::kNoType;
      case STT_OBJECT: return ElfSymbol::Type::kObject;
      case STT_FUNC: return ElfSymbol::Type::kFunc;
      case STT_SECTION: return ElfSymbol::Type::kSection;
      case STT_FILE: return ElfSymbol::Type::kFile;
      default: return ElfSymbol::Type::kUnknown;
    }
  }
  static ElfSymbol::Visibility DetermineSymbolVisibility(unsigned char other) {
    switch (ELF64_ST_VISIBILITY(other)) {
      case STV_DEFAULT: return ElfSymbol::Visibility::kDefault;
      case STV_INTERNAL: return ElfSymbol::Visibility::kInternal;
      case STV_HIDDEN: return ElfSymbol::Visibility::kHidden;
      case STV_PROTECTED: return ElfSymbol::Visibility::kProtected;
      default: return ElfSymbol::Visibility::kUnknown;
    }
  }

  std::vector<ElfSection> sections_;
  std::vector<ElfSymbol> symbols_;
};

std::unique_ptr<ElfFile> ParseElfFile(const std::string &file_name) {
  // open executable file
  int fd = open(file_name.c_str(), O_RDONLY);
  if (fd == -1) {
    int error_code = errno;
    throw std::runtime_error("Failed to open file '" + file_name + "': " + strerror(error_code));
  }
  auto defer_close_fd = DeferCloseFileDescriptor(fd);

  // determine file size
  struct stat file_stat{};
  if (-1 == fstat(fd, &file_stat)) {
    int error_code = errno;
    throw std::runtime_error("Failed to stat file '" + file_name + "': " + strerror(error_code));
  }
  size_t file_size = file_stat.st_size;

  // map file content to memory
  char *elf = (char *) mmap(nullptr, file_stat.st_size, PROT_READ, MAP_ANONYMOUS, fd, file_size);
  if (MAP_FAILED == elf) {
    int error_code = errno;
    throw std::runtime_error("Failed to mmap file '" + file_name + "': " + strerror(error_code));
  }

  // TODO: support Elf32
  ASSERT(ELFCLASS64 == elf[EI_CLASS], "Only 64-bit ELF format is supported");
  return std::unique_ptr<ElfFile>(new Elf64File(elf, file_size));
}
