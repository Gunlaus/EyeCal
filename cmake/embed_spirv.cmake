file(READ "${INPUT}" bytes HEX)
string(REGEX REPLACE "(..)(..)(..)(..)" "0x\\4\\3\\2\\1u,\n" words "${bytes}")
file(WRITE "${OUTPUT}" "#pragma once\n#include <cstdint>\ninline constexpr std::uint32_t ${NAME}[] = {\n${words}};\n")
