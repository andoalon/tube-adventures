#include "annotations.hh"

#include <tinyxml2/tinyxml2.h>

#include <cassert>
#include <iostream>
#include <filesystem>
#include <string>
#include <string_view>
#include <charconv>

namespace
{
	// Check when floating-point std::from_chars is available: https://en.cppreference.com/w/cpp/compiler_support
	// https://en.cppreference.com/w/cpp/utility/from_chars
	// https://en.cppreference.com/w/cpp/string/basic_string/stof
	template <typename Floating, typename = std::enable_if_t<std::is_floating_point_v<Floating>>>
	std::from_chars_result from_chars(const char * const begin, const char * const end, Floating & out_value, const std::chars_format format = std::chars_format::general) noexcept
	{
#if defined __cpp_lib_to_chars || (defined _MSC_VER && _MSC_VER >= 1915 /* Visual Studio 2017 15.8*/)
		return std::from_chars(begin, end, out_value, format);
#else
		assert(format == std::chars_format::general);

		try
		{
			const std::string string(begin, end);
			Floating result_value;

			if constexpr (std::is_same_v<Floating, float>)
				result_value = std::stof(string);
			else if constexpr (std::is_same_v<Floating, double>)
			{
				result_value = std::stod(string);
			}
			else
			{
				static_assert(std::is_same_v<Floating, long double>);
				result_value = std::stold(string);
			}

			out_value = result_value;

			std::from_chars_result result;
			result.ec = {};
			result.ptr = end;

			return result;
		}
		catch (const std::invalid_argument &)
		{
			std::from_chars_result result;
			result.ec = std::errc::invalid_argument;
			result.ptr = begin;

			return result;
		}
		catch (const std::out_of_range &)
		{
			std::from_chars_result result;
			result.ec = std::errc::result_out_of_range;

			// Technically it should point to the first character
			// not matching the pattern, but we can't get that information
			result.ptr = begin;

			return result;
		}
		catch (...)
		{
			std::abort();
		}
#endif
	}

	template <typename Floating, typename = std::enable_if_t<std::is_floating_point_v<Floating>>>
	std::from_chars_result from_chars(const std::string_view str, Floating & out_value, const std::chars_format format = std::chars_format::general) noexcept
	{
		return ::from_chars(str.data(), str.data() + str.size(), out_value, format);
	}

	template <typename Integral, typename = std::enable_if_t<std::is_integral_v<Integral> && !std::is_same_v<Integral, bool>>>
	std::from_chars_result from_chars(const std::string_view str, Integral & out_value, const int base = 10) noexcept
	{
		return std::from_chars(str.data(), str.data() + str.size(), out_value, base);
	}
} // namespace

#ifdef TUBE_ADVENTURES_DEBUG
#	include <QMessageBox>
	void maybe_this_xml_format_should_be_handled(const QString & message)
	{
		QMessageBox::warning(nullptr, "XML considered invalid. Should this be valid/handled in another way?", message, QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::NoButton);
	}

#	define TUBE_ADVENTURES_MAYBE_THIS_XML_FORMAT_SHOULD_BE_HANDLED(message) maybe_this_xml_format_should_be_handled(message)
#else
#	define TUBE_ADVENTURES_MAYBE_THIS_XML_FORMAT_SHOULD_BE_HANDLED(message)
#endif // TUBE_ADVENTURES_DEBUG

using namespace std::literals;
using namespace std::literals::string_view_literals;

