#include "custom_writer/xml_writer.hpp"

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

namespace {

int failures = 0;

void check(bool condition, std::string_view expression, int line) {
  if (condition) {
    return;
  }
  ++failures;
  std::cerr << "line " << line << ": check failed: " << expression << '\n';
}

#define CHECK(expression) check((expression), #expression, __LINE__)

std::filesystem::path temporary_path(std::string_view name) {
  static std::size_t counter = 0;
  const auto stamp =
      std::chrono::steady_clock::now().time_since_epoch().count();
  std::filesystem::path path = std::filesystem::temp_directory_path();
  path /= "customwriter-" + std::string(name) + "-" +
          std::to_string(stamp) + "-" + std::to_string(counter++) + ".xml";
  return path;
}

std::string read_file(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::in | std::ios::binary);
  return std::string(std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>());
}

void test_basic_document() {
  const auto path = temporary_path("basic");

  {
    custom_writer::XmlWriter writer(path);
    CHECK(writer.isOpen());
    CHECK(writer.writeDeclaration());
    CHECK(writer.startElement("catalog"));
    CHECK(writer.startElement("book", {{"id", "42"}}));
    CHECK(writer.startElement("title"));
    CHECK(writer.writeText("Example Book"));
    CHECK(writer.endElement());
    CHECK(writer.endElement());
    CHECK(writer.emptyElement("notice"));
    CHECK(writer.endElement());
    CHECK(writer.close());
  }

  const std::string expected =
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<catalog>\n"
      "  <book id=\"42\">\n"
      "    <title>Example Book</title>\n"
      "  </book>\n"
      "  <notice/>\n"
      "</catalog>\n";
  const std::string actual = read_file(path);
  if (actual != expected) {
    std::cerr << "expected:\n" << expected << "actual:\n" << actual;
  }
  CHECK(actual == expected);
  std::filesystem::remove(path);
}

}  // namespace

int main() {
  test_basic_document();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "all tests passed\n";
  return 0;
}
