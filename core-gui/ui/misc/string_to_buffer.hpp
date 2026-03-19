
#include <string>
#include <array>
#include <cstring>

template <size_t N>
void CopyStringToBuffer(std::array<char, N>& buffer, const std::string& text) {
	const size_t copy_len = std::min(text.size(), N > 0 ? N - 1 : 0);
	if (copy_len > 0) {
		std::memcpy(buffer.data(), text.data(), copy_len);
	}
	if (N > 0) {
		buffer[copy_len] = '\0';
	}
}