#include <catch2/catch.hpp>

#include "annotations.hh"

#if defined CATCH_CONFIG_RUNTIME_STATIC_REQUIRE // Should be constexpr but isn't in msvc
TEST_CASE("Can get video if from youtube URL")
{
	using namespace std::string_view_literals;

	STATIC_REQUIRE(youtube_video_id_from_url("https://www.youtube.com/watch?v=MnBL8LY4kgc"sv) == "MnBL8LY4kgc"sv);
	STATIC_REQUIRE(youtube_video_id_from_url("https://www.youtube.com/watch?annotation_id=annotation_671574&ei=hKMCXMeuJIG5Va7bkYAJ&feature=iv&src_vid=BckqqsJiDUI&v=yVebIlvkOnU"sv) == "yVebIlvkOnU"sv);
	STATIC_REQUIRE(youtube_video_id_from_url("https://www.youtube.com/watch?annotation_id=annotation_776505&ei=hKMCXMeuJIG5Va7bkYAJ&feature=iv&src_vid=BckqqsJiDUI&v=MnBL8LY4kgc"sv) == "MnBL8LY4kgc"sv);
}
#endif
