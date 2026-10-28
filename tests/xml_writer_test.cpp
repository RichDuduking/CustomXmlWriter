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

void test_attribute_and_text_escaping() {
  const auto path = temporary_path("escaping");
  {
    custom_writer::XmlWriter writer(path);
    CHECK(writer.writeDeclaration());
    CHECK(writer.startElement("root"));
    CHECK(writer.emptyElement(
        "item", {{"value", "A&B <tag> \"quote\" 'apostrophe'\t\n\r"}}));
    CHECK(writer.startElement("text"));
    CHECK(writer.writeText("A&B <C> \"Q\" 'P'\n"));
    CHECK(writer.endElement());
    CHECK(writer.endElement());
    CHECK(writer.close());
  }

  const std::string expected =
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<root>\n"
      "  <item value=\"A&amp;B &lt;tag&gt; &quot;quote&quot; "
      "&apos;apostrophe&apos;&#9;&#10;&#13;\"/>\n"
      "  <text>A&amp;B &lt;C&gt; \"Q\" 'P'\n"
      "</text>\n"
      "</root>\n";
  CHECK(read_file(path) == expected);
  std::filesystem::remove(path);
}

void test_invalid_names_and_attributes() {
  {
    const auto path = temporary_path("invalid-name");
    custom_writer::XmlWriter writer(path);
    CHECK(!writer.startElement("bad name"));
    CHECK(!writer.lastError().empty());
    CHECK(!writer.close());
    std::filesystem::remove(path);
  }
  {
    const auto path = temporary_path("invalid-attribute-name");
    custom_writer::XmlWriter writer(path);
    CHECK(!writer.emptyElement("root", {{"bad name", "1"}}));
    CHECK(!writer.lastError().empty());
    CHECK(!writer.close());
    std::filesystem::remove(path);
  }
  {
    const auto path = temporary_path("duplicate-attribute");
    custom_writer::XmlWriter writer(path);
    CHECK(!writer.emptyElement("root", {{"id", "1"}, {"id", "2"}}));
    CHECK(!writer.lastError().empty());
    CHECK(!writer.close());
    std::filesystem::remove(path);
  }
}

void test_mixed_content_is_rejected() {
  {
    const auto path = temporary_path("text-then-child");
    custom_writer::XmlWriter writer(path);
    CHECK(writer.startElement("root"));
    CHECK(writer.writeText("text"));
    CHECK(!writer.startElement("child"));
    CHECK(!writer.close());
    std::filesystem::remove(path);
  }
  {
    const auto path = temporary_path("child-then-text");
    custom_writer::XmlWriter writer(path);
    CHECK(writer.startElement("root"));
    CHECK(writer.emptyElement("child"));
    CHECK(!writer.writeText("text"));
    CHECK(!writer.close());
    std::filesystem::remove(path);
  }
}

void test_document_structure_errors() {
  {
    const auto path = temporary_path("missing-root");
    custom_writer::XmlWriter writer(path);
    CHECK(writer.writeDeclaration());
    CHECK(!writer.close());
    CHECK(!writer.lastError().empty());
    std::filesystem::remove(path);
  }
  {
    const auto path = temporary_path("duplicate-root");
    custom_writer::XmlWriter writer(path);
    CHECK(writer.emptyElement("first"));
    CHECK(!writer.emptyElement("second"));
    CHECK(!writer.close());
    std::filesystem::remove(path);
  }
  {
    const auto path = temporary_path("late-declaration");
    custom_writer::XmlWriter writer(path);
    CHECK(writer.startElement("root"));
    CHECK(!writer.writeDeclaration());
    CHECK(!writer.close());
    std::filesystem::remove(path);
  }
  {
    const auto path = temporary_path("unclosed");
    custom_writer::XmlWriter writer(path);
    CHECK(writer.startElement("root"));
    CHECK(!writer.close());
    CHECK(!writer.lastError().empty());
    std::filesystem::remove(path);
  }
}

void test_open_failure() {
  const auto directory = temporary_path("open-failure");
  std::filesystem::create_directory(directory);
  custom_writer::XmlWriter writer(directory);
  CHECK(!writer.isOpen());
  CHECK(!writer.lastError().empty());
  CHECK(!writer.close());
  std::filesystem::remove(directory);
}

void test_configurable_indent_without_declaration() {
  const auto path = temporary_path("custom-indent");
  {
    custom_writer::XmlWriter writer(path, 4);
    CHECK(writer.startElement("root"));
    CHECK(writer.emptyElement("child"));
    CHECK(writer.endElement());
    CHECK(writer.close());
  }
  const std::string expected =
      "<root>\n"
      "    <child/>\n"
      "</root>\n";
  CHECK(read_file(path) == expected);
  std::filesystem::remove(path);
}

}  // namespace

int main() {
  test_basic_document();
  test_attribute_and_text_escaping();
  test_invalid_names_and_attributes();
  test_mixed_content_is_rejected();
  test_document_structure_errors();
  test_open_failure();
  test_configurable_indent_without_declaration();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "all tests passed\n";
  return 0;
}