#define TUBE_ADVENTURES_FROM_CHARS_REQUIRED(string, out_value) \
	{\
		using namespace std::string_literals;\
		const std::from_chars_result parse_##string##_result = ::from_chars(string, out_value);\
		if (parse_##string##_result.ec != std::errc{})\
		{\
			return { ParseAnnotationsError::invalid_format, {}, "Failed to parse " #out_value " from string: \""s /*+ std::string(string) + "\". Error: " + std::to_string(static_cast<std::underlying_type_t<std::errc>>(parse_##string##_result.ec))*/ };\
		}\
	}

#define TUBE_ADVENTURES_GET_REQUIRED_ATTRIBUTE(node, attribute)\
	const tinyxml2::XMLAttribute * const node##_##attribute = node->FindAttribute(#attribute);\
	if (node##_##attribute == nullptr)\
	{\
		return { ParseAnnotationsError::invalid_format, {}, "<" #node "> without \"" #attribute "\" attribute"s }; \
	}\
	const char * const node##_##attribute##_str = node##_##attribute->Value();\
	if (node##_##attribute##_str == nullptr)\
	{\
		return { ParseAnnotationsError::invalid_format, {}, "<" #node " " #attribute " = nullptr>"s }; \
	}\
	const std::string_view node##_##attribute##_str_view = node##_##attribute##_str

#define TUBE_ADVENTURES_EXPECT_ATTRIBUTE_WITH_VALUE(node, attribute, expected_attribute_value) \
	TUBE_ADVENTURES_GET_REQUIRED_ATTRIBUTE(node, attribute);\
	if (node##_##attribute##_str_view != TUBE_ADVENTURES_STRING_VIEW_LITERAL(expected_attribute_value))\
	{\
		return { ParseAnnotationsError::invalid_format, {}, std::string("<" #node " " #attribute " != \"" expected_attribute_value) + "\"> (actual value = \"" + std::string(node##_##attribute##_str_view) + "\")" };\
	}


ParseAnnotationsResult parse_annotations(const char * xml_filename)
{
	assert(xml_filename != nullptr);

	tinyxml2::XMLDocument doc;
	if (const tinyxml2::XMLError error = doc.LoadFile(xml_filename); error != tinyxml2::XMLError::XML_SUCCESS)
	{
		std::error_code filename_error;
		auto full_filename = std::filesystem::weakly_canonical(xml_filename, filename_error);

		if (filename_error)
		{
			std::clog << "Can't get absolute filename of path: \"" << xml_filename << "\". Error: " << filename_error.message() << '\n';
			full_filename = xml_filename;
		}

		switch (error)
		{
		case tinyxml2::XMLError::XML_ERROR_FILE_NOT_FOUND:
			return { ParseAnnotationsError::file_not_found, {}, "File \"" + full_filename.u8string() + "\" not found" };
		case tinyxml2::XMLError::XML_ERROR_FILE_COULD_NOT_BE_OPENED:
		case tinyxml2::XMLError::XML_ERROR_FILE_READ_ERROR:
			return { ParseAnnotationsError::cannot_read_file, {}, "Cannot open or read file" };
		default:
			return { ParseAnnotationsError::invalid_xml, {}, "XML parse error: " + full_filename.u8string() + ". Error type: " + doc.ErrorName() + ". Error: " + doc.ErrorStr() + '\n' };
		}
	}

	const tinyxml2::XMLElement* const document = doc.FirstChildElement("document");
	if (document == nullptr)
	{
		return { ParseAnnotationsError::invalid_format, {}, "File with no <document>" };
	}

	const tinyxml2::XMLElement* const annotations = document->FirstChildElement("annotations");
	if (annotations == nullptr)
	{
		return { ParseAnnotationsError::invalid_format, {}, "<document> with no <annotations> inside" };
	}

	constexpr char annotation_name[] = "annotation";
	const tinyxml2::XMLElement* annotation = annotations->FirstChildElement(annotation_name);
	if (annotation == nullptr) // Maybe a file with 0 annotations should be considered valid?
	{
		return { ParseAnnotationsError::invalid_format, {}, "<annotations> with no <annotation> inside" };
	}

	std::vector<Annotation> result_annotations;

	do
	{
		assert(annotation != nullptr);

		Annotation result_annotation;

		if (const tinyxml2::XMLAttribute * const annotation_id = annotation->FindAttribute("id"); annotation_id != nullptr)
		{
			const char * const annotation_id_str = annotation_id->Value();
			if (annotation_id_str != nullptr)
				result_annotation.id = annotation_id_str;
		}

		{
			// Detect "non real" annotations
			TUBE_ADVENTURES_GET_REQUIRED_ATTRIBUTE(annotation, type);
			if (annotation_type_str_view != "text"sv)
				continue;

			TUBE_ADVENTURES_GET_REQUIRED_ATTRIBUTE(annotation, style);
			if (annotation_style_str_view != "popup"sv)
				continue;
		}

		const tinyxml2::XMLElement * const annotation_text = annotation->FirstChildElement("TEXT");
		if (annotation_text != nullptr)
		{
			const char* const text = annotation_text->GetText();
			if (text == nullptr)
			{
				return { ParseAnnotationsError::invalid_format, {}, "<TEXT> with no text" };
			}

			result_annotation.text = text;
		}

		if (const tinyxml2::XMLElement* const action = annotation->FirstChildElement("action"); action == nullptr)
			result_annotation.type = Annotation::Type::notes;
		else // Should have url
		{
			{
				TUBE_ADVENTURES_EXPECT_ATTRIBUTE_WITH_VALUE(action, type, "openUrl");
			}
			{
				TUBE_ADVENTURES_EXPECT_ATTRIBUTE_WITH_VALUE(action, trigger, "click");
			}

			const tinyxml2::XMLElement* const url = action->FirstChildElement("url");
			if (url == nullptr)
			{
				return { ParseAnnotationsError::invalid_format, {}, "<action> with no url" };
			}

			{
				TUBE_ADVENTURES_GET_REQUIRED_ATTRIBUTE(url, value);
				result_annotation.click_url = url_value_str_view;
			}

			{
				TUBE_ADVENTURES_GET_REQUIRED_ATTRIBUTE(url, target);
				if (url_target_str_view == "current"sv)
					result_annotation.type = Annotation::Type::gameplay;
				else if (url_target_str_view == "new"sv)
				{
					{
						TUBE_ADVENTURES_EXPECT_ATTRIBUTE_WITH_VALUE(url, type, "hyperlink");
					}
					result_annotation.type = Annotation::Type::external_link;
				}
			}
		}

		if (const tinyxml2::XMLElement * const segment = annotation->FirstChildElement("segment"); segment == nullptr)
		{
			return { ParseAnnotationsError::invalid_format, {}, "<annotation> without <segment>" };
		}
		else
		{
			if (segment->NoChildren())
				continue; // Not a real annotation

			const tinyxml2::XMLElement * const moving_region = segment->FirstChildElement("movingRegion");
			if (moving_region == nullptr)
			{
				return { ParseAnnotationsError::invalid_format, {}, "<segment> without <movingRegion>" };
			}

			{
				TUBE_ADVENTURES_EXPECT_ATTRIBUTE_WITH_VALUE(moving_region, type, "rect");
			}

			{
				constexpr char rect_region_name[] = "rectRegion";
				const tinyxml2::XMLElement * rect_region = moving_region->FirstChildElement(rect_region_name);
				if (rect_region == nullptr)
				{
					return { ParseAnnotationsError::invalid_format, {}, "<movingRegion> without <rectRegion>" };
				}

				int rect_region_index = 0;
				do
				{
					if (rect_region_index > 1)
					{
						return { ParseAnnotationsError::invalid_format, {}, "More than 2 <rectRegion> in <movingRegion>" };
					}

					assert(rect_region_index == 0 || rect_region_index == 1);
					Annotation::RectRegion & result_rect_region = (rect_region_index == 0)
						? result_annotation.start_rect
						: (*(result_annotation.end_rect = Annotation::RectRegion{}));
					++rect_region_index;

					{
						TUBE_ADVENTURES_GET_REQUIRED_ATTRIBUTE(rect_region, x);
						TUBE_ADVENTURES_FROM_CHARS_REQUIRED(rect_region_x_str_view, result_rect_region.x);
					}

					{
						TUBE_ADVENTURES_GET_REQUIRED_ATTRIBUTE(rect_region, y);
						TUBE_ADVENTURES_FROM_CHARS_REQUIRED(rect_region_y_str_view, result_rect_region.y);
					}

					{
						TUBE_ADVENTURES_GET_REQUIRED_ATTRIBUTE(rect_region, w);
						TUBE_ADVENTURES_FROM_CHARS_REQUIRED(rect_region_w_str_view, result_rect_region.width);
					}

					{
						TUBE_ADVENTURES_GET_REQUIRED_ATTRIBUTE(rect_region, h);
						TUBE_ADVENTURES_FROM_CHARS_REQUIRED(rect_region_h_str_view, result_rect_region.height);
					}

					{
						TUBE_ADVENTURES_GET_REQUIRED_ATTRIBUTE(rect_region, t);

						unsigned hours, minutes, seconds, centiseconds;
						const auto amount_of_correct =
#					ifdef _MSC_VER
							sscanf_s(
#					else
								std::sscanf
#					endif
								rect_region_t_str, "%u:%u:%u.%u", &hours, &minutes, &seconds, &centiseconds);

								constexpr auto expected_amount_of_correct = 4;
								if (amount_of_correct != expected_amount_of_correct)
								{
									return { ParseAnnotationsError::invalid_format, {}, "Parsing timestamp. Amount of correct parses (" + std::to_string(amount_of_correct) + ") is different from the expected (" + std::to_string(expected_amount_of_correct) + ')' };
								}

								using centiseconds_t = std::chrono::duration<std::chrono::seconds::rep, std::ratio<1, 100>>;
								result_rect_region.time = std::chrono::hours{ hours } +std::chrono::minutes{ minutes } +std::chrono::seconds{ seconds } +centiseconds_t{ centiseconds };
					}
				} while (rect_region = rect_region->NextSiblingElement(rect_region_name));

#ifdef TUBE_ADVENTURES_DEBUG
				if (rect_region_index == 1)
					__debugbreak(); // There was actually an annotation with a single rect region
#endif
			}

			const tinyxml2::XMLElement * appearance = annotation->FirstChildElement("appearance");
			if (appearance == nullptr)
			{
				return { ParseAnnotationsError::invalid_format, {}, "<annotation> without <appearance>" };
			}
			
			{
				TUBE_ADVENTURES_GET_REQUIRED_ATTRIBUTE(appearance, textSize);
				TUBE_ADVENTURES_FROM_CHARS_REQUIRED(appearance_textSize_str_view, result_annotation.text_size);
			}

			{
				const auto check_color_rgb = [](const QRgb original_color, const QColor & color, const char * color_name) -> std::string
				{
					if (color.isValid())
						return {};

					char buffer[256];
					buffer[0] = '0';
					buffer[1] = 'x';
					const std::to_chars_result result = std::to_chars(std::begin(buffer) + 2, std::end(buffer), original_color, 16);
					assert(result.ec == std::errc{});

					const auto length = result.ptr - std::begin(buffer);
					assert(length > 0 && length < static_cast<std::ptrdiff_t>(std::size(buffer)));

					const auto color_str_view = std::string_view(&buffer[0], static_cast<std::size_t>(length));

					return color_name + " rgb color is invalid ("s + std::string(color_str_view) + "). Max is: 0xFFFFFF";
				};

				{
					TUBE_ADVENTURES_GET_REQUIRED_ATTRIBUTE(appearance, bgColor);
					QRgb background_color;
					TUBE_ADVENTURES_FROM_CHARS_REQUIRED(appearance_bgColor_str_view, background_color);

					result_annotation.background_color = QColor::fromRgb(background_color);

					if (auto color_error = check_color_rgb(background_color, result_annotation.background_color, "Background"); !color_error.empty())
						return { ParseAnnotationsError::invalid_format, {}, std::move(color_error) };


					TUBE_ADVENTURES_GET_REQUIRED_ATTRIBUTE(appearance, bgAlpha);
					float background_alpha;
					TUBE_ADVENTURES_FROM_CHARS_REQUIRED(appearance_bgAlpha_str_view, background_alpha);

					result_annotation.background_color.setAlphaF(background_alpha);
					if (!result_annotation.background_color.isValid())
					{
						return { ParseAnnotationsError::invalid_format, {}, "Background alpha is invalid (" + std::to_string(background_alpha) + "). The valid range is: [0.0, 1.0]" };
					}
				}

				TUBE_ADVENTURES_GET_REQUIRED_ATTRIBUTE(appearance, fgColor);
				QRgb foreground_color;
				TUBE_ADVENTURES_FROM_CHARS_REQUIRED(appearance_fgColor_str_view, foreground_color);
				result_annotation.foreground_color = QColor::fromRgb(foreground_color);

				if (auto color_error = check_color_rgb(foreground_color, result_annotation.foreground_color, "Foreground"); !color_error.empty())
					return { ParseAnnotationsError::invalid_format, {}, std::move(color_error) };

				{
					TUBE_ADVENTURES_GET_REQUIRED_ATTRIBUTE(appearance, effects);
					if (!appearance_effects_str_view.empty())
					{
						return { ParseAnnotationsError::invalid_format, {}, "The \"effects\" attribute of <appearance> is not empty. \"effects\" is (currently) not supported" };
					}
				}
			}
		}

		result_annotations.emplace_back(std::move(result_annotation));
	} while (annotation = annotation->NextSiblingElement(annotation_name));

	return { ParseAnnotationsError::success, std::move(result_annotations), {} };
}

std::optional<std::string> full_youtube_url_from_id(const std::string_view video_id)
{
	constexpr std::string_view base = "https://www.youtube.com/watch?v=";

	if (video_id.size() != youtube_video_id_length)
		return std::nullopt;

	constexpr auto final_size = base.size() + youtube_video_id_length;

	std::string result;
	result.reserve(final_size);
	result = base;
	result += video_id;

	return result;
}

[[nodiscard]] std::optional<std::string> path_to_youtube_video_id(const std::filesystem::path & annotation_file_path, const std::filesystem::path & expected_extension)
{
	 const auto stem = annotation_file_path.stem();

	 if (stem.empty())
		 return std::nullopt;

	 if (annotation_file_path.extension() != expected_extension)
		 return std::nullopt;

	 const auto stem_str = stem.string();
	 const auto stem_size = stem_str.size();

	 if (stem_size <= youtube_video_id_length) // ID + space
		 return std::nullopt;

	 const auto found_index = stem_str.find_last_of(' ');
	 if (found_index == std::string::npos || found_index + youtube_video_id_length + 1 != stem_size)
		 return std::nullopt;

	 return stem_str.substr(found_index + 1);
}
