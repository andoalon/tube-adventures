#pragma once

#include <optional>
#include <string>
#include <chrono>
#include <string_view>

#include <QColor>

#ifdef __cpp_char8_t
	using u8char = char8_t;
#else
	using u8char = char;
#endif

using u8string = std::basic_string<u8char>;
using u8string_view = std::basic_string_view<u8char>;

struct Annotation
{
	std::string id;
	u8string text; // optional

	struct RectRegion
	{
		float x;
		float y;

		float width;
		float height;
		std::chrono::milliseconds time;
	};

	RectRegion start_rect;
	std::optional<RectRegion> end_rect;

	QColor background_color; // RGBA
	QColor foreground_color; // RGB
	float text_size;

	std::string click_url; // optional
	
	enum class Type
	{
		gameplay,
		notes,
		external_link,
	};

	Type type;
};

#define TUBE_ADVENTURES_PARSE_ANNOTATION_ERROR_ENUMERATORS  \
	TUBE_ADVENTURES_PARSE_ANNOTATION_ERROR_ENUMERATOR(success)\
	TUBE_ADVENTURES_PARSE_ANNOTATION_ERROR_ENUMERATOR(file_not_found)\
	TUBE_ADVENTURES_PARSE_ANNOTATION_ERROR_ENUMERATOR(cannot_read_file)\
	TUBE_ADVENTURES_PARSE_ANNOTATION_ERROR_ENUMERATOR(invalid_xml)\
	TUBE_ADVENTURES_PARSE_ANNOTATION_ERROR_ENUMERATOR(invalid_format)

#define TUBE_ADVENTURES_PARSE_ANNOTATION_ERROR_ENUMERATOR(enumerator) enumerator,

enum class ParseAnnotationsError
{
	TUBE_ADVENTURES_PARSE_ANNOTATION_ERROR_ENUMERATORS
};
#undef TUBE_ADVENTURES_PARSE_ANNOTATION_ERROR_ENUMERATOR

#define TUBE_ADVENTURES_STRING_VIEW_LITERAL_IMPL(str) str##sv
#define TUBE_ADVENTURES_STRING_VIEW_LITERAL(str) TUBE_ADVENTURES_STRING_VIEW_LITERAL_IMPL(str)

#ifdef TUBE_ADVENTURES_DEBUG
#	define TUBE_ADVENTURES_PARSE_ANNOTATION_ERROR_ENUMERATOR(enumerator) TUBE_ADVENTURES_STRING_VIEW_LITERAL(#enumerator),

	[[nodiscard]] constexpr std::string_view to_string(const ParseAnnotationsError error) noexcept
	{
		using namespace std::string_view_literals;

		constexpr std::string_view strings[] = {
			TUBE_ADVENTURES_PARSE_ANNOTATION_ERROR_ENUMERATORS
		};
	
		using underlying = std::underlying_type_t<ParseAnnotationsError>;
	
		assert(static_cast<underlying>(error) >= 0 && static_cast<underlying>(error) < static_cast<underlying>(std::size(strings)));

		return strings[static_cast<underlying>(error)];
	}
#	undef TUBE_ADVENTURES_PARSE_ANNOTATION_ERROR_ENUMERATOR
#endif // TUBE_ADVENTURES_DEBUG

#undef TUBE_ADVENTURES_PARSE_ANNOTATION_ERROR_ENUMERATORS


struct ParseAnnotationsResult
{
	ParseAnnotationsError error;
	std::vector<Annotation> annotations; // Empty unless error == ParseAnnotationsError::success
	std::string error_string;
};

ParseAnnotationsResult parse_annotations(const char * xml_filename);

