#include <algorithm>

struct KeyValue {
	String key;
	String value;
};

struct KvParseResult {
	std::vector<KeyValue> pairs;

	String getOrDefault(const String& key, const String& valueIfNull = "") const {
		const auto it = std::find_if(pairs.begin(), pairs.end(), [&key](const KeyValue& kv) { return kv.key == key; });

		if (it != pairs.end())
			return it->value;

		return valueIfNull;
	}

	bool contains(const String& key) const {
		return std::find_if(pairs.begin(), pairs.end(), [&key](const KeyValue& kv) { return kv.key == key; }) != pairs.end();
	}
};

std::vector<String> splitString(const String& input, const String& delimiter) {
	std::vector<String> result;

	int start = 0;
	int end   = input.indexOf(delimiter);

	while (end != -1) {
		result.push_back(input.substring(start, end));
		start = end + delimiter.length();
		end   = input.indexOf(delimiter, start);
	}

	result.push_back(input.substring(start));

	return result;
}

KvParseResult kvParseMultiline(const String& input) {
	KvParseResult result{};

	const auto lines = splitString(input, "\n");

	for (const auto& line : lines) {
		const auto parts = splitString(line, "=");

		if (parts.size() < 2)
			continue;

		result.pairs.push_back({parts[0], parts[1]});
	}

	return result;
}