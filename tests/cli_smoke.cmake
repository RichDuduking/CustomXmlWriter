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
