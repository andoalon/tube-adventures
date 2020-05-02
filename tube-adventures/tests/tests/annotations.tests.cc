#include <catch2/catch.hpp>

#include "annotations.hh"

#include <string_view>
#include <filesystem>
#include <set>

#include <QUrl>

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
	
	const u8string go_here(u8"Ir aqu\u00ED / go here"sv);
	const u8string missing_url = {};

	const QColor foreground_color = QColor::fromRgb(1710618);
	const QColor background_color = unsigned_rgb_and_alpha_float(16777215, 0.8f);

	constexpr float text_size = 3.6107f;

	check_annotation("annotations[0]", annotations[0], Annotation{
		"annotation_103323"s,
		u8"M\u00FAsica y + info en www.tube-adventures.blogspot.com"s,
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

TEST_CASE("Can get full youtube url from video ID")
{
	const auto result = full_youtube_url_from_id("BckqqsJiDUI"sv);

	constexpr std::string_view expected = "https://www.youtube.com/watch?v=BckqqsJiDUI"sv;

	REQUIRE(result.has_value());
	CHECK(*result == expected);
	CHECK(QUrl(QString::fromStdString(*result)).isValid());
}

TEST_CASE("Can extract youtube video ID from annotation file path")
{
	struct Test
	{
		std::filesystem::path path;
		std::string expected_id;
	};

	SECTION("Success")
	{
		const Test tests[] = {
			{ "TA01 yVebIlvkOnU.xml", "yVebIlvkOnU" },
			{ "TA02 5AkWHfJV8RQ.xml", "5AkWHfJV8RQ" },
			{ "TA03 Nz3OeyRyUfE.xml", "Nz3OeyRyUfE" },
			{ "TA04 MnBL8LY4kgc.xml", "MnBL8LY4kgc" },
			{ "TA05 ccmNrLmG-6U.xml", "ccmNrLmG-6U" },
			{ "TA06 SQ4VCOT5w-w.xml", "SQ4VCOT5w-w" },
			{ "TA07 5JQBACKTLO8.xml", "5JQBACKTLO8" },
			{ "TA08 LyGdkID_EOg.xml", "LyGdkID_EOg" },
			{ "TA09 _EIxIlpqRio.xml", "_EIxIlpqRio" },
			{ "TA10 KdSImTQRQEA.xml", "KdSImTQRQEA" },
			{ "TA11 xizooMsqv5s.xml", "xizooMsqv5s" },
			{ "TA12 bKPrAyI95dM.xml", "bKPrAyI95dM" },
			{ "TA13 bvH-4uA_9-Q.xml", "bvH-4uA_9-Q" },
			{ "TA14 esNIoSleZqA.xml", "esNIoSleZqA" },
			{ "TA15 texlhDSgVRc.xml", "texlhDSgVRc" },
			{ "TA16 FMAjPmAmYYY.xml", "FMAjPmAmYYY" },
			{ "TA17 1w0GG4LO9Xs.xml", "1w0GG4LO9Xs" },
			{ "TA18 UwudycbD3oo.xml", "UwudycbD3oo" },
			{ "TA19 dJLI81Ydo84.xml", "dJLI81Ydo84" },
			{ "TA20 KiTQRP1KK0U.xml", "KiTQRP1KK0U" },
			{ "TA21 AVf-W6MkqA0.xml", "AVf-W6MkqA0" },
			{ "TA22 5j4gwujTVjg.xml", "5j4gwujTVjg" },
			{ "TA23 iTQK5q5RoUw.xml", "iTQK5q5RoUw" },
			{ "TA24 sncgDs4YBpo.xml", "sncgDs4YBpo" },
			{ "TA25 akGnLUy6aWU.xml", "akGnLUy6aWU" },
			{ "TA26 bQltg0Hwq84.xml", "bQltg0Hwq84" },
			{ "TA27 bPUSj_brL6w.xml", "bPUSj_brL6w" },
			{ "TA28 RpIRRZjL40g.xml", "RpIRRZjL40g" },
			{ "TA29 QqCFr0w2Z18.xml", "QqCFr0w2Z18" },
			{ "TA30 rlZRFsPpCyQ.xml", "rlZRFsPpCyQ" },
			{ "TA31 1nIr4OsRGvM.xml", "1nIr4OsRGvM" },
			{ "TA32 ZYgBT9_Oi54.xml", "ZYgBT9_Oi54" },
			{ "TA33 FzD4oiaYAZQ.xml", "FzD4oiaYAZQ" },
			{ "TA34 w0TWWHDSdFE.xml", "w0TWWHDSdFE" },
			{ "TA35 -3h0wRZq_1I.xml", "-3h0wRZq_1I" },
			{ "TA36 -BdkyO3SgJA.xml", "-BdkyO3SgJA" },
			{ "TA37 5AtSNnxBxE4.xml", "5AtSNnxBxE4" },
			{ "TA38 eOz7LM6DYRM.xml", "eOz7LM6DYRM" },
			{ "TA39 05TL_bQF0os.xml", "05TL_bQF0os" },
			{ "TA40 GcPZcM7qYQQ.xml", "GcPZcM7qYQQ" },
			{ "TA41 kEvQU8A2veo.xml", "kEvQU8A2veo" },
			{ "TA42 AVDMgPeEHpU.xml", "AVDMgPeEHpU" },
			{ "TA43 4t0j7xOF0os.xml", "4t0j7xOF0os" },
			{ "TA44 hf9bnHws0X8.xml", "hf9bnHws0X8" },
			{ "TA45 JErgINI4lMg.xml", "JErgINI4lMg" },
			{ "TA46 HNgX-S85cuc.xml", "HNgX-S85cuc" },
			{ "TA47 u4i5OTVZxvE.xml", "u4i5OTVZxvE" },
			{ "TA48 XxYu8eAqC5U.xml", "XxYu8eAqC5U" },
			{ "TA49 OLc9gKhKADg.xml", "OLc9gKhKADg" },
			{ "TA50 ZciilZf1spU.xml", "ZciilZf1spU" },
			{ "TA51 AAA_NxhT-Zw.xml", "AAA_NxhT-Zw" },
			{ "TA52 pHT-afoqzt8.xml", "pHT-afoqzt8" },
			{ "TA53 Ow_ssg5gTgo.xml", "Ow_ssg5gTgo" },
			{ "TA54 ONN-aTxrhck.xml", "ONN-aTxrhck" },
			{ "TA55 Y7gaVAHxzE0.xml", "Y7gaVAHxzE0" },
			{ "TA56 x7rfiStC_rg.xml", "x7rfiStC_rg" },
			{ "TA57 q01JoAIS0Ww.xml", "q01JoAIS0Ww" },
			{ "TA58 T6GxHvVIbhk.xml", "T6GxHvVIbhk" },
			{ "TA59 pO-oIZbQnqQ.xml", "pO-oIZbQnqQ" },
			{ "TA60 c0CtvZKz2zw.xml", "c0CtvZKz2zw" },
			{ "TA61 F87N3uM33p0.xml", "F87N3uM33p0" },
			{ "TA62 x6V9YYEVPpc.xml", "x6V9YYEVPpc" },
			{ "TA63 DY1YtV9-Z1c.xml", "DY1YtV9-Z1c" },
			{ "TA64 -wtvbB-moOE.xml", "-wtvbB-moOE" },
			{ "TA65 IDD8kv-VnaI.xml", "IDD8kv-VnaI" },
			{ "TA66 nEcIafhGKh8.xml", "nEcIafhGKh8" },
			{ "TA67 mMrZT2GzeKA.xml", "mMrZT2GzeKA" },
			{ "TUBE-ADVENTURES (aventura interactiva) BckqqsJiDUI.xml", "BckqqsJiDUI" }
		};

		for (const Test & test : tests)
		{
			const auto result = path_to_youtube_video_id(test.path, annotation_file_extension);
			CHECK(result == test.expected_id);
		}
	}

	// TODO: check failing cases
}
