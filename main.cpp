#include "print.h"
#include <charconv>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

struct JSONObject;
using JSONDict = std::unordered_map<std::string, JSONObject>;
using JSONList = std::vector<JSONObject>;

struct JSONObject
{
	std::variant<
		std::monostate  // null
		, bool          // True
		, int           // 42
		, double        // 3.14
		, std::string   // "hello"
		, JSONList      // [42, "hello"]
		, JSONDict      // {"hello":985, "world":211}
	> inner;

	void do_print() const { // Customize print(variant) for the need of print.h .
		print(inner);
	};

	template<class T>
	bool is() const {
		return bool(std::holds_alternative<T>(inner));
	};

	template<class T>
	T& get() {
		return std::get<T>(inner);
	}
	template<class T>
	T const& get() const {
		return std::get<T>(inner);
	}
};

template<class T>
std::optional<T> tryParseNum(std::string_view str) // int and double.
{
	T val;
	auto res = std::from_chars(str.data(), str.data() + str.size(), val);
	if (res.ec == std::errc() && res.ptr == str.data() + str.size())
	{
		return val;
	}

	return std::nullopt;
}

char unescaped_char(char c) {
	switch (c)
	{
	case 'n':
		return '\n';
	case 'r':
		return '\r';
	case '0':
		return '\0';
	case 't':
		return '\t';
	case 'v':
		return '\v';
	case 'f':
		return '\f';
	case 'b':
		return '\b';
	case 'a':
		return '\a';
	default:
		return c;
	}
}

std::pair<JSONObject, size_t> parse(std::string_view json) {
	if (json.empty()) // null
	{
		return { JSONObject{ std::monostate{}}, 0 };
	}
	else if (size_t off = json.find_first_not_of(" \n\r\t\v\f\0");off != 0 && off != std::string_view::npos)
	{ // Input format problem.
		auto [obj, eaten] = parse(json.substr(off));
		return { std::move(obj), eaten + off };
	}
	else if ((json.at(0) >= '0' && json.at(0) <= '9') || json[0] == '+' || json[0] == '-') // int and double.
	{
		std::regex
			numRe{ "[+-]?[0-9]+(\\.[0-9]*)?([eE][+-]?[0-9]+)?" }; // scientific notation based regular expression.
		std::cmatch match;
		if (std::regex_search(json.data(), json.data() + json.size(), match, numRe))
		{
			std::string str = match.str();
			if (auto num = tryParseNum<int>(str);
				num.has_value())
			{ // Another way to write it: if(auto num: = tryParseNum<int> str)).
				return { JSONObject{ num.value() }, str.size() }; // Another way to write it: return JSONObject(*num).
			}
			if (auto num = tryParseNum<double>(str); num.has_value())
			{
				return { JSONObject{ num.value() }, str.size() };
			}
		}
	}
	else if (json[0] == '"') // string
	{
		std::string str;
		enum
		{
			Raw,
			Escaped,
		} phase = Raw;
		size_t i;
		for (i = 1; i < json.size(); ++i)
		{
			char ch = json[i];
			if (phase == Raw)
			{
				if (ch == '\\')
				{
					phase = Escaped;
				}
				else if (ch == '"')
				{
					i += 1;
					break;
				}
				else
				{
					str += ch;
				}
			}
			else
			{
				str += unescaped_char(ch);
				phase = Raw;
			}
		}
		return { JSONObject{ str }, i }; // return 'i' instead of 'str.size()', means 'eaten'.
	}
	else if (json[0] == '[') // vector<JSONObject>
	{
		std::vector<JSONObject> res;
		size_t i;
		for (i = 1; i < json.size();)
		{
			if (json[i] == ']')
			{
				i += 1;
				break;
			}
			auto [obj, eaten] = parse(json.substr(i)); // parse subJSONObject.
			// when obj is std::string, the size of eaten should be obj.inner.size()+2.
			if (eaten == 0)
			{
				i = 0; // when an exception occurs here, exit and clear the state.
				break;
			}
			res.push_back(std::move(obj));
			i += eaten;
			if (json[i] == ',')
			{
				i += 1;
			}
		}

		return { JSONObject{ std::move(res) }, i };
	}
	else if (json[0] == '{')
	{
		std::unordered_map<std::string, JSONObject> res;
		size_t i;
		for (i = 1; i < json.size();)
		{
			if (json[i] == '}')
			{
				i += 1;
				break;
			}
			auto [keyObj, keyEaten] = parse(json.substr(i));
			if (keyEaten == 0)
			{
				i = 0;
				break;
			}
			i += keyEaten;
			if (!std::holds_alternative<std::string>(keyObj.inner))
			{
				i = 0;
				break;
			}
			if (json[i] == ':')
			{
				i += 1;
			}
			std::string key = std::move(std::get<std::string>(keyObj.inner));
			auto [valObj, valEaten] = parse(json.substr(i));
			if (valEaten == 0)
			{
				i = 0;
				break;
			}
			i += valEaten;
			res.insert_or_assign(std::move(key), std::move(valObj)); // hot refresh.
			if (json[i] == ',')
			{
				i += 1;
			}
		}
		return { JSONObject{ std::move(res) }, i };
	}

	return { JSONObject{ std::monostate{}}, 0 };
};

int main() {
	std::string_view str = R"JSON({"work":996,"school":[985,[211,101]]})JSON";
	auto [obj, eaten] = parse(str);
//	print(std::get<std::string>(obj.inner).c_str());
	print(obj.is<JSONDict>());
	auto const& dict = obj.get<JSONDict>();
	print("The capitalist make me work in", dict.at("work").get<int>());

	auto const& school = dict.at("school");

	auto doVisit = [](auto& doVisit,auto const& school)->void
	{
	  std::visit([&](auto const&school)
	  {
		  if constexpr (std::is_same_v<std::decay_t<decltype(school)>, JSONList>)
		  {
			  for (auto const& subSchool : school)
			  {
				  doVisit(doVisit,subSchool);
			  }
		  }
		  else
		  {
			  print("I purchased my diploma from", school);
		  }
	  },school.inner);
	}; // Recursive calls to lambda expressions. when the code runs here, lambda expression is initialized.
	doVisit(doVisit,school);

	return 0;
}
