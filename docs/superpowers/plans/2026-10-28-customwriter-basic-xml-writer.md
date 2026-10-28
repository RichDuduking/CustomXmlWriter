# CustomWriter 基础 XML 写出器实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 实现一个使用 `std::ofstream` 的 C++17 通用 XML 写出库，并提供可运行的命令行示例、文档和自动测试。

**Architecture:** `XmlWriter` 以流式状态机维护元素栈和元素内容类型，直接写出带缩进的 XML。CMake 将核心实现编译为 `custom_writer` 库，测试程序和 `CustomWriter` 命令行程序分别链接它。

**Tech Stack:** C++17、CMake 3.16+、CTest、`std::ofstream`、`std::ifstream`、`std::filesystem`，无第三方依赖。

## Global Constraints

- 使用 C++17，不引入第三方库。
- XML 文件使用 `std::ofstream`，以 `std::ios::out | std::ios::trunc | std::ios::binary` 打开。
- 输出 UTF-8 字节和 LF 换行，不写 BOM。
- XML 声明固定为 `<?xml version="1.0" encoding="UTF-8"?>`。
- 默认缩进两个空格；缩进宽度由构造函数参数决定。
- 文档必须包含且只能包含一个根元素。
- 元素名和属性名不能为空；ASCII 名称语法必须合法；非 ASCII 字节原样接受。
- 同一元素不能同时包含内联文本和子元素。
- 文本转义 `&`、`<`、`>`。
- 属性值转义 `&`、`<`、`>`、`"`、`'`，并将制表符、换行、回车写成 `&#9;`、`&#10;`、`&#13;`。
- 所有会修改 XML 输出的接口返回 `bool`；第一次失败后锁存错误，后续输出调用不写更多字节。
- `close()` 在失败状态下仍关闭文件，但返回 `false`。
- 命令行用法固定为 `CustomWriter <output.xml>`。
- 本机缺少 PATH 中的编译器时，验证使用 `E:\Qt6.8.3\Tools\mingw1310_64\bin` 中的 GCC 和 MinGW Make。

---

### Task 1: 核心 API 与基础流式写出

**Files:**
- Create: `CMakeLists.txt`
- Create: `.gitignore`
- Create: `include/custom_writer/xml_writer.hpp`
- Create: `src/xml_writer.cpp`
- Test: `tests/xml_writer_test.cpp`

**Interfaces:**
- Consumes: C++17 标准库。
- Produces: `custom_writer::Attribute`、`custom_writer::XmlWriter`，以及 CMake 目标 `custom_writer` 和 `xml_writer_tests`。

- [ ] **Step 1: 写基础文档的失败测试和构建入口**

Create `tests/xml_writer_test.cpp`:

```cpp
#include "custom_writer/xml_writer.hpp"

#include <chrono>
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
    CHECK(writer.emptyElement("notice"));
    CHECK(writer.endElement());
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
  CHECK(read_file(path) == expected);
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
```

Create `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)
project(CustomWriter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(custom_writer
  src/xml_writer.cpp
)
target_include_directories(custom_writer
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
  target_compile_options(custom_writer PRIVATE /W4 /permissive- /utf-8)
else()
  target_compile_options(custom_writer PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()

add_executable(xml_writer_tests
  tests/xml_writer_test.cpp
)
target_link_libraries(xml_writer_tests PRIVATE custom_writer)
add_test(NAME xml_writer_tests COMMAND xml_writer_tests)
```

Create `.gitignore`:

```gitignore
/build/
/.vs/
/.vscode/
*.user
```

- [ ] **Step 2: 运行测试，确认当前失败**

Run:

```powershell
$env:PATH = "E:\Qt6.8.3\Tools\mingw1310_64\bin;" + $env:PATH
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: CMake 配置或编译失败，因为 `src/xml_writer.cpp` 和 `include/custom_writer/xml_writer.hpp` 尚不存在。

- [ ] **Step 3: 添加公共头文件和基础实现**

Create `include/custom_writer/xml_writer.hpp`:

```cpp
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
```

Create `src/xml_writer.cpp` with these exact core behaviors:

```cpp
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
```

- [ ] **Step 4: 运行基础测试，确认通过**

Run:

```powershell
$env:PATH = "E:\Qt6.8.3\Tools\mingw1310_64\bin;" + $env:PATH
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: `xml_writer_tests` 通过。

- [ ] **Step 5: 提交基础流式写出器**

```powershell
git add CMakeLists.txt .gitignore include/custom_writer/xml_writer.hpp src/xml_writer.cpp tests/xml_writer_test.cpp
git commit -m "feat: add basic XML writer"
```

---

### Task 2: 名称校验、文档约束与错误锁存

**Files:**
- Modify: `include/custom_writer/xml_writer.hpp`
- Modify: `src/xml_writer.cpp`
- Test: `tests/xml_writer_test.cpp`

**Interfaces:**
- Consumes: Task 1 的 `XmlWriter` 公共接口和测试辅助函数。
- Produces: 对非法名称、重复属性、混合内容、重复根元素、缺少根元素、未闭合元素和文件打开失败的确定性错误行为。

