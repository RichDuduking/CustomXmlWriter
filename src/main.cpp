#include "custom_writer/xml_writer.hpp"

#include <filesystem>
#include <iostream>

namespace {

bool write_demo(custom_writer::XmlWriter& writer) {
  return writer.writeDeclaration() &&
         writer.startElement("catalog") &&
         writer.startElement("book", {{"id", "42"}}) &&
         writer.startElement("title") &&
         writer.writeText("Example Book") &&
         writer.endElement() &&
         writer.endElement() &&
         writer.emptyElement("notice") &&
         writer.endElement() &&
         writer.close();
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: CustomWriter <output.xml>\n";
    return 2;
  }

  const std::filesystem::path output_path(argv[1]);
  custom_writer::XmlWriter writer(output_path);
  if (!writer.isOpen()) {
    std::cerr << writer.lastError() << '\n';
    return 1;
  }
  if (!write_demo(writer)) {
    std::cerr << writer.lastError() << '\n';
    return 1;
  }

  std::cout << "Wrote XML to " << output_path.string() << '\n';
  return 0;
}
