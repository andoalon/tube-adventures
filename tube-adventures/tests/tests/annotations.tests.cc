#include <catch2/catch.hpp>

#include "annotations.hh"

#include <string_view>
#include <filesystem>
#include <set>

using namespace std::string_view_literals;

#ifdef TUBE_ADVENTURES_DEBUG
namespace Catch
{
	template<>
	struct StringMaker<ParseAnnotationsError>
	{
		[[nodiscard]] static std::string convert(const ParseAnnotationsError error) noexcept
		{
			return std::string(to_string(error));
		}
	};
} // namespace Catch
#endif // TUBE_ADVENTURES_DEBUG

namespace
{
	const std::filesystem::path annotations_dir = "../../../data";
	const std::filesystem::path tube_adventures_1_dir = annotations_dir / "TUBE-ADVENTURES";

	using centiseconds = std::chrono::duration<std::chrono::seconds::rep, std::ratio<1, 100>>;

	[[nodiscard]] constexpr std::chrono::milliseconds time_to_timestamp(
		const std::chrono::hours hours,
		const std::chrono::minutes minutes,
		const std::chrono::seconds seconds,
		const centiseconds centiseconds) noexcept
	{
		return hours + minutes + seconds + centiseconds;
	}

	[[nodiscard]] QColor unsigned_rgb_and_alpha_float(const std::uint32_t rgb, const float alpha)
	{
		QColor color = rgb;
		color.setAlphaF(alpha);

		return color;
	}

	void check_rect_region(const Annotation::RectRegion & actual, const Annotation::RectRegion & expected)
	{
		CHECK(actual.x == expected.x);
		CHECK(actual.y == expected.y);
		CHECK(actual.width == expected.width);
		CHECK(actual.height == expected.height);
		CHECK(actual.time == expected.time);
	}

	void check_annotation(const std::string_view info, const Annotation & actual, const Annotation & expected)
	{
		INFO(info);
		CHECK(actual.text == expected.text);
		CHECK(actual.click_url == expected.click_url);
		CHECK(actual.background_color == expected.background_color);
		CHECK(actual.foreground_color == expected.foreground_color);
		CHECK(actual.id == expected.id);
		CHECK(actual.text == expected.text);
		CHECK(actual.text_size == expected.text_size);
		CHECK(actual.type == expected.type);

		check_rect_region(actual.start_rect, expected.start_rect);

		CHECK(actual.end_rect.has_value() == expected.end_rect.has_value());
		if (actual.end_rect.has_value() && expected.end_rect.has_value())
			check_rect_region(*actual.end_rect, *expected.end_rect);
	}
} // namespace

