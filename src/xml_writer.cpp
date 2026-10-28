#include "custom_writer/xml_writer.hpp"

#include <utility>

namespace custom_writer {

XmlWriter::XmlWriter(const std::filesystem::path& output_path,
                     std::size_t indent_width)
    : indent_width_(indent_width) {
  output_.open(output_path,
               std::ios::out | std::ios::trunc | std::ios::binary);
  if (!output_.is_open()) {
    fail("Unable to open output file: " + output_path.u8string());
  }
}

XmlWriter::~XmlWriter() {
  if (output_.is_open()) {
    output_.close();
  }
}

bool XmlWriter::isOpen() const noexcept {
  return output_.is_open();
}

const std::string& XmlWriter::lastError() const noexcept {
  return last_error_;
}

bool XmlWriter::fail(std::string message) {
  if (!failed_) {
    failed_ = true;
    last_error_ = std::move(message);
  }
  return false;
}

bool XmlWriter::checkStream() {
  return output_.good() ? true : fail("Failed while writing XML output");
}

bool XmlWriter::writeDeclarationBytes() {
  output_ << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  at_line_start_ = true;
  return checkStream();
}

bool XmlWriter::writeIndent(std::size_t depth) {
  for (std::size_t level = 0; level < depth; ++level) {
    for (std::size_t index = 0; index < indent_width_; ++index) {
      output_.put(' ');
    }
  }
  return checkStream();
}

bool XmlWriter::beginElement(std::string_view name) {
  if (!at_line_start_) {
    output_.put('\n');
  }
  if (!writeIndent(elements_.size())) {
    return false;
  }
  output_.put('<');
  output_.write(name.data(), static_cast<std::streamsize>(name.size()));
  at_line_start_ = false;
  return checkStream();
}

bool XmlWriter::writeAttributes(
    const std::vector<Attribute>& attributes) {
  for (const Attribute& attribute : attributes) {
    output_.put(' ');
    output_.write(attribute.name.data(),
                  static_cast<std::streamsize>(attribute.name.size()));
    output_ << "=\"";
    if (!writeEscaped(attribute.value, true)) {
      return false;
    }
    output_.put('"');
  }
  return checkStream();
}

bool XmlWriter::finishElementStart() {
  output_.put('>');
  at_line_start_ = false;
  return checkStream();
}

bool XmlWriter::writeEscaped(std::string_view value, bool attribute) {
  for (const char character : value) {
    switch (character) {
      case '&':
        output_ << "&amp;";
        break;
      case '<':
        output_ << "&lt;";
        break;
      case '>':
        output_ << "&gt;";
        break;
      case '"':
        output_ << (attribute ? "&quot;" : "\"");
        break;
      case '\'':
        output_ << (attribute ? "&apos;" : "'");
        break;
      case '\t':
        output_ << (attribute ? "&#9;" : "\t");
        break;
      case '\n':
        output_ << (attribute ? "&#10;" : "\n");
        break;
      case '\r':
        output_ << (attribute ? "&#13;" : "\r");
        break;
      default:
        output_.put(character);
        break;
    }
  }
  return checkStream();
}

bool XmlWriter::writeDeclaration() {
  if (failed_ || !isOpen() || declaration_written_ || root_written_) {
    return fail("XML declaration must be written once before content");
  }
  declaration_written_ = true;
  return writeDeclarationBytes();
}

bool XmlWriter::startElement(
    std::string_view name,
    const std::vector<Attribute>& attributes) {
  if (failed_ || !isOpen() || root_closed_) {
    return fail("Cannot start element in the current document state");
  }

  if (!elements_.empty() &&
      elements_.back().content == ContentKind::kText) {
    return fail("Cannot mix text and child elements in one element");
  }

  if (!beginElement(name) || !writeAttributes(attributes) ||
      !finishElementStart()) {
    return false;
  }

  if (!elements_.empty()) {
    elements_.back().content = ContentKind::kChildren;
  } else {
    root_written_ = true;
  }
  elements_.push_back({std::string(name), ContentKind::kNone});
  return true;
}

bool XmlWriter::writeText(std::string_view text) {
  if (failed_ || !isOpen() || elements_.empty()) {
    return fail("Text must be written inside an element");
  }
  if (elements_.back().content == ContentKind::kChildren) {
    return fail("Cannot mix text and child elements in one element");
  }
  elements_.back().content = ContentKind::kText;
  return writeEscaped(text, false);
}

bool XmlWriter::endElement() {
  if (failed_ || !isOpen() || elements_.empty()) {
    return fail("No open element to close");
  }
  const ElementFrame frame = elements_.back();
  if (frame.content == ContentKind::kChildren && at_line_start_) {
    if (!writeIndent(elements_.size() - 1)) {
      return false;
    }
  }
  output_ << "</" << frame.name << ">\n";
  at_line_start_ = true;
  elements_.pop_back();
  if (elements_.empty()) {
    root_closed_ = true;
  }
  return checkStream();
}

bool XmlWriter::emptyElement(
    std::string_view name,
    const std::vector<Attribute>& attributes) {
  if (failed_ || !isOpen() || root_closed_) {
    return fail("Cannot write empty element in the current document state");
  }
  if (!elements_.empty() &&
      elements_.back().content == ContentKind::kText) {
    return fail("Cannot mix text and child elements in one element");
  }
  if (!beginElement(name) || !writeAttributes(attributes)) {
    return false;
  }
  output_ << "/>\n";
  at_line_start_ = true;
  if (!elements_.empty()) {
    elements_.back().content = ContentKind::kChildren;
  } else {
    root_written_ = true;
    root_closed_ = true;
  }
  return checkStream();
}

bool XmlWriter::close() {
  if (!isOpen()) {
    return fail("Writer is already closed");
  }
  if (failed_) {
    output_.close();
    return false;
  }
  if (!elements_.empty()) {
    fail("Cannot close document with open elements");
    output_.close();
    return false;
  }
  if (!root_written_ || !root_closed_) {
    fail("Document must contain exactly one completed root element");
    output_.close();
    return false;
  }
  output_.flush();
  if (!output_.good()) {
    fail("Failed to flush XML output");
    output_.close();
    return false;
  }
  output_.close();
  return true;
}

}  // namespace custom_writer