- [ ] **Step 1: 添加失败测试**

Extend `tests/xml_writer_test.cpp` with these functions and declarations:

```cpp
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
```

Update `main()`:

```cpp
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
```

- [ ] **Step 2: 运行测试，确认失败**

Run:

```powershell
$env:PATH = "E:\Qt6.8.3\Tools\mingw1310_64\bin;" + $env:PATH
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: 新增测试失败，因为非法名称和重复属性尚未校验。

- [ ] **Step 3: 在开始标签写出前完成全部校验**

Add these private declarations to `XmlWriter`:

```cpp
  bool validateElement(
      std::string_view name,
      const std::vector<Attribute>& attributes,
      bool closesImmediately);
  static bool isValidName(std::string_view name);
```

Add these definitions to `src/xml_writer.cpp`:

```cpp
bool XmlWriter::isValidName(std::string_view name) {
  if (name.empty()) {
    return false;
  }

  for (std::size_t index = 0; index < name.size(); ++index) {
    const unsigned char value =
        static_cast<unsigned char>(name[index]);
    if (value >= 0x80) {
      continue;
    }
    const bool alpha =
        (value >= 'A' && value <= 'Z') ||
        (value >= 'a' && value <= 'z');
    const bool digit = value >= '0' && value <= '9';
    if (index == 0 && (alpha || value == '_' || value == ':')) {
      continue;
    }
    if (index > 0 &&
        (alpha || digit || value == '_' || value == '-' ||
         value == '.' || value == ':')) {
      continue;
    }
    return false;
  }
  return true;
}

bool XmlWriter::validateElement(
    std::string_view name,
    const std::vector<Attribute>& attributes,
    bool closesImmediately) {
  if (!isValidName(name)) {
    return fail("Invalid XML element name");
  }

  for (std::size_t index = 0; index < attributes.size(); ++index) {
    if (!isValidName(attributes[index].name)) {
      return fail("Invalid XML attribute name");
    }
    for (std::size_t previous = 0; previous < index; ++previous) {
      if (attributes[previous].name == attributes[index].name) {
        return fail("Duplicate XML attribute name");
      }
    }
  }

  if (closesImmediately && elements_.empty() && root_closed_) {
    return fail("Document already has a root element");
  }
  return true;
}
```

In both `startElement()` and `emptyElement()`, call
`validateElement(name, attributes, false)` before `beginElement()`.

Add duplicate-root and declaration-order checks:

```cpp
bool XmlWriter::writeDeclaration() {
  if (failed_ || !isOpen()) {
    return false;
  }
  if (declaration_written_) {
    return fail("XML declaration can only be written once");
  }
  if (root_written_ || !elements_.empty()) {
    return fail("XML declaration must precede document content");
  }
  declaration_written_ = true;
  return writeDeclarationBytes();
}

bool XmlWriter::startElement(
    std::string_view name,
    const std::vector<Attribute>& attributes) {
  if (failed_ || !isOpen()) {
    return false;
  }
  if (!validateElement(name, attributes, false)) {
    return false;
  }
  if (elements_.empty() && root_closed_) {
    return fail("Document already has a root element");
  }
  if (!elements_.empty() &&
      elements_.back().content == ContentKind::kText) {
    return fail("Cannot mix text and child elements in one element");
  }
  if (!beginElement(name) || !writeAttributes(attributes) ||
      !finishElementStart()) {
    return false;
  }
  if (elements_.empty()) {
    root_written_ = true;
  } else {
    elements_.back().content = ContentKind::kChildren;
  }
  elements_.push_back({std::string(name), ContentKind::kNone});
  return true;
}