TEST_CASE("Can parse an annotation file")
{
	using namespace std::chrono_literals;
	using namespace std::string_literals;

	const auto filename = tube_adventures_1_dir / "TUBE-ADVENTURES (aventura interactiva) BckqqsJiDUI.xml";

	const ParseAnnotationsResult result = parse_annotations(filename.u8string().c_str());
	const std::vector<Annotation> & annotations = result.annotations;

	INFO("Result: { " + result.error_string + " }");
	REQUIRE(result.error == ParseAnnotationsError::success);
	REQUIRE(result.error_string == "");
	REQUIRE(annotations.size() == 7);

	const u8string go_here(u8"Ir aquí / go here"sv);
	const u8string missing_url = {};

	const QColor foreground_color = QColor::fromRgb(1710618);
	const QColor background_color = unsigned_rgb_and_alpha_float(16777215, 0.8f);

	constexpr float text_size = 3.6107f;

	check_annotation("annotations[0]", annotations[0], Annotation{
		"annotation_103323"s,
		u8"Música y + info en www.tube-adventures.blogspot.com"s,
		Annotation::RectRegion{
			14.16700f, 100.00000f,
			71.25000f, 12.22200f,
			time_to_timestamp(0h, 0min, 4s, centiseconds{ 0 })
		},
		Annotation::RectRegion{
			14.16700f, 100.00000f,
			71.25000f, 12.22200f,
			time_to_timestamp(0h, 0min, 8s, centiseconds{ 0 })
		},
		background_color, // background color (RGBA)
		foreground_color, // foreground color (RGB)
		text_size, // text size
		missing_url, // url
		Annotation::Type::notes
	});

	check_annotation("annotations[1]", annotations[1], Annotation{
		"annotation_281740"s,
		u8"Welcome"s,
		Annotation::RectRegion{
			23.54200f, 69.63000f,
			27.70800f, 7.03700f,
			time_to_timestamp(0h, 0min, 33s, centiseconds{ 79 })
		},
		Annotation::RectRegion{
			23.54200f, 69.63000f,
			27.70800f, 7.03700f,
			time_to_timestamp(0h, 0min, 36s, centiseconds{ 50 })
		},
		background_color, // background color (RGBA)
		foreground_color, // foreground color (RGB)
		text_size, // text size
		missing_url, // url
		Annotation::Type::notes
	});

	check_annotation("annotations[2]", annotations[2], Annotation{
		"annotation_30312"s,
		u8"Starring"s,
		Annotation::RectRegion{
			8.75000f, 10.74100f,
			18.75000f, 8.51800f,
			time_to_timestamp(0h, 0min, 19s, centiseconds{ 80 })
		},
		Annotation::RectRegion{
			8.75000f, 10.74100f,
			18.75000f, 8.51800f,
			time_to_timestamp(0h, 0min, 21s, centiseconds{ 89 })
		},
		background_color, // background color (RGBA)
		foreground_color, // foreground color (RGB)
		text_size, // text size
		missing_url, // url
		Annotation::Type::notes
	});

	check_annotation("annotations[3]", annotations[3], Annotation{
		"annotation_671574"s,
		go_here,
		Annotation::RectRegion{
			3.54200f, 53.05600f,
			26.45800f, 8.05600f,
			time_to_timestamp(0h, 1min, 45s, centiseconds{ 35 })
		},
		Annotation::RectRegion{
			3.54200f, 53.05600f,
			26.45800f, 8.05600f,
			time_to_timestamp(0h, 1min, 59s, centiseconds{ 4 })
		},
		background_color, // background color (RGBA)
		foreground_color, // foreground color (RGB)
		text_size, // text size
		"https://www.youtube.com/watch?annotation_id=annotation_671574&ei=hKMCXMeuJIG5Va7bkYAJ&feature=iv&src_vid=BckqqsJiDUI&v=yVebIlvkOnU"s, // url
		Annotation::Type::gameplay
	});

	check_annotation("annotations[4]", annotations[4], Annotation{
		"annotation_776505"s,
		go_here,
		Annotation::RectRegion{
			69.58300f, 51.66700f,
			27.29200f, 8.05600f,
			time_to_timestamp(0h, 1min, 45s, centiseconds{ 35 })
		},
		Annotation::RectRegion{
			69.58300f, 51.66700f,
			27.29200f, 8.05600f,
			time_to_timestamp(0h, 1min, 59s, centiseconds{ 4 })
		},
		background_color, // background color (RGBA)
		foreground_color, // foreground color (RGB)
		text_size, // text size
		"https://www.youtube.com/watch?annotation_id=annotation_776505&ei=hKMCXMeuJIG5Va7bkYAJ&feature=iv&src_vid=BckqqsJiDUI&v=MnBL8LY4kgc"s, // url
		Annotation::Type::gameplay
	});

	check_annotation("annotations[5]", annotations[5], Annotation{
		"annotation_782940"s,
		u8"English subtitles will be fully available on friday. Sorry for the inconvenience"s,
		Annotation::RectRegion{
			0.41700f, 75.83300f,
			52.08300f, 11.66700f,
			time_to_timestamp(0h, 1min, 7s, centiseconds{ 43 })
		},
		Annotation::RectRegion{
			0.41700f, 75.83300f,
			52.08300f, 11.66700f,
			time_to_timestamp(0h, 1min, 13s, centiseconds{ 40 })
		},
		background_color, // background color (RGBA)
		foreground_color, // foreground color (RGB)
		text_size, // text size
		missing_url, // url
		Annotation::Type::notes
	});

	check_annotation("annotations[6]", annotations[6], Annotation{
		"annotation_953980"s,
		go_here,
		Annotation::RectRegion{
			29.37500f, 39.44400f,
			27.50000f, 8.05600f,
			time_to_timestamp(0h, 1min, 45s, centiseconds{ 35 })
		},
		Annotation::RectRegion{
			29.37500f, 39.44400f,
			27.50000f, 8.05600f,
			time_to_timestamp(0h, 1min, 59s, centiseconds{ 4 })
		},
		background_color, // background color (RGBA)
		foreground_color, // foreground color (RGB)
		text_size, // text size
		"https://www.youtube.com/watch?annotation_id=annotation_953980&ei=hKMCXMeuJIG5Va7bkYAJ&feature=iv&src_vid=BckqqsJiDUI&v=5AkWHfJV8RQ"s, // url
		Annotation::Type::gameplay
	});
}


TEST_CASE("Can parse all of tube-adventures 1")
{
	int files_parsed = 0;

	for (const auto & file : std::filesystem::directory_iterator(tube_adventures_1_dir))
	{
		const ParseAnnotationsResult result = parse_annotations(file.path().u8string().c_str());
		++files_parsed;

		INFO("Filename: \"" + std::filesystem::relative(file.path(), tube_adventures_1_dir).u8string() + '"');
		CHECK(result.error == ParseAnnotationsError::success);
		CHECK(result.error_string == "");

		std::set<std::string_view> annotation_ids;
		for (const Annotation & annotation : result.annotations)
		{
			INFO("The annotation ID \"" + annotation.id + "\" should be unique");
			CHECK(annotation_ids.emplace(annotation.id).second);
		}
	}

	std::printf("%d files parsed in the tube adventures 1 directory\n", files_parsed);
}
