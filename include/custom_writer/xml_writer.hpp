#pragma once

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace custom_writer {

struct Attribute {
  std::string_view name;
  std::string_view value;
};

class XmlWriter {
 public:
  explicit XmlWriter(const std::filesystem::path& output_path,
                     std::size_t indent_width = 2);
  ~XmlWriter();

  XmlWriter(const XmlWriter&) = delete;
  XmlWriter& operator=(const XmlWriter&) = delete;
  XmlWriter(XmlWriter&&) = delete;
  XmlWriter& operator=(XmlWriter&&) = delete;

  bool writeDeclaration();
  bool startElement(
      std::string_view name,
      const std::vector<Attribute>& attributes = {});
  bool writeText(std::string_view text);
  bool endElement();
  bool emptyElement(
      std::string_view name,
      const std::vector<Attribute>& attributes = {});
  bool close();

  bool isOpen() const noexcept;
  const std::string& lastError() const noexcept;

 private:
  enum class ContentKind {
    kNone,
    kText,
    kChildren,
  };

  struct ElementFrame {
    std::string name;
    ContentKind content = ContentKind::kNone;
  };

  bool writeDeclarationBytes();
  bool beginElement(std::string_view name);
  bool writeAttributes(const std::vector<Attribute>& attributes);
  bool finishElementStart();
  bool writeIndent(std::size_t depth);
  bool writeEscaped(std::string_view value, bool attribute);
  bool checkStream();
  bool fail(std::string message);
  bool validateElement(
      std::string_view name,
      const std::vector<Attribute>& attributes);
  static bool isValidName(std::string_view name);

  std::ofstream output_;
  std::vector<ElementFrame> elements_;
  std::size_t indent_width_ = 2;
  bool declaration_written_ = false;
  bool root_written_ = false;
  bool root_closed_ = false;
  bool at_line_start_ = true;
  bool failed_ = false;
  std::string last_error_;
};

}  // namespace custom_writer