bool XmlWriter::emptyElement(
    std::string_view name,
    const std::vector<Attribute>& attributes) {
  if (failed_ || !isOpen()) {
    return false;
  }
  if (!validateElement(name, attributes, true)) {
    return false;
  }
  if (elements_.empty() && root_closed_) {
    return fail("Document already has a root element");
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
  if (elements_.empty()) {
    root_written_ = true;
    root_closed_ = true;
  } else {
    elements_.back().content = ContentKind::kChildren;
  }
  return checkStream();
}
```

Replace the remaining `if (failed_ || !isOpen() || ...)` guards in mutating
methods with:

```cpp
  if (failed_ || !isOpen()) {
    return false;
  }
```

Keep the method-specific state errors, such as `writeText()` outside an
element and `endElement()` without an open element, but return them through
`fail("...")`.

- [ ] **Step 4: 运行全部测试**

Run:

```powershell
$env:PATH = "E:\Qt6.8.3\Tools\mingw1310_64\bin;" + $env:PATH
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: `xml_writer_tests` 通过。

- [ ] **Step 5: 提交校验和错误处理**

```powershell
git add include/custom_writer/xml_writer.hpp src/xml_writer.cpp tests/xml_writer_test.cpp
git commit -m "feat: validate XML writer state"
```

---

### Task 3: 命令行程序、集成测试和中文文档

**Files:**
- Create: `src/main.cpp`
- Create: `tests/cli_smoke.cmake`
- Create: `README.md`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `custom_writer::XmlWriter` 的完整公共接口。
- Produces: `CustomWriter` 可执行文件、CTest 集成测试 `customwriter_cli` 和项目使用文档。

- [ ] **Step 1: 添加命令行集成测试，确认失败**

Create `tests/cli_smoke.cmake`:

```cmake
if(NOT EXISTS "${WRITER_EXE}")
  message(FATAL_ERROR "CustomWriter executable not found: ${WRITER_EXE}")
endif()

file(REMOVE "${OUTPUT_FILE}")
execute_process(
  COMMAND "${WRITER_EXE}" "${OUTPUT_FILE}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE stdout_text
  ERROR_VARIABLE stderr_text
)

if(NOT result EQUAL 0)
  message(FATAL_ERROR
    "CustomWriter failed (${result})\nstdout:\n${stdout_text}\nstderr:\n${stderr_text}")
endif()

if(NOT EXISTS "${OUTPUT_FILE}")
  message(FATAL_ERROR "CustomWriter did not create ${OUTPUT_FILE}")
endif()

file(READ "${OUTPUT_FILE}" actual)
string(REPLACE "\r\n" "\n" actual "${actual}")
set(expected
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<catalog>
  <book id=\"42\">
    <title>Example Book</title>
  </book>
  <notice/>
</catalog>
")

if(NOT actual STREQUAL expected)
  message(FATAL_ERROR
    "Unexpected XML output\nExpected:\n${expected}\nActual:\n${actual}")
endif()

file(REMOVE "${OUTPUT_FILE}")
```

Append this to `CMakeLists.txt`:

```cmake
add_executable(CustomWriter
  src/main.cpp
)
target_link_libraries(CustomWriter PRIVATE custom_writer)

add_test(NAME customwriter_cli
  COMMAND ${CMAKE_COMMAND}
    "-DWRITER_EXE=$<TARGET_FILE:CustomWriter>"
    "-DOUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/cli-smoke.xml"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_smoke.cmake"
)
```

Run:

```powershell
$env:PATH = "E:\Qt6.8.3\Tools\mingw1310_64\bin;" + $env:PATH
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

Expected: 编译失败，因为 `src/main.cpp` 尚不存在。

- [ ] **Step 2: 实现命令行程序**

Create `src/main.cpp`:

```cpp
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
         writer.emptyElement("notice") &&
         writer.endElement() &&
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
```

- [ ] **Step 3: 运行全部自动测试**

Run:

```powershell
$env:PATH = "E:\Qt6.8.3\Tools\mingw1310_64\bin;" + $env:PATH
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: `xml_writer_tests` 和 `customwriter_cli` 均通过。

- [ ] **Step 4: 编写中文 README 并手工运行**

Create `README.md` with:

````markdown
# CustomWriter

一个使用 C++17 文件流写出 XML 的基础工具，同时提供可复用库和命令行示例。

## 功能

- XML 声明、嵌套元素、属性、文本和空元素。
- 文本及属性自动转义。
- 默认两空格缩进。
- 元素嵌套、根元素和文件 I/O 错误检查。
- UTF-8 和 LF 输出。

## 构建

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

如果当前 PowerShell 找不到 C++ 编译器，可以将本机 MinGW 加入当前会话：

```powershell
$env:PATH = "E:\Qt6.8.3\Tools\mingw1310_64\bin;" + $env:PATH
cmake -S . -B build -G "MinGW Makefiles"
```

## 运行

```powershell
.\build\CustomWriter.exe output.xml
```

## 库接口

```cpp
#include "custom_writer/xml_writer.hpp"

custom_writer::XmlWriter writer("output.xml");
writer.writeDeclaration();
writer.startElement("root");
writer.startElement("message");
writer.writeText("Hello & XML");
writer.endElement();
writer.endElement();
writer.close();
```

每次调用都返回 `bool`。失败时通过 `lastError()` 获取原因，并显式调用
`close()` 完成文件流清理。
````

Run a manual build in a path containing spaces:

```powershell
$env:PATH = "E:\Qt6.8.3\Tools\mingw1310_64\bin;" + $env:PATH
$output = Join-Path $PWD "build output\example.xml"
New-Item -ItemType Directory -Force (Split-Path $output) | Out-Null
.\build\CustomWriter.exe $output
Get-Content -Raw $output
```

Expected: 命令返回 0，回读结果与 CLI 集成测试中的目录文档完全一致。

- [ ] **Step 5: 提交命令行程序和文档**

```powershell
git add CMakeLists.txt README.md src/main.cpp tests/cli_smoke.cmake
git commit -m "feat: add CustomWriter command-line example"
```

---

## Final Verification

- [ ] 从干净状态配置 Release 构建。
- [ ] 编译所有目标且无警告。
- [ ] 运行 `ctest --test-dir build -C Release --output-on-failure`，全部测试通过。
- [ ] 运行 `CustomWriter.exe` 生成 XML，并用 `Get-Content -Raw` 回读确认内容。
- [ ] 运行 `git status --short`，确认工作区干净。
