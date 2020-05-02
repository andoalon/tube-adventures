#include "ui_mainwindow.h"

#include "mainwindow.hh"

#include <map>
#include <cassert>

#include <QGuiApplication>
#include <QMessageBox>

using namespace std::chrono_literals;

namespace
{
	using video_position = std::chrono::duration<qint64, std::milli>;

	[[nodiscard]] video_position get_video_position(const QMediaPlayer & player)
	{
		return video_position(player.position());
	}

	void set_video_position(QMediaPlayer & player, const video_position position)
	{
		if (!player.isSeekable())
		{
			qDebug() << "Can't change video position because the player is not seekable";
		}

		const auto max_position = video_position(player.duration());
		const auto final_position = std::clamp(position, video_position(0), std::max(video_position(0), max_position - 1s));

		player.setPosition(final_position.count());
	}

	void change_video_position(QMediaPlayer & player, const video_position position_change)
	{
		set_video_position(player, get_video_position(player) + position_change);
	}

	[[nodiscard]] video_position video_seek_duration()
	{
		const Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();

		switch (modifiers)
		{
		default:
		case Qt::KeyboardModifier::NoModifier:
			return 10s;
		case Qt::KeyboardModifier::ControlModifier | Qt::KeyboardModifier::ShiftModifier:
			return 0s;
		case Qt::KeyboardModifier::ShiftModifier:
			return 3s;
		case Qt::KeyboardModifier::ControlModifier:
			return 1min;
		}
	}

	[[noreturn]] void unreachable(const char* message) noexcept
	{
		std::fprintf(stderr, "Reached unreachable code: %s\n", message);
		std::fflush(stderr);

		std::abort();
	}

	// Returns empty path if not found
	[[nodiscard]] std::filesystem::path find_path_with_youtube_id(const std::string_view youtube_id, const std::filesystem::path & search_directory, const std::filesystem::path & expected_extension) try
	{
		const auto end_it = std::filesystem::directory_iterator{};
		const auto path_it = std::find_if(std::filesystem::directory_iterator(search_directory), end_it, [youtube_id, &expected_extension](const std::filesystem::directory_entry & entry)
		{
			if (const auto id = path_to_youtube_video_id(entry.path(), expected_extension); id.has_value())
				return *id == youtube_id;

			return false;
		});

		if (path_it == end_it)
			return {};

		return path_it->path();
	}
	catch (const std::filesystem::filesystem_error &)
	{
		return {};
	}
	

	// Returns empty url if failed
	[[nodiscard]] QUrl video_url_from_youtube_id(const std::string_view youtube_id)
	{
		using namespace std::string_view_literals;

		static const std::map<std::string_view, std::string_view> youtube_id_to_direct_video_url = {
			/* TA00 */ { "BckqqsJiDUI"sv, "https://r4---sn-4g5edns6.googlevideo.com/videoplayback?expire=1588457763&ei=w5ytXvn0AdfngAf2oazQAQ&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AEc4CYdvOKVunkkurF-IhAOdEFVLIQuJVJnUmZU1YBsy&itag=18&source=youtube&requiressl=yes&mh=Vu&mm=31%2C29&mn=sn-4g5edns6%2Csn-h0jeened&ms=au%2Crdu&mv=m&mvi=3&pl=33&initcwndbps=2133750&vprv=1&mime=video%2Fmp4&gir=yes&clen=5695831&ratebypass=yes&dur=119.211&lmt=1390721598884458&mt=1588436071&fvip=4&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIgUNoMMvDSEp-gPb2VrhsekrytCoVhhe5QQ6OVvtWzZWgCIQDMLk5WWl2LMbgYZktKgxVhwtBJGf7sXl-vC4d5xE_8fw%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIgPNPelO7c14ozTglBbPVmUETUipuPJbduEH5WWW-t8PMCIQDb_und3L2cBJTh4OcDNF-ag27D1bkGWs_91PfQ4U2J1w%3D%3D" },
			/* TA01 */ { "yVebIlvkOnU"sv, "https://r2---sn-4g5e6nzl.googlevideo.com/videoplayback?expire=1588462790&ei=ZbCtXpX1O4SNgQflo6nQDw&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-ANXwIQaOOjXG3DEqi_6FABK8vbPRbISLw9k3iaRyQ1wm&itag=18&source=youtube&requiressl=yes&mh=kU&mm=31%2C29&mn=sn-4g5e6nzl%2Csn-h0jeen7d&ms=au%2Crdu&mv=m&mvi=1&pl=33&initcwndbps=2013750&vprv=1&mime=video%2Fmp4&gir=yes&clen=3698756&ratebypass=yes&dur=65.201&lmt=1392027277614062&mt=1588441111&fvip=2&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRAIgfSgXvpis6TdLH-6r4-AZjXhRNu_87EdUOxdVVbEHxksCIHPdE8kLRuDa1LUzsCzhlGy1w_z456SWCUqs50Nd0IVI&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRAIgFQj4oMx2c7VJRlhZDm28eFsQBV6VqAoIv3xlFyNOf3gCIEpY6EqsGU5SBTz52X5LIrQ1enjGvGS0xOkCh95ZeICN"sv },
			/* TA02 */ { "5AkWHfJV8RQ"sv, "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?expire=1588462858&ei=qrCtXvOcD8e7gAedtbiQCg&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AKckfnijWDaKn2VIFpIuah_elypPyNZK3J35oICR8K2X&itag=18&source=youtube&requiressl=yes&mh=2z&mm=31%2C29&mn=sn-4g5ednld%2Csn-h0jeen76&ms=au%2Crdu&mv=m&mvi=3&pl=33&initcwndbps=2060000&vprv=1&mime=video%2Fmp4&gir=yes&clen=2650511&ratebypass=yes&dur=42.144&lmt=1404251359161338&mt=1588441171&fvip=4&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIgD3zFhPHEKseGz-91g3KcrByovviGPRh08TBH9y5N_M0CIQCWZuOo1ozbdosbYCH58qPGn1dgj0wJxPVEeGY_qfbaxg%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIhAJTaoEbw1XygQe4eE7vnJBva4ia7K2L8uxKNahX5b9zJAiBxZqg18XSf9Rp5g0Qz99JgR4HOM47u4hXA9ladHbAbUg%3D%3D"sv },
			/* TA03 */ { "Nz3OeyRyUfE"sv, "https://r2---sn-4g5e6nzl.googlevideo.com/videoplayback?expire=1588462790&ei=ZbCtXpX1O4SNgQflo6nQDw&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-ANXwIQaOOjXG3DEqi_6FABK8vbPRbISLw9k3iaRyQ1wm&itag=18&source=youtube&requiressl=yes&mh=kU&mm=31%2C29&mn=sn-4g5e6nzl%2Csn-h0jeen7d&ms=au%2Crdu&mv=m&mvi=1&pl=33&initcwndbps=2013750&vprv=1&mime=video%2Fmp4&gir=yes&clen=3698756&ratebypass=yes&dur=65.201&lmt=1392027277614062&mt=1588441111&fvip=2&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRAIgfSgXvpis6TdLH-6r4-AZjXhRNu_87EdUOxdVVbEHxksCIHPdE8kLRuDa1LUzsCzhlGy1w_z456SWCUqs50Nd0IVI&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRAIgFQj4oMx2c7VJRlhZDm28eFsQBV6VqAoIv3xlFyNOf3gCIEpY6EqsGU5SBTz52X5LIrQ1enjGvGS0xOkCh95ZeICN"sv },
			/* TA04 */ { "MnBL8LY4kgc"sv, "https://r2---sn-4g5ednle.googlevideo.com/videoplayback?expire=1588464212&ei=9LWtXvW_BI_l1wKft7rIAg&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AGdyDkAEul4wX3N0rwfoCiWY--C1g7W6fyk89INPOU50&itag=18&source=youtube&requiressl=yes&mh=Fk&mm=31%2C29&mn=sn-4g5ednle%2Csn-h0jeln7e&ms=au%2Crdu&mv=m&mvi=1&pl=33&initcwndbps=1961250&vprv=1&mime=video%2Fmp4&gir=yes&clen=1588565&ratebypass=yes&dur=31.346&lmt=1409880938300084&mt=1588442492&fvip=2&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIhAOd6Hg-qhNT7swUwvQkCzsjcpnoPQ_LQaau5ziSy0IP-AiBi-to4_DvOv_wxBjjyCmT_TSqXDiAx0SeX7i94v2oC_w%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIgatAIOO1RhVQ6bMFA4HVKr_9tqDJQGZYxRaIHW4nqPjICIQDWqqTirJoR9QbLRvXyZeFrDAzhWorXq1mzYTcjzRd8ZA%3D%3D"sv },
			/* TA05 */ { "ccmNrLmG-6U"sv, "https://r1---sn-4g5ednsz.googlevideo.com/videoplayback?expire=1588464215&ei=9rWtXoD0PNCh8gOes4_4CA&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AH2SUPKTC-Lx45GKw6R4CLZVRWcN04qFYK9gvG68-COQ&itag=18&source=youtube&requiressl=yes&mh=Yy&mm=31%2C29&mn=sn-4g5ednsz%2Csn-h0jeener&ms=au%2Crdu&mv=m&mvi=0&pl=33&initcwndbps=2016250&vprv=1&mime=video%2Fmp4&gir=yes&clen=1932273&ratebypass=yes&dur=36.223&lmt=1394274142519230&mt=1588442492&fvip=1&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRgIhAOWMHGBhSan81o16PWpm0FZSmK-UN928VpnMh3q6_3j1AiEArPuTztPW5pyuu8UddzL7AyLEiKI8D753ykQyV-fEKec%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRgIhAJeqLpkhk1NPjC8lmL57Szu6M1MFvFMLT2p9gN6a9wbnAiEAru-ubKkrRD1W2dleVrknwx8WB47Qr5Sw_zfHS6fH6wg%3D"sv },
			/* TA06 */ { "SQ4VCOT5w-w"sv, "https://r1---sn-4g5e6ns7.googlevideo.com/videoplayback?expire=1588464655&ei=r7etXvD-JoGO1gK4uZfABw&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AHerG6b32oGVXl26jvV6qBEbnZfrzwQ5He4AjVklXEWw&itag=18&source=youtube&requiressl=yes&mh=KC&mm=31%2C29&mn=sn-4g5e6ns7%2Csn-h0jeen76&ms=au%2Crdu&mv=m&mvi=0&pl=33&initcwndbps=2043750&vprv=1&mime=video%2Fmp4&gir=yes&clen=2116844&ratebypass=yes&dur=31.672&lmt=1216915804710768&mt=1588442971&fvip=1&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIhAJjSewhN4bwDonYumak2t8-XzTA7VLW2cKAfCrfKIRpaAiA5poDcD9dlgVx59cvxCAr69n8sM1N0dvf_fo0wFc2XFg%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIgRmC3NC5fAdjnm5y0OCxQzpIU46l-cVybmgJyDBayW8sCIQClNtDeZPHLDV4q6W_-9TUElmhJzJ0C5q8C8nZ6Ax9Tpw%3D%3D"sv },
			/* TA07 */ { "5JQBACKTLO8"sv, "https://r3---sn-4g5ednsz.googlevideo.com/videoplayback?expire=1588464671&ei=v7etXoLHLZOwgAeNwImACQ&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AEn5c6fr6kx09KHIQVWVyoTaqjWbstyuX7eo8K1eBQJ3&itag=18&source=youtube&requiressl=yes&mh=hF&mm=31%2C29&mn=sn-4g5ednsz%2Csn-h0jelne7&ms=au%2Crdu&mv=m&mvi=2&pl=33&initcwndbps=2043750&vprv=1&mime=video%2Fmp4&gir=yes&clen=2188108&ratebypass=yes&dur=33.506&lmt=1389656477072038&mt=1588442971&fvip=3&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIgH5tPude6O3MhgEIIC2LxAsg9jt8e7JItKgVlcPMO5aUCIQD6A1Alu4mmEamzly6wRTyFmRhY3rzbbU3r6Y-OEDQm4Q%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIgXPvcCPOYJ-W6hy49GLpZ9734Ee6pzmkY40Q63FDh-nMCIQD0ruV0iHpakuP2vn-PG8qtpEOKfIX0ceGn03_XeUJNAA%3D%3D"sv },
			/* TA08 */ { "LyGdkID_EOg"sv, "https://r6---sn-4g5e6nl7.googlevideo.com/videoplayback?expire=1588464691&ei=07etXsnzAsq71gKzrISIAg&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AAdC3Ej-dtGxVmQyB8NMtJYCC3Y3-LHLIBj-1gTx4ae0&itag=18&source=youtube&requiressl=yes&mh=oj&mm=31%2C29&mn=sn-4g5e6nl7%2Csn-h0jeln7l&ms=au%2Crdu&mv=m&mvi=5&pl=33&initcwndbps=2042500&vprv=1&mime=video%2Fmp4&gir=yes&clen=3063709&ratebypass=yes&dur=52.895&lmt=1410485715959311&mt=1588443028&fvip=2&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIhALQjuUh8iFP6_hGmKnM073v3DtF5auE_pjY0BKzoV_ibAiB_6Nsh5F0SoNgMvEKe0W0CzDLr2lNjJBjEFGdsIjSFZQ%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIhAIONkNthTfSzwL0QnwoH8XhIdONlDiP_8llK3coMoTD3AiAc7jBAEXx0TFCiaXZOa6piUXni3Y6c-FZzY7fP1d0CIQ%3D%3D"sv },
			/* TA09 */ { "_EIxIlpqRio"sv, "https://r1---sn-4g5edne7.googlevideo.com/videoplayback?expire=1588464716&ei=7LetXprEIIfvgAeI2L2YAw&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AK_KTwG6SvKYkZ9bnwy9AKZuL-rcZgMRyC9pO1RRin_4&itag=18&source=youtube&requiressl=yes&mh=xT&mm=31%2C29&mn=sn-4g5edne7%2Csn-h0jeenek&ms=au%2Crdu&mv=m&mvi=0&pl=33&initcwndbps=1905000&vprv=1&mime=video%2Fmp4&gir=yes&clen=3286697&ratebypass=yes&dur=47.020&lmt=1393579468208547&mt=1588443028&fvip=1&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIgdu0k8bL9h_BgZsISQCaU3RyhIutaIlirIYboJDmRYDMCIQC6d-K0duMb8E7r0F7YMTacIJLTUDV0DHxFV8ZkaVIHAQ%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRAIgBRD2P9RFJw3WpPg2t5Y1gdTZC_cB1bMDFfAlzCyQq5ICIFf4oj5pwS-7qTl7ttevcXnp9RQfvSsEr6klEqF0PA3K"sv },
			/* TA10 */ { "KdSImTQRQEA"sv, "https://r2---sn-4g5ednsk.googlevideo.com/videoplayback?expire=1588464728&ei=-LetXuSpIsGE6dsP1_-KiA0&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-ANJKiWqhVkYWzsu7QeklaiSSYBKTFXOPv746cNv0CZu9&itag=18&source=youtube&requiressl=yes&mh=2c&mm=31%2C29&mn=sn-4g5ednsk%2Csn-h0jelne7&ms=au%2Crdu&mv=m&mvi=1&pl=33&initcwndbps=2042500&vprv=1&mime=video%2Fmp4&gir=yes&clen=2527916&ratebypass=yes&dur=37.755&lmt=1216925531834654&mt=1588443028&fvip=2&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIhAIj-TSsBadzw4Akq24RA_nQAs2ENN7eRTTbmyu9bbAZQAiB293Mloq1FpO7gfiD-OQo1_Qa0-kKjbWxs58TXNOB3aQ%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRgIhAI3V6RrHLVLnpJnCrRnu4XhZiGlIyYUOVfh7iW89ZAT-AiEAs1xaP3uMu3BxyhLUdPzu4ECxNkNlpsZLITM7JCQVKUA%3D"sv },
			/* TA11 */ { "xizooMsqv5s"sv, "https://r4---sn-4g5e6nsk.googlevideo.com/videoplayback?expire=1588464771&ei=I7itXvuSAsCEx_APsMak6AM&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AI05_MGUd7jZMwUEEco_hjNRvwrIhFS2Br_c3sVJtkTY&itag=18&source=youtube&requiressl=yes&mh=EN&mm=31%2C29&mn=sn-4g5e6nsk%2Csn-h0jeln7k&ms=au%2Crdu&mv=m&mvi=3&pl=33&initcwndbps=2026250&vprv=1&mime=video%2Fmp4&gir=yes&clen=3339428&ratebypass=yes&dur=49.667&lmt=1216998077049766&mt=1588443092&fvip=4&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIgCck_cPvHqz8s-kxbL2gh7-w54GeUGipAnLdHnhGPTUACIQDGIR7lQbvl7qhJRoz88EDbsBpa-PeWUIZ5PqSO9j2gZg%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIgCDJa6c01hsBV0BlFUk9Bn4qw1PqgOa84xUdxmowdBr4CIQC-nh73MVEEqkIiYq4xHpM6_vPzfAcQudujPdfl1vJcuQ%3D%3D"sv },
			/* TA12 */ { "bKPrAyI95dM"sv, "https://r5---sn-4g5ednsd.googlevideo.com/videoplayback?expire=1588464787&ei=M7itXsGWGJWRgAeg64mADQ&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AClcCqzxF89EOUc03TwSwMIAYHkyEwaZV7WKUexTHI-y&itag=18&source=youtube&requiressl=yes&mh=uE&mm=31%2C29&mn=sn-4g5ednsd%2Csn-h0jeened&ms=au%2Crdu&mv=m&mvi=4&pl=33&initcwndbps=1992500&vprv=1&mime=video%2Fmp4&gir=yes&clen=3658324&ratebypass=yes&dur=50.062&lmt=1242362519339270&mt=1588443092&fvip=5&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIgUVoWLIoEgvqHKO_9yUTUrf4g5FT_qfYlhvmQPeEuflkCIQCD0mCpteY3F5jybGKhswRkc6of03YMQxfQXl6Vnk6ilQ%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRgIhALGdINznimIqfe9-eiaGyzDhO6V-WB84vjJiesHw6fNoAiEAgCGwwTmMJmdWg0sMyIQG5rUjwBqBBuG05jur19aC364%3D"sv },
			/* TA13 */ { "bvH-4uA_9-Q"sv, "https://r4---sn-4g5e6nzs.googlevideo.com/videoplayback?expire=1588464717&ei=7betXoDbBYGO1gLi3ZvwCw&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AGfqw6X-ulMDP8eNZHgbPOKF1l5julZXs9fX88nqXaln&itag=18&source=youtube&requiressl=yes&mh=0V&mm=31%2C29&mn=sn-4g5e6nzs%2Csn-h0jeln7e&ms=au%2Crdu&mv=m&mvi=3&pl=33&initcwndbps=2090000&vprv=1&mime=video%2Fmp4&gir=yes&clen=7087028&ratebypass=yes&dur=90.093&lmt=1394513451594361&mt=1588443028&fvip=4&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIgI1vgpJYBV_a2cPEC7cFxDzSXInwR3BW1uh2Eos7SXXkCIQDSr4AK3N5-r1VRYWR_cjXGI-tU1xqMHr_plFSrUuYhtQ%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRgIhAMXZxiOwBlaVouwGyibn6TsuhSZC4kYafpRMbYZwaY6qAiEApkqf7rQuuRjM8Ieq0WtuRs3pIBq5OapaiHzDrzYX_dU%3D"sv },
			/* TA14 */ { "esNIoSleZqA"sv, "https://r4---sn-4g5ednsy.googlevideo.com/videoplayback?expire=1588464206&ei=7rWtXvb9HoaF1gLNw4C4Ag&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AGm0WyFB8ObamcyTKRiw7zJa-BU5JPivuvm2xLdK2q6Y&itag=18&source=youtube&requiressl=yes&mh=om&mm=31%2C29&mn=sn-4g5ednsy%2Csn-h0jeln7l&ms=au%2Crdu&mv=m&mvi=3&pl=33&initcwndbps=2040000&vprv=1&mime=video%2Fmp4&gir=yes&clen=6115708&ratebypass=yes&dur=90.720&lmt=1217012041917937&mt=1588442492&fvip=4&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIgZv7Ybv5HaCJqGOsnogDMIqeNCSaKtDnccwfSticqOBsCIQDTkEM7CLcgpi9CCJplLTWXxoJwHOTHWTD7uYoNBWZxTg%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIgc0NJ5w4vipjeMwFA8fAE9kS427o4gG4bao4NA80E_1cCIQCQ1avL7XKuvCT2mDCWw6jqi8bEcUz24yQ8ShbXmAqzJQ%3D%3D"sv },
			/* TA15 */ { "texlhDSgVRc"sv, "https://r3---sn-4g5e6ns6.googlevideo.com/videoplayback?expire=1588464209&ei=8bWtXrHgBZj-gQfD-aHAAQ&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-ACmY2FWCc9qstYM9fufeeaWKT3P0z8KgYkh1lXj9giRy&itag=18&source=youtube&requiressl=yes&mh=SG&mm=31%2C29&mn=sn-4g5e6ns6%2Csn-h0jeenek&ms=au%2Crdu&mv=m&mvi=2&pl=33&initcwndbps=2040000&vprv=1&mime=video%2Fmp4&gir=yes&clen=3380279&ratebypass=yes&dur=43.490&lmt=1390479304791581&mt=1588442492&fvip=3&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRgIhAMrTvURQl7hHTweVBxzz4aM91gA30e2gYOi649abeMj4AiEAv6PqC1s0xbH9-Rgmb8xaEg5aFS874EGI8n5u_qZRaeI%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRgIhAP0CuPN8gvdi1Mfg_BlvN8eN9WwBx410VrTzq5U4z6K5AiEA3mtGCpRvr8CdFwnIhY8xnkDyXsbRj-tcxneR8HycMSk%3D"sv },
			/* TA16 */ { "FMAjPmAmYYY"sv, "https://r2---sn-4g5ednss.googlevideo.com/videoplayback?expire=1588464853&ei=dbitXu6mNOX57gPq85ngCw&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AClAV0hskPGljqwg-QQxR0ULFql_UJu6j6tuaYSdHTZ2&itag=18&source=youtube&requiressl=yes&mh=AT&mm=31%2C29&mn=sn-4g5ednss%2Csn-h0jelne7&ms=au%2Crdu&mv=m&mvi=1&pl=33&initcwndbps=1992500&vprv=1&mime=video%2Fmp4&gir=yes&clen=2903642&ratebypass=yes&dur=36.223&lmt=1394992826751938&mt=1588443150&fvip=2&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIgSDFUSHRfkDoJ22Rr9U9l9nnET6KAawnNd_sp9fFZkNMCIQC0IzeZIoE_4GsEpMYktqqi2UdgqLMHCWtzOaRAvtj4Jw%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIhAK_H2qycmbAdAMsdZfOuG2-2DpO-oQ-aZDwkdWrK5OWsAiAeZ3d-wh1Ibkw_vlSh-tYHZ0R4cfhywL0tAiDhvDttYA%3D%3D"sv },
			/* TA17 */ { "1w0GG4LO9Xs"sv, "https://r2---sn-4g5e6nzl.googlevideo.com/videoplayback?expire=1588464208&ei=77WtXrDBOZKQ-gaHkrGACQ&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-ACfYmnIMkRYTZk68Dcae2qJpzEFXjDBXJlTwN9zH53hp&itag=18&source=youtube&requiressl=yes&mh=1K&mm=31%2C29&mn=sn-4g5e6nzl%2Csn-h0jeln7y&ms=au%2Crdu&mv=m&mvi=1&pl=33&initcwndbps=2040000&vprv=1&mime=video%2Fmp4&gir=yes&clen=4343531&ratebypass=yes&dur=59.698&lmt=1552673597861216&mt=1588442552&fvip=2&c=WEB&txp=5431432&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRgIhAOG89oGTh7jqSkTHLKAH4Yll5g7OSvIJ85Hy_CkOGkFqAiEA8HnwHi2sDQqbrVTRYJFPatdzYj7R-ryrtzD17Mc3mwM%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRgIhAMQYKe-XiFXWLs2G7DI893d8LlxSk5BPwBfbD7xtH6V1AiEAwMX9CltsCo4diJjXOD8dBY4jLsa7Atwc_HcBMbAUr_0%3D"sv },
			/* TA18 */ { "UwudycbD3oo"sv, "https://r3---sn-4g5e6nz7.googlevideo.com/videoplayback?expire=1588464905&ei=qbitXqW_OJKQ-ga0raTAAg&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AO-FMJXiycOuyVexTnwcUgQTprEXcjHm8ppSuSjnVR6C&itag=18&source=youtube&requiressl=yes&mh=UT&mm=31%2C29&mn=sn-4g5e6nz7%2Csn-h0jeen7d&ms=au%2Crdu&mv=m&mvi=2&pl=33&initcwndbps=1982500&vprv=1&mime=video%2Fmp4&gir=yes&clen=4341905&ratebypass=yes&dur=59.698&lmt=1552618959271937&mt=1588443211&fvip=3&c=WEB&txp=5431432&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIhAKZWUKi9pq3-ddVqN5tYhX_AiKc2BisttaYQJkFT26wpAiADtZCuO9a81i8_saEln2FP6yZ4epO5qyqrdbe-ypepyA%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRAIgBJg-BodGTFUOlyKhPGhVmgCNLZVM7sY45sNUeXfqnyoCIBFTZZ04irKg0Og-Hm8fsa-sucIibvA4YnKX3OvgMtjF"sv },
			/* TA19 */ { "dJLI81Ydo84"sv, "https://r5---sn-4g5e6nz7.googlevideo.com/videoplayback?expire=1588464854&ei=dritXq7SM8-bgQefjpho&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-ANVBrtsFHA95HSdzCPimZCIseoeO-jGHxn8NRsAVJZ7K&itag=18&source=youtube&requiressl=yes&mh=mU&mm=31%2C29&mn=sn-4g5e6nz7%2Csn-h0jeln7y&ms=au%2Crdu&mv=m&mvi=4&pl=33&initcwndbps=1992500&vprv=1&mime=video%2Fmp4&gir=yes&clen=4505434&ratebypass=yes&dur=59.698&lmt=1386955718924667&mt=1588443150&fvip=5&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIgXt2I5F2CMcAO2fmK5xQK_gbpplI1_ZtIV22LQx5YGWwCIQDByVqySziNwEd4g0X29Chywnb9fG5AMqkB51Ag5_Y4rQ%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRAIgFLGst8q15qaEvyQaoyEOJ4qypKdxDswBR6ReXQ07RAACIGg0jay0zJ_Q36ib9C-Vd01vfIl-a4di5_1aGRxxm925"sv },
			/* TA20 */ { "KiTQRP1KK0U"sv, "https://r3---sn-4g5ednz7.googlevideo.com/videoplayback?expire=1588464932&ei=xLitXo65E4qcgQe7tZfoAw&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-APOaHN5advOu-7K51n_V7zxFqLSGmefX22ZJFtz7WsYz&itag=18&source=youtube&requiressl=yes&mh=Xi&mm=31%2C29&mn=sn-4g5ednz7%2Csn-h0jeln7l&ms=au%2Crdu&mv=m&mvi=2&pl=33&initcwndbps=2062500&vprv=1&mime=video%2Fmp4&gir=yes&clen=4499887&ratebypass=yes&dur=59.698&lmt=1418482401778346&mt=1588443211&fvip=3&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRgIhALrZBHFdKpNRewfY-8A37MoiSx3vEv7Jc7RiAjjlqy4_AiEA-3FsVA4rsHc3gha-KtwNz9cITG0K8Qb5Pp_CE4myncs%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIgc71pC304RgrrBhB5zmKcYIcPjy9hefZejVY_8q5osu8CIQDhuBqDWb80n3LKpzzrRvCJQejJFI7798W5knOPuknXQw%3D%3D"sv },
			/* TA21 */ { "AVf-W6MkqA0"sv, "https://r6---sn-4g5ednee.googlevideo.com/videoplayback?expire=1588464691&ei=07etXo66L_f57gOcyrmIAQ&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AI657Ys9-hJZRiCsSm27TnQRjKSQ6Z8FGkPVB_M--oRH&itag=18&source=youtube&requiressl=yes&mh=7E&mm=31%2C29&mn=sn-4g5ednee%2Csn-h0jelne7&ms=au%2Crdu&mv=m&mvi=5&pl=33&initcwndbps=2090000&vprv=1&mime=video%2Fmp4&gir=yes&clen=3978575&ratebypass=yes&dur=59.071&lmt=1217020211601989&mt=1588442971&fvip=3&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRgIhAK5QpDWwEYzAOMFjut9cFUVbFHslc3HJaFvEWVvOnLNJAiEAuL2kcdDmuxPwhGZWHLrag8_o85ksouI1Jxeu-8JjanA%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRgIhAOAOe9_aFK-vRjdq3hxWdYmT9iktbH7PhtI6zNA6CoWkAiEAskbS_3YVUs94AO8R-xEfugzrnbZQXWgaGwxT3IpmLEo%3D"sv },
			/* TA22 */ { "5j4gwujTVjg"sv, "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?expire=1588464673&ei=wbetXuqYFY6BgQeJ8LSwAw&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AFahTdl7e9UuJHWJQNcZ4wHkmYTIHjESFCjjcpiMpMk1&itag=18&source=youtube&requiressl=yes&mh=no&mm=31%2C29&mn=sn-4g5ednld%2Csn-h0jeln7l&ms=au%2Crdu&mv=m&mvi=3&pl=33&initcwndbps=1905000&vprv=1&mime=video%2Fmp4&gir=yes&clen=4081904&ratebypass=yes&dur=58.700&lmt=1391527354563305&mt=1588442971&fvip=4&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIhAMT4y_ZZHTHoLYgDgY7StBJGy3vxAu8bMXpFbSOjFL3nAiASXXhtAprcSeeL_fjx9MvcoC9ZChCufmwh_tvPltn8OA%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIgIndmsAaaA0mwtMIMkWk7HPu2Ux2NbhdF0ZrcFKvc2i8CIQCRNriUbWLRrNDfP4w5I0JXtUPkwnaZHtvSk4DcX6PH4g%3D%3D"sv },
			/* TA23 */ { "iTQK5q5RoUw"sv, "https://r3---sn-4g5ednsk.googlevideo.com/videoplayback?expire=1588464657&ei=sbetXrPsF4qcgQfok5KQCw&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-ADCJanOz3eYTi_DuriGCruyc5P0-uAaHRodGpgB-VE08&itag=18&source=youtube&requiressl=yes&mh=Np&mm=31%2C29&mn=sn-4g5ednsk%2Csn-h0jeln7e&ms=au%2Crdu&mv=m&mvi=2&pl=33&initcwndbps=2090000&vprv=1&mime=video%2Fmp4&gir=yes&clen=2207540&ratebypass=yes&dur=32.995&lmt=1217021633092506&mt=1588442971&fvip=3&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRgIhAKIfoXgV4pcZTiEYZNncoK-cOyKIB9uZUlrXoTlV1tcoAiEA3q0SoFy0ke4FtKavcY0T6m2M0GSvvuKHBYJLUFv9D6s%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIhAOoQUb8A9bpMjSk8cnTA-BDYNXIZws1i2yJLHkP0DT0IAiB1nUalA6wTEBiR-rUVIdDqG9jxLisi5Esx49ESuN5o_w%3D%3D"sv },
			/* TA24 */ { "sncgDs4YBpo"sv, "https://r2---sn-4g5e6ns7.googlevideo.com/videoplayback?expire=1588464855&ei=d7itXqe2GLuC6dsPp8aeoAo&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AIOHAnHUD1xSjPw5dn-Mx9f3tamvr8gdYCGDcfjfJ1zw&itag=18&source=youtube&requiressl=yes&mh=Aj&mm=31%2C29&mn=sn-4g5e6ns7%2Csn-h0jeen76&ms=au%2Crdu&mv=m&mvi=1&pl=33&initcwndbps=1992500&vprv=1&mime=video%2Fmp4&gir=yes&clen=4191367&ratebypass=yes&dur=62.345&lmt=1217179810151705&mt=1588443150&fvip=2&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIhAJmaf8KgFuSNkyDc_IoN1-7uy4r7yBaj8UJaeanrYrI1AiAg7o5OEaVq5676IC7ArNRggQTuGZ571so2NzGaF_IumQ%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRgIhAL2poF6gsUMIERDHLEg8xmfVfjBXaJlG8O4lPkXXmC-AAiEA-ObtfFfjmq6ZmPI2VWhvwZguQzdastUqtm4RYl9cxZM%3D"sv },
			/* TA25 */ { "akGnLUy6aWU"sv, "https://r3---sn-4g5ednsd.googlevideo.com/videoplayback?expire=1588464672&ei=wLetXoWTMJjF-gahoIXIAg&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-ALZ-Byw_cGywm7Kv6vyU1ETkQvqpKNfCpy5HM4oeVANy&itag=18&source=youtube&requiressl=yes&mh=gB&mm=31%2C29&mn=sn-4g5ednsd%2Csn-h0jeen7d&ms=au%2Crdu&mv=m&mvi=2&pl=33&initcwndbps=2042500&vprv=1&mime=video%2Fmp4&gir=yes&clen=2207310&ratebypass=yes&dur=32.995&lmt=1217022199347477&mt=1588443028&fvip=3&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIgRog0JIGujR13pz7xaqRsXkaHZcKKknOSicIm1H3oQxECIQD28GzwIp7z0Ot8lbXBGFKCiSOoDBwt8o47VqZEHk_BbQ%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIgCAe7xTRdarl3Z68PX-rvpEcZpnEIxSAQhcF4XrjbBU0CIQD4r-i7sftClLdU8PhB-AZgzSdB1O5LTu6RmcZzO1t8sw%3D%3D"sv },
			/* TA26 */ { "bQltg0Hwq84"sv, "https://r3---sn-4g5ednsy.googlevideo.com/videoplayback?expire=1588465014&ei=FrmtXpzmCc-W8gPP-K3YCg&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AKNzjtr4flN3dEpY6Wg6NBjC56xr4RrrEm3jKiWTM5d7&itag=18&source=youtube&requiressl=yes&mh=F7&mm=31%2C29&mn=sn-4g5ednsy%2Csn-h0jeln7y&ms=au%2Crdu&mv=m&mvi=2&pl=33&initcwndbps=2000000&vprv=1&mime=video%2Fmp4&gir=yes&clen=5460407&ratebypass=yes&dur=80.921&lmt=1217043365974590&mt=1588443328&fvip=6&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIhAMyebrQ0l8n12sQPzV9wBfAzbHMsLe53cAcrI39VtwYkAiA8XzzZgXpxVfqw44XlReOAGLlmTDxtFU28_jpWl-q8cQ%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRAIgIOndav5mvT-C9MP06yhMGaIluXS2l9Vk1YDBlk_wyiECIDVZNlVugVpC-aGR1O1MENVTOhtUqb2MzMBpIcceq9Nb"sv },
			/* TA27 */ { "bPUSj_brL6w"sv, "https://r4---sn-4g5ednle.googlevideo.com/videoplayback?expire=1588465035&ei=K7mtXsKXJYOQ1gLL9bnwBA&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AAsAPWRieNisJINTpBqT_KAMJuIV0TUoW4bMJ_nLWYM_&itag=18&source=youtube&requiressl=yes&mh=ZQ&mm=31%2C29&mn=sn-4g5ednle%2Csn-h0jeened&ms=au%2Crdu&mv=m&mvi=3&pl=33&initcwndbps=2028750&vprv=1&mime=video%2Fmp4&gir=yes&clen=5222423&ratebypass=yes&dur=77.392&lmt=1217044211963185&mt=1588443328&fvip=4&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRAIgPIOw4NPKQiCGZLeXBi0h_hoX62eX8zmNn1LJpW3U1bACIFG4ghXCaeBjXaWh9ooHbPcJPa5IMWo22M_7Pm9SUDtA&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIhANMLIC310wkNp-jWqiB0FON5L4CefS8oTbsaLxpHVBD7AiBFJLyUjddGIt8CqiRoYO76ILkM4srAhdI01bX1kMHicQ%3D%3D"sv },
			/* TA28 */ { "RpIRRZjL40g"sv, "https://r2---sn-4g5ednld.googlevideo.com/videoplayback?expire=1588465052&ei=PLmtXsf1Dp6W1gLg44roBw&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-ALGwCdCJtFZhJrr4wiwTrEbGiimWFS3DToEsbQl5recc&itag=18&source=youtube&requiressl=yes&mh=aR&mm=31%2C29&mn=sn-4g5ednld%2Csn-h0jeen76&ms=au%2Crdu&mv=m&mvi=1&pl=33&initcwndbps=2000000&vprv=1&mime=video%2Fmp4&gir=yes&clen=1548155&ratebypass=yes&dur=27.980&lmt=1386564644607583&mt=1588443328&fvip=2&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRAIgTNPFo0yRRCejSnconDDzIduQFTGg8LPL0imYqTAwjIwCIHac24OGRlS7IBzVSyw5x9MzfKfvLFiatO2Kso0TAGux&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRgIhAIa4cZinQ_q63N5YnFGAc1N0QbYMZPnH8kFAuuPasdAEAiEAp0wdvo0bjsNudAsbLohp4W4nldWD54THp-4_wKz35Hk%3D"sv },
			/* TA29 */ { "QqCFr0w2Z18"sv, "https://r2---sn-4g5e6nsd.googlevideo.com/videoplayback?expire=1588465069&ei=TbmtXonIGNXk-gbs9qDQAw&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AGt8vPxaHO_MQLG_B8scfd7IYwk9yll1jcvePC7eESOI&itag=18&source=youtube&requiressl=yes&mh=9C&mm=31%2C29&mn=sn-4g5e6nsd%2Csn-h0jeened&ms=au%2Crdu&mv=m&mvi=1&pl=33&initcwndbps=1926250&vprv=1&mime=video%2Fmp4&gir=yes&clen=2081377&ratebypass=yes&dur=31.602&lmt=1217045120423671&mt=1588443448&fvip=2&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIgZDnN9ojZhrqyaQwdONFHYsUQF6z_je2s0PvQ5_p7h1oCIQDNuzTZk-MgCzfgepZ_s-PBsHPR0tQPRPTTgvm2AXIk_w%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRAIgA0UES0ijwtIkRu-wifcj4pe0c4yYZKjKY-XJa1C7ILICIDUOxsOT2NbgTHkb93plDTQeWZAvF9h7mzL0D5OD2Fm_"sv },
			/* TA30 */ { "rlZRFsPpCyQ"sv, "https://r4---sn-4g5ednsl.googlevideo.com/videoplayback?expire=1588465085&ei=XbmtXu27HJLn1gLg-4q4CQ&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AEqA8IvtxUfkkZWyLhLR28yJ7P0toJW_vl5TMsMbyQfe&itag=18&source=youtube&requiressl=yes&mh=4I&mm=31%2C29&mn=sn-4g5ednsl%2Csn-h0jeln7l&ms=au%2Crdu&mv=m&mvi=3&pl=33&initcwndbps=1997500&vprv=1&mime=video%2Fmp4&gir=yes&clen=1813588&ratebypass=yes&dur=27.980&lmt=1580755845522775&mt=1588443448&fvip=4&c=WEB&txp=1311222&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIga-_W9KGd2WD1eqGdqOMDVczV4tGeFhQ9v9QJak9mEqcCIQCJOfWY7T_ARTasdICuSN9p5cf_s2urhLGWbyYqzvMT3Q%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIgD6K4HdAvDLsc05U_E9JkeZbQgfWo_TcVvUn8uk6EYMMCIQCiqfHhyW8WsrMUuh2_kiCne_jmhGTB0cfcYY20n-qu_A%3D%3D"sv },
			/* TA31 */ { "1nIr4OsRGvM"sv, "https://r4---sn-4g5ednly.googlevideo.com/videoplayback?expire=1588464215&ei=97WtXuqkKoan7gOQ9JP4Dg&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-ALhL6UNWkliz8MSFvkz8GVozXW-1v5576eUcBNVjiDlX&itag=18&source=youtube&requiressl=yes&mh=Ar&mm=31%2C29&mn=sn-4g5ednly%2Csn-h0jeened&ms=au%2Crdu&mv=m&mvi=3&pl=33&initcwndbps=2040000&vprv=1&mime=video%2Fmp4&gir=yes&clen=1884603&ratebypass=yes&dur=28.351&lmt=1217047693577946&mt=1588442492&fvip=4&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRAIgGrTT3igJcjEZ6XWn3Cl1Rsb1e-SYcTVIXYOpllGaNFECIHOvQO2clqwgA9gsyEOajgeoQdXLR5wSEyy8jglSf1nm&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRAIgMYqqY2n_iuNhu81yW_N1qsIMjwA7O24sLQOx503hIPQCIGHE28-MKhxrQZR1FFw_OYeheRt6DbkCtGq3ILjjLVWR"sv },
			/* TA32 */ { "ZYgBT9_Oi54"sv, "https://r1---sn-4g5ednld.googlevideo.com/videoplayback?expire=1588465114&ei=ermtXquuKsvrgAfN_qToDw&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-ALyd_nS-8plkua9EklfxKh5O55hOtUxI5zhL9NVrAxrx&itag=18&source=youtube&requiressl=yes&mh=y4&mm=31%2C29&mn=sn-4g5ednld%2Csn-h0jeln7l&ms=au%2Crdu&mv=m&mvi=0&pl=33&initcwndbps=1996250&vprv=1&mime=video%2Fmp4&gir=yes&clen=5089143&ratebypass=yes&dur=64.365&lmt=1393758802905036&mt=1588443448&fvip=1&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIgG6NKFIo6q9md2r1N-ry5wTKygsx4YqWpp_tqtGdKb3UCIQCyqXdlm4ubyqmjH6NCItCUUL_25GVzTcKoWsoE1ifDBg%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRgIhAPeXpQpCeexAqj638xYP73U-YJ3IJO63mgHAK4KLLDkjAiEApUBGLbhhXtTm8s92JWtxrB2OHvv5vMluTwKIIKUJtGU%3D"sv },
			/* TA33 */ { "FzD4oiaYAZQ"sv, "https://r5---sn-4g5edns6.googlevideo.com/videoplayback?expire=1588464656&ei=sLetXvCjMpmngAfL4L3QDA&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-ANOzGwYSORoLCAAGzsmpqYj4iCGHL_QsOEpxAhAlcgFM&itag=18&source=youtube&requiressl=yes&mh=7e&mm=31%2C29&mn=sn-4g5edns6%2Csn-h0jeen76&ms=au%2Crdu&mv=m&mvi=4&pl=33&initcwndbps=2042500&vprv=1&mime=video%2Fmp4&gir=yes&clen=6080292&ratebypass=yes&dur=72.562&lmt=1394853486972957&mt=1588442971&fvip=5&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRAIgeWeWsET93WyVav78LMngdnfgkQdzaHDdUDVOrQimFgwCIEQ_-UF-hu4RIxpd9gXv9GQ4ZIq6unnaO0gVQ_1bypa7&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIgHPDAdU_4JWyUjcHMSMwAUZolJTkoOXOFoUt7iz8AaIQCIQDOzbvQP5sccJvqFEhtFwbFCVlHCQubT-6DKtzmFn3PBA%3D%3D"sv },
			/* TA34 */ { "w0TWWHDSdFE"sv, "https://r4---sn-4g5edne6.googlevideo.com/videoplayback?expire=1588465071&ei=T7mtXpveDcPkgQe6vajIBA&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AJo5b2cdc3lVUdYw57l27cSo58KABFGShgq4aE8f8yf3&itag=18&source=youtube&requiressl=yes&mh=sV&mm=31%2C29&mn=sn-4g5edne6%2Csn-h0jelne7&ms=au%2Crdu&mv=m&mvi=3&pl=33&initcwndbps=2028750&vprv=1&mime=video%2Fmp4&gir=yes&clen=9786072&ratebypass=yes&dur=147.400&lmt=1387681350193527&mt=1588443390&fvip=4&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIhAIOfofIwy3xuarlYT7QkVn3w1swybEKNX8gysNP94TM_AiBovU4zi32LiGKKyt5F_ixxVWiyHyuWjUFxGRYY3qKt0Q%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRAIgWy1RCltucQ_sx-gTK7BWgnWMQ0rlOBWyVYIauoZgMcsCIDcq-j7d11MkCvhVlYQyG1UE2lGK6S4YjquGXAjWVhlv"sv },
			/* TA35 */ { "-3h0wRZq_1I"sv, "https://r4---sn-4g5e6nez.googlevideo.com/videoplayback?expire=1588464202&ei=6rWtXsSlBLOR8gO2lb-YBg&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-ADQVMgKO384EmF-yWmEqjYgbnfYFHraSH8sUd8ERXUs1&itag=18&source=youtube&requiressl=yes&mh=Ur&mm=31%2C29&mn=sn-4g5e6nez%2Csn-h0jeln7y&ms=au%2Crdu&mv=m&mvi=3&pl=33&initcwndbps=2016250&vprv=1&mime=video%2Fmp4&gir=yes&clen=7043425&ratebypass=yes&dur=118.096&lmt=1395432481203228&mt=1588442492&fvip=4&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRAIgddiQ-S1yUwPYTBKWefx8WaXolqWB8ONhdD6sQaWs53oCIBKM-vn7RQ5_3TjiF1VAIsGuhSQNKibRuuvvFfmVA04c&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRgIhAMsLeP63oUF93aLHcYkQWh1AO8iPVM1B-hsOeEtBpocfAiEA6VzJ62-f08pP2oVdX6SEasI06PO4-MYS6gmTYx-FUZg%3D"sv },
			/* TA36 */ { "-BdkyO3SgJA"sv, "https://r2---sn-4g5e6nsr.googlevideo.com/videoplayback?expire=1588464854&ei=dritXtqAF5CIgQf4_oWICw&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-ANoXTjXOUuMtUYAecNBtwW5iv9hXiZqk8VLEUkk-Ifg_&itag=18&source=youtube&requiressl=yes&mh=bO&mm=31%2C29&mn=sn-4g5e6nsr%2Csn-h0jeenek&ms=au%2Crdu&mv=m&mvi=1&pl=33&initcwndbps=2062500&vprv=1&mime=video%2Fmp4&gir=yes&clen=9975115&ratebypass=yes&dur=147.516&lmt=1217102682726680&mt=1588443211&fvip=2&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIgFSBrHZVGbDjm7UYvnSpKaCTcwN0bUH9ct7rlWobXVWMCIQDEKh7EQaPQGNdju1zqz6c1W1c5zeTmyR8c5ufexv6c8A%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIhAIpaqOSL0Lp6AaRHTPYFIX54NhOouF5sGeC9D0UMLEIIAiAI3noYt4Bkd3Ou8c8AF23koE_fc0x4g9eqCdh1c9oh1g%3D%3D"sv },
			/* TA37 */ { "5AtSNnxBxE4"sv, "https://r6---sn-4g5ednll.googlevideo.com/videoplayback?expire=1588465053&ei=PbmtXu6qG4io7gOD1oX4Bg&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AGTfUzamUJqE8QrBI8qSqVAlNxj5ktobE8NpwWrg3nT8&itag=18&source=youtube&requiressl=yes&mh=sZ&mm=31%2C29&mn=sn-4g5ednll%2Csn-h0jeln7e&ms=au%2Crdu&mv=m&mvi=5&pl=33&initcwndbps=2028750&vprv=1&mime=video%2Fmp4&gir=yes&clen=6762744&ratebypass=yes&dur=118.305&lmt=1394460335715528&mt=1588443328&fvip=4&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIhAOaVLgyR96tedMAvX8Kgtzi2DA1wRJbXUOSPP-nxSVOGAiAhKjRhPOaLd2f1YFKrHiP1JsFm6QqYYcvORTMrhm6kyg%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIhAJQeiC33sS23u98bWeLd3IsG78Y2x68bYIlalM6fjs4JAiAxhBKDY2Aut0qy-lxpYbP6jU1iijlGuKvtHs3ebVymDA%3D%3D"sv },
			/* TA38 */ { "eOz7LM6DYRM"sv, "https://r1---sn-4g5ednld.googlevideo.com/videoplayback?expire=1588464692&ei=1LetXv6TEomh4gGQkrfgDw&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-ABqfosUv9L2550AtKngjB-kcZZmyhRfnx85y5QjDN5QI&itag=18&source=youtube&requiressl=yes&mh=Ig&mm=31%2C29&mn=sn-4g5ednld%2Csn-h0jeener&ms=au%2Crdu&mv=m&mvi=0&pl=33&initcwndbps=1905000&vprv=1&mime=video%2Fmp4&gir=yes&clen=4593469&ratebypass=yes&dur=94.040&lmt=1392283063808513&mt=1588442971&fvip=1&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIhAPnmbuWUgTZcUaWSgr9IjyE41fDfVWblUr2ghlgs50I3AiAtzmmNqRA6_3ZSlH7BJfzLjUvBdX2NhUYk71HhYLIjZg%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIgDQoRNzzu6qDUmf6bdTu-kmvrasztAFfd4cfuwJzPGOsCIQD6frtfDA4qcZJwJXuHFu6RvL3PBIgHYcwNp0mo0QHoFg%3D%3D"sv },
			/* TA39 */ { "05TL_bQF0os"sv, "https://r3---sn-4g5ednsd.googlevideo.com/videoplayback?expire=1588465206&ei=1rmtXvK8D9nH7gOr_rmACQ&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AM2TzYTD-Pbne5MmI-jGVL-fLchCwARiBgw3NKB9zlsI&itag=18&source=youtube&requiressl=yes&mh=i5&mm=31%2C29&mn=sn-4g5ednsd%2Csn-h0jeener&ms=au%2Crdu&mv=m&mvi=2&pl=33&initcwndbps=1980000&vprv=1&mime=video%2Fmp4&gir=yes&clen=1468530&ratebypass=yes&dur=25.983&lmt=1582127338307233&mt=1588443572&fvip=3&c=WEB&txp=1311222&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRgIhAL9y4R2ztp9LAn6Mvf5Vnhcf_4uEillri5aHWmWX5lK8AiEAzctaH9NroVhmWuiLy6EI_4_1nNBnwH9R0LOtOfjxAWM%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRgIhAMqSyyFQ5C1lJR5C-tLUMymjG_f2X41ky7uXHU4Z4AZFAiEA4OCZVW2pCHVYg8L-OCSquM936j3EYU7Im2Z3GeikzoY%3D"sv },
			/* TA40 */ { "GcPZcM7qYQQ"sv, "https://r2---sn-4g5ednz7.googlevideo.com/videoplayback?expire=1588465220&ei=5LmtXoHPBoKrgQetvpjwDQ&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-ALC6QqL64O7-FvnSXif3oyNwOslJKJZ7tDDrsLraNBjB&itag=18&source=youtube&requiressl=yes&mh=T7&mm=31%2C29&mn=sn-4g5ednz7%2Csn-h0jelne7&ms=au%2Crdu&mv=m&mvi=1&pl=33&initcwndbps=1997500&vprv=1&mime=video%2Fmp4&gir=yes&clen=1383863&ratebypass=yes&dur=25.959&lmt=1392363141741550&mt=1588443507&fvip=2&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRAIgK6JzObhTBG0h9OCBZ6FJMWdr_dYGqB5npNPDJ1W9kgsCIFn_qZDK-oisrIy3COj_T60JxiFOltuqpIRz5g8huQVT&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIhAPclREu39wly_YAvTFCNECwGw_LT8vN3VTzaSwrhF_WPAiAS_K-okejjAza5uHGeP5U8odeKeRlryTrlasEGit4A6A%3D%3D"sv },
			/* TA41 */ { "kEvQU8A2veo"sv, "https://r2---sn-4g5edne6.googlevideo.com/videoplayback?expire=1588465233&ei=8bmtXsvPEorbgQfJv4rYCQ&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AA-z3ZdlTIbPFCpYxlqR92hTRKmuXGUCfyg5hjT8c4Rf&itag=18&source=youtube&requiressl=yes&mh=uk&mm=31%2C29&mn=sn-4g5edne6%2Csn-h0jeln7l&ms=au%2Crdu&mv=m&mvi=1&pl=33&initcwndbps=2038750&vprv=1&mime=video%2Fmp4&gir=yes&clen=1751622&ratebypass=yes&dur=26.354&lmt=1217177204068666&mt=1588443507&fvip=2&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRgIhALZPwJdAaP7rtsQCNCDkQhmAVQ-90kL4Q32aIi5bea9cAiEAw2Vjj-IsYJAM0AHPkDyf3CUSir8FeOeqPkJgDoLwcto%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRAIgMnUYeWezHNpa48ylkPtBKJKphsOIH65lPh4EBmci0JMCIFe1rMzYhI8e76Zp70ID4uI9GuO5APkl--KhLa-DmXvn"sv },
			/* TA42 */ { "AVDMgPeEHpU"sv, "https://r3---sn-4g5ednle.googlevideo.com/videoplayback?expire=1588464206&ei=7bWtXvedOZqB8gP5jIXoCQ&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AKCewtXX8ZbJLskldrJJbz2F1NW6vdklPjAW2BTFLqSG&itag=18&source=youtube&requiressl=yes&mh=ar&mm=31%2C29&mn=sn-4g5ednle%2Csn-h0jeenek&ms=au%2Crdu&mv=m&mvi=2&pl=33&initcwndbps=2142500&vprv=1&mime=video%2Fmp4&gir=yes&clen=1748751&ratebypass=yes&dur=26.354&lmt=1217178423990574&mt=1588442492&fvip=3&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRgIhAJnuoWyUwfnfxpgIzfA5CnnzNztZjqxec1L7gL2TmhcOAiEAg1w2mIQYjlcd3ekq7vojO8GUC5gGU2xSjM2FO2zxnic%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIgLvwuvOay-CNaDrmQGr8ETrZAxKYouoBlvHF5BSxFT7QCIQDxO4UDRZDWS0G1zPwZ7m5hSdE9ZQsG50u6I4utslkaLA%3D%3D"sv },
			/* TA43 */ { "4t0j7xOF0os"sv, "https://r6---sn-4g5e6nle.googlevideo.com/videoplayback?expire=1588465071&ei=T7mtXpbULY7y-gbBqJRQ&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AB336ww6E-fZ34H7yehs2axMOmUQKPMpYwQ9T61_Tm4J&itag=18&source=youtube&requiressl=yes&mh=Vr&mm=31%2C29&mn=sn-4g5e6nle%2Csn-h0jeln7y&ms=au%2Crdu&mv=m&mvi=5&pl=33&initcwndbps=2028750&vprv=1&mime=video%2Fmp4&gir=yes&clen=4189175&ratebypass=yes&dur=62.345&lmt=1217179110224415&mt=1588443390&fvip=6&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRAIgMEUSzMViM1hr-xi03TH4jitT0o8cQjCr1NKvj7oBvpwCIBv-kTgdzic25R9biOfN3zyCBYYSaTYtmxMBLaPXSgnd&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIgEplsDF6c1jFUdkUMh8CUS7NrXKXG1s6ux2J-eD0aEIsCIQCTeV4bMM3CpGbTGjaDpFYp2LDYgOyO9065xnHIveQKuA%3D%3D"sv },
			/* TA44 */ { "hf9bnHws0X8"sv, "https://r3---sn-4g5ednll.googlevideo.com/videoplayback?expire=1588464656&ei=sLetXozyEonFgAetvrPABw&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AO-8_2O1qGQGznLb346SQDAlIYxb7Iw9dF1-1YyJqaSi&itag=18&source=youtube&requiressl=yes&mh=vv&mm=31%2C29&mn=sn-4g5ednll%2Csn-h0jeln7k&ms=au%2Crdu&mv=m&mvi=2&pl=33&initcwndbps=2043750&vprv=1&mime=video%2Fmp4&gir=yes&clen=2324702&ratebypass=yes&dur=34.644&lmt=1392515310861680&mt=1588442971&fvip=3&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIhANtb4kAW7dgvIEAsNtmF-8vqLXcvqnsasdwobWJJBaL_AiB_qaQ2ITbrn-hECaeSFdd-ZOQe2s-agWDpxiSU1Gph1g%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIgI4W-djMba9pHg3eZi3PHGG-iUiM5NuoMc4pF895KZHYCIQDfjG2IdxVzjXfwTjfAdF5NRPktLvRnsE7fCb8T1XkM7g%3D%3D"sv },
			/* TA45 */ { "JErgINI4lMg"sv, "https://r2---sn-4g5ednsz.googlevideo.com/videoplayback?expire=1588464200&ei=6LWtXsC3MpKQ-gbdppSYDA&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AKwoZ2tNw7Qi3exatr4ECdfDX8bIoep5JUBH9Irn_Q4p&itag=18&source=youtube&requiressl=yes&mh=h5&mm=31%2C29&mn=sn-4g5ednsz%2Csn-h0jeln7l&ms=au%2Crdu&mv=m&mvi=1&pl=33&initcwndbps=1961250&vprv=1&mime=video%2Fmp4&gir=yes&clen=2290990&ratebypass=yes&dur=34.667&lmt=1582123483966462&mt=1588442492&fvip=2&c=WEB&txp=1311222&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIhAM7sSBrM3HYEeFCO8R1aEg6rahsPmVY8sE9qUWiNSwRgAiAgP-I50un5yhZBQFABi-RU5KfWsKPcq6R2pasCnjUeLA%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIgDx8T5NlZ-E6didXJZ4yWD3R6z4kmSJ5jaNJMxtgXmJkCIQC830Rcrwo0AbIzHu-K9uCWkDH2AJEIKG71Un4lZ8JJoA%3D%3D"sv },
			/* TA46 */ { "HNgX-S85cuc"sv, "https://r2---sn-4g5ednly.googlevideo.com/videoplayback?expire=1588464212&ei=9LWtXsOAMIWK1wKyg7ngCQ&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AKHDg1CEyh5msyeusJqpiP1pZxAe641MjN0kx_qZgozu&itag=18&source=youtube&requiressl=yes&mh=gZ&mm=31%2C29&mn=sn-4g5ednly%2Csn-h0jeenek&ms=au%2Crdu&mv=m&mvi=1&pl=33&initcwndbps=2016250&vprv=1&mime=video%2Fmp4&gir=yes&clen=3382405&ratebypass=yes&dur=43.351&lmt=1552617553300234&mt=1588442492&fvip=2&c=WEB&txp=1311222&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRgIhANS3L629P316imPamY-jYUviOQPQDIM1uEk-KsRyd-m-AiEAirX9YT0OjvTM4StvCQN-BeA-VbSBLe5q1464jM6U96E%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIhAPYJx4vOIiFrXOs6aSodDamZz4NVZiNfAn0RC_jDZbYxAiBHeaM7F5tomtDBvITLegehH6Zg9WAAR5-N0FqCySuPMQ%3D%3D"sv },
			/* TA47 */ { "u4i5OTVZxvE"sv, "https://r5---sn-4g5e6nsz.googlevideo.com/videoplayback?expire=1588464692&ei=1LetXs-3NJbI7gOhpK6gBQ&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-ANiRVg9XVH8WuJ1iU8ROZaMAuEtxAAkkhxFpPdFt-oSZ&itag=18&source=youtube&requiressl=yes&mh=rV&mm=31%2C29&mn=sn-4g5e6nsz%2Csn-h0jeened&ms=au%2Crdu&mv=m&mvi=4&pl=33&initcwndbps=2090000&vprv=1&mime=video%2Fmp4&gir=yes&clen=3273915&ratebypass=yes&dur=43.351&lmt=1391355474542090&mt=1588442971&fvip=5&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIgHZo6Tj84kqKB8ugBZHBCqKQHYP_BIMwNbV80_LQ5D74CIQDM-on41d6jBHEJcbfmxT3K4dP2TPDzhqg3DXU92C1zjQ%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRgIhAKlnSazIOAso9uxSQwmr2JjAyyWmjNXoNCqyW-QMGRsqAiEA-7tqWMIebaEFV1jNl9ZjJ8LN08rG5qM_GkIUqcZaqDc%3D"sv },
			/* TA48 */ { "XxYu8eAqC5U"sv, "https://r3---sn-4g5ednsk.googlevideo.com/videoplayback?expire=1588465322&ei=SrqtXoaHJImX1gKmg6XADw&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AIxe7Zrlu5ylAN6qx9FMyeECeZMp_l1oqe8YkZV_rpj-&itag=18&source=youtube&requiressl=yes&mh=Pk&mm=31%2C29&mn=sn-4g5ednsk%2Csn-h0jeln7l&ms=au%2Crdu&mv=m&mvi=2&pl=33&initcwndbps=2050000&vprv=1&mime=video%2Fmp4&gir=yes&clen=3252653&ratebypass=yes&dur=41.006&lmt=1389531605051918&mt=1588443633&fvip=3&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRAIgM8q5eIMNvup-eDIGInE5Y-x-0gAh5q3dh3OcmFGsgZICIBhtk9_j0M-t32glKYK2QKTORoZgc5zalWn1KgR5sse4&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRAIgTNlQEewzNYmwr1qwtQ-yItfpxUiToQNYisC09fbJHuoCIBktMORuTaemk4-47M1Ul_64ltdorZUNF3t2GGWicZ6g"sv },
			/* TA49 */ { "OLc9gKhKADg"sv, "https://r5---sn-4g5e6nss.googlevideo.com/videoplayback?expire=1588465347&ei=Y7qtXoqgFI7l1wK64pWwDw&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AMmfrGitiQ-qYTW1bFhA8BNmcF3NiUkoS0wPn5BybnXo&itag=18&source=youtube&requiressl=yes&mh=8E&mm=31%2C29&mn=sn-4g5e6nss%2Csn-h0jeened&ms=au%2Crdu&mv=m&mvi=4&pl=33&initcwndbps=2028750&vprv=1&mime=video%2Fmp4&gir=yes&clen=3252488&ratebypass=yes&dur=41.006&lmt=1391319520107087&mt=1588443633&fvip=5&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIhALZVevq4oVVEEs7cmKDg1mNt-V5JOGp2w-P44nMam0d-AiB3A-_UHUqN1tGD6PBtFzv4RJiw-_mnFEcUfIMyvN72aA%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIhAMQ2e9dzV-IPH_Ul4B74TXdToG9wmDeKOLDTYawKzb8aAiAsGIw8T9F_LQGYOjhgej6X8_qV9nXkACu2TsjzC_tgcQ%3D%3D"sv },
			/* TA50 */ { "ZciilZf1spU"sv, "https://r3---sn-4g5e6nss.googlevideo.com/videoplayback?expire=1588464659&ei=s7etXsD1CoyN7gOCwbywBw&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AG6mX7lTV-FV3g2lwLeQ6obmhN4r_yCebVc--fRbtu4X&itag=18&source=youtube&requiressl=yes&mh=5n&mm=31%2C29&mn=sn-4g5e6nss%2Csn-h0jeln7k&ms=au%2Crdu&mv=m&mvi=2&pl=33&initcwndbps=2042500&vprv=1&mime=video%2Fmp4&gir=yes&clen=3448897&ratebypass=yes&dur=47.113&lmt=1217194147474936&mt=1588442971&fvip=6&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRAIgT3-LXQhAK8vr-JZo2PJXgry-5xMIe5Dpdz7gDMrBtEACIAwEn7zzHnSqdjUbOBWJUVFmWBOWJbSzrVnRKPAF5a6R&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRgIhAIwNZxp1U0GDtV4n3a0gx7C2H-k2hkqJzCkXO_tgyePRAiEAjvr7FYdhOI09-orJK81QVxiA_6xV2zVqXrEBCkd56Zw%3D"sv },
			/* TA51 */ { "AAA_NxhT-Zw"sv, "https://r1---sn-4g5e6nez.googlevideo.com/videoplayback?expire=1588464189&ei=3bWtXoOJFZ3-1wLw4aeQCQ&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AIlufJTwO8AuzgU0RCZl1xo659Zl9OD2JHtpgVr4_px7&itag=18&source=youtube&requiressl=yes&mh=oM&mm=31%2C29&mn=sn-4g5e6nez%2Csn-h0jeen7d&ms=au%2Crdu&mv=m&mvi=0&pl=33&initcwndbps=2016250&vprv=1&mime=video%2Fmp4&gir=yes&clen=3191930&ratebypass=yes&dur=43.281&lmt=1217195978515156&mt=1588442552&fvip=1&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRgIhAMFanYdi8UGrkG3iAgfsxp7vcskQTeKDc7qUVQmfvrQGAiEAo30sSpV5MSgJJ_mRlVYK1cY2UZ6bJY657N_KyUFeEvE%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIgBHFzdZZm81myYGYr3ZcM6BJn99nHBqItxlEaRqQQflcCIQCTQzwvsZPxlLa6Xk3Z49tsaSCk1H6jxZguRxCA3KQBhw%3D%3D"sv },
			/* TA52 */ { "pHT-afoqzt8"sv, "https://r1---sn-4g5e6nss.googlevideo.com/videoplayback?expire=1588464672&ei=wLetXqOKEdPIgAfb6I_QDQ&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AB1vqTWnHptzTFdPq4qQ-PfFgoZ1zzm5z6Huh8rh5fl2&itag=18&source=youtube&requiressl=yes&mh=Na&mm=31%2C29&mn=sn-4g5e6nss%2Csn-h0jeenek&ms=au%2Crdu&mv=m&mvi=0&pl=33&initcwndbps=1905000&vprv=1&mime=video%2Fmp4&gir=yes&clen=2645220&ratebypass=yes&dur=36.455&lmt=1580624851763720&mt=1588442971&fvip=1&c=WEB&txp=1311222&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIhAJqxj34r6rN8CPs8oTkN5S4e0L1WTPU6rR5wQMPcHttDAiAZssmTsoCvSU044iVNR0I0ha6V3inKQHPVfGAi_jupxA%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRgIhAI0Pzoh85juSFlIfAtciLJFwMoSr5iU_ZIejDO9JhG5mAiEAu_gErQww1fou9SQysqvqBO8TOHWfuRxzX6HOS7uwcU0%3D"sv },
			/* TA53 */ { "Ow_ssg5gTgo"sv, "https://r6---sn-4g5ednls.googlevideo.com/videoplayback?expire=1588464658&ei=sbetXpv8PIjH7gOBvKrICA&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AF3DeRmbon4vVM_M1ycDvEJ1DMi2AsOzpJJJd2pbFgj8&itag=18&source=youtube&requiressl=yes&mh=2z&mm=31%2C29&mn=sn-4g5ednls%2Csn-h0jeener&ms=au%2Crdu&mv=m&mvi=5&pl=33&initcwndbps=2042500&vprv=1&mime=video%2Fmp4&gir=yes&clen=2485268&ratebypass=yes&dur=37.151&lmt=1217197228946617&mt=1588442971&fvip=1&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIgIsXWKKtfNm7ebEVLSBy_kpPI7nHe2TysDKSeQJS_7ucCIQCa1YvvOMT7_oTXFp2oWzrUqL5KXdKFJwH357ovSD2kug%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRAIgJpQvn25q9McL-ioiWMiUc4LRrq5rIV5EzR9s_4Zc3KkCIFo17EDOb0OApmAJrPN_13LfQxjPEG_0djfvcjkU_e2_"sv },
			/* TA54 */ { "ONN-aTxrhck"sv, "https://r4---sn-4g5e6ns6.googlevideo.com/videoplayback?expire=1588465421&ei=rbqtXribEovwgQeO96DABw&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AEw51nCM6wd8kAsfaWOa3g0HRhKVhAYNjR6h19VVJW8N&itag=18&source=youtube&requiressl=yes&mh=BZ&mm=31%2C29&mn=sn-4g5e6ns6%2Csn-h0jeen7d&ms=au%2Crdu&mv=m&mvi=3&pl=33&initcwndbps=2028750&vprv=1&mime=video%2Fmp4&gir=yes&clen=2595279&ratebypass=yes&dur=38.777&lmt=1391316002037661&mt=1588443751&fvip=4&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIgI8ZKPCvVc7S7VHg5lOoogeRdk1cm_UgXKKOXlSKB6GoCIQDlq716fq4dPjxH-nn53xrZ4zhnp5lN7umTbYg68MryeA%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRAIgKzjF0BspSBovNi9-HFEDwStrAF48CLf_Yezyw5Bwo9ACIAUo3XufzFkmn6vK-0n44tp5Gxbg-gozPvxfVAg3y0Cy"sv },
			/* TA55 */ { "Y7gaVAHxzE0"sv, "https://r2---sn-4g5e6nez.googlevideo.com/videoplayback?expire=1588465052&ei=PLmtXoT3M8vh-gaxmquwAQ&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-ADB1kfwgLXAYDfbwkQUVh5x_vba0sZh-FgX3DwvC2zqb&itag=18&source=youtube&requiressl=yes&mh=8s&mm=31%2C29&mn=sn-4g5e6nez%2Csn-h0jeln7k&ms=au%2Crdu&mv=m&mvi=1&pl=33&initcwndbps=2000000&vprv=1&mime=video%2Fmp4&gir=yes&clen=2594939&ratebypass=yes&dur=38.777&lmt=1392397741150205&mt=1588443328&fvip=2&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRgIhAIt-qVe6rBk9CTW3uw2ulWWbCeQgJodZx71yH-3Fy6K9AiEA1GQNHZTbc5-rDovGGJ8cPhyq08q2VuFDF7oaGq9SFsg%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIhAKDZ6Hh4Ijd6OtYoryXOCmaQk4bEM0sl-kijoj38YV_3AiAZmOvJqAQT15ouFt_lBeB421u611P-bFtDUH5iZiT09w%3D%3D"sv },
			/* TA56 */ { "x7rfiStC_rg"sv, "https://redirector.googlevideo.com/videoplayback?expire=1588465453&ei=zbqtXr_2Csql1gLYmJ54&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=c7badf892b42feb8&itag=18&source=youtube&requiressl=yes&vprv=1&mime=video%2Fmp4&gir=yes&clen=2893797&ratebypass=yes&dur=43.119&lmt=1217222134154298&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRgIhAIXd0cLjXKvePUhz_kK0QGa9nneHE-kZVDSXYI5Q9wWvAiEAgWj2uLqcVNndDvO9pqQ66ybMarJ2gjTM-JYw5KGBIL8%3D"sv },
			/* TA57 */ { "q01JoAIS0Ww"sv, "https://r3---sn-4g5ednsl.googlevideo.com/videoplayback?expire=1588465471&ei=3rqtXqPmPNy18gPXg4X4Dw&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AGqvZsFCppdVPVzgbcFnJx11dk2-ujKfDyw_fpv61xoV&itag=18&source=youtube&requiressl=yes&mh=Vk&mm=31%2C29&mn=sn-4g5ednsl%2Csn-h0jeenek&ms=au%2Crdu&mv=m&mvi=2&pl=33&initcwndbps=1977500&vprv=1&mime=video%2Fmp4&gir=yes&clen=2610935&ratebypass=yes&dur=42.747&lmt=1582063182721341&mt=1588443810&fvip=3&c=WEB&txp=1311222&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRAIgTtVJyvKpr6QF4bcNG1iyFxiVtPqC7vK6GJnnZuTa6OwCIG-AWifrwUwmamgOrTqNiZAY8ifcSd7J4K3SSmT7i6bp&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRgIhAKHVh2rT058bOjMcrOvdM7BOUBl4Ompbvjbe_D7QjnIiAiEApOLq6vDd918hOvTwVDZpQpXCqHqyINWGs5Wz_IToyFY%3D"sv },
			/* TA58 */ { "T6GxHvVIbhk"sv, "https://r3---sn-4g5e6nsz.googlevideo.com/videoplayback?expire=1588464209&ei=8bWtXv_HJpfvgAfRp6ioCA&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AKitK1JkgsVlHv-8OPz0P8mjn_yzYSAAgDbxlN1KRNIp&itag=18&source=youtube&requiressl=yes&mh=31&mm=31%2C29&mn=sn-4g5e6nsz%2Csn-h0jeened&ms=au%2Crdu&mv=m&mvi=2&pl=33&initcwndbps=2142500&vprv=1&mime=video%2Fmp4&gir=yes&clen=1189468&ratebypass=yes&dur=21.478&lmt=1217253338164669&mt=1588442492&fvip=3&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRgIhAPjJ5yn-b5gssb_vh2bUTyCLOK73aYPg58lrTFkbWjJWAiEAvdndDviivNevKz8-IY3fK4iGToIKWEKSF6p3ao5ehWQ%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIhAOYy8gM9uYE0ffg9yJ7XodPZwaRSd6eZ-x_C1oknhYQyAiBt7y-llEDrS63c5bXDcwqvKsxOU3bXWPI3SsVUaC3FiA%3D%3D"sv },
			/* TA59 */ { "pO-oIZbQnqQ"sv, "https://r5---sn-4g5e6ns7.googlevideo.com/videoplayback?expire=1588464208&ei=8LWtXt-THoG8gQep1by4Dg&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AGgr3DAq4JiKdAN9E3gg_5abdBRUKlSqBxCiSTcgN_aT&itag=18&source=youtube&requiressl=yes&mh=9S&mm=31%2C29&mn=sn-4g5e6ns7%2Csn-h0jeen7d&ms=au%2Crdu&mv=m&mvi=4&pl=33&initcwndbps=2040000&vprv=1&mime=video%2Fmp4&gir=yes&clen=956885&ratebypass=yes&dur=21.083&lmt=1391771846798967&mt=1588442492&fvip=6&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRgIhAInHunWsEJRo5W33G6DFx-efyPIvmmVxHeaPrkaIfxppAiEApWZRQbR6jNpTJmXk8WPkQmRytVYn-VR1hJPvmPv1VoY%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIgJ78KzHntl9_0LjKULgcKYM99i4B3cNFKYlRANloQ-xcCIQC0Jz6aau4jBMtErW2yB3gGI5YH8ObdfSQmBmL0wLmxUg%3D%3D"sv },
			/* TA60 */ { "c0CtvZKz2zw"sv, "https://r2---sn-4g5e6nzl.googlevideo.com/videoplayback?expire=1588465515&ei=C7utXtmABdKp7gOy-7KgBA&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AOMcdvrURLr0e3DrWXnMlWlkdOSZh4pGbrFoa1T2sWH4&itag=18&source=youtube&requiressl=yes&mh=Y8&mm=31%2C29&mn=sn-4g5e6nzl%2Csn-h0jeln7e&ms=au%2Crdu&mv=m&mvi=1&pl=33&initcwndbps=1967500&vprv=1&mime=video%2Fmp4&gir=yes&clen=4923942&ratebypass=yes&dur=73.026&lmt=1217236012749140&mt=1588443810&fvip=2&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRAIgBwIsTWzhQaC2AMyCvQtPVzE9l8lRI6MmQdKeMWGmjbQCIHQ6FZejzPVCJy2IYj8xALaS6OwjCpnLTS59vwzkav71&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIhAJL979N7Ev1im9zWEpVFI40XIKXCzUxNGD3J4WWkCo6SAiB3VCfHA3w5gxgmGGKaer2kRUBi5HWKmFaGIW637kxcBQ%3D%3D"sv },
			/* TA61 */ { "F87N3uM33p0"sv, "https://r4---sn-4g5e6nez.googlevideo.com/videoplayback?expire=1588465070&ei=TrmtXsvXJZnN1gL3raRQ&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-ADjIocSXz2Qev6rOUDEzBmCzXJPopoUMk1QjLlbcbuIo&itag=18&source=youtube&requiressl=yes&mh=wg&mm=31%2C29&mn=sn-4g5e6nez%2Csn-h0jeener&ms=au%2Crdu&mv=m&mvi=3&pl=33&initcwndbps=2000000&vprv=1&mime=video%2Fmp4&gir=yes&clen=6308977&ratebypass=yes&dur=90.418&lmt=1552620356807499&mt=1588443390&fvip=4&c=WEB&txp=1311222&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIhAMX9QI6ezYPY_OcZiTb7S8EQG8VtHJ9qS8BmXlRAyN4JAiAuxmjnL-ZPCNLsKm6TC2xXfR2rnn420Z1P6YoYz9LM1g%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRQIgF7Bq4h1A0J1icaKA4Nu6dYFm5j0pMLCIAoiQX2QeREACIQD3rk6lZY_rVC2IwPBJcZk-cO19BJ-fhMOCT0KAtAHkjg%3D%3D"sv },
			/* TA62 */ { "x6V9YYEVPpc"sv, "https://r1---sn-4g5edned.googlevideo.com/videoplayback?expire=1588465542&ei=JrutXtqKHce7gAfLg4TYAg&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AHbjU73nEZsVQyNrnfL3xUKqy9tnMArRztdcoQi8NKqe&itag=18&source=youtube&requiressl=yes&mh=VT&mm=31%2C29&mn=sn-4g5edned%2Csn-h0jeener&ms=au%2Crdu&mv=m&mvi=0&pl=33&initcwndbps=1898750&vprv=1&mime=video%2Fmp4&gir=yes&clen=6104649&ratebypass=yes&dur=90.557&lmt=1217253398682812&mt=1588443932&fvip=1&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIhAKLFmbNTaFruznGbr3hRgPu-htDVBJiS4Pl7F_0av1S1AiB01kxllkXmlis4JMyEhNdHlBPheKbdfD1ycWck0glckw%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRAIgTI1JGuyrZxxTIMm2jh0VbGUXOlGoKlxBKbm3Ce46MwICICs_uFTWGB3ZEGXvC_ouE88OktebdbzFITogCOc5pY68"sv },
			/* TA63 */ { "DY1YtV9-Z1c"sv, "https://r2---sn-4g5e6nzz.googlevideo.com/videoplayback?expire=1588464788&ei=M7itXrnvOYj21wKR05-QCw&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AH-ML41aWKWVXh2z_eoKxj9mE5FFesq30yTXHxDe42Go&itag=18&source=youtube&requiressl=yes&mh=rZ&mm=31%2C29&mn=sn-4g5e6nzz%2Csn-h0jeln7e&ms=au%2Crdu&mv=m&mvi=1&pl=33&initcwndbps=2010000&vprv=1&mime=video%2Fmp4&gir=yes&clen=2949442&ratebypass=yes&dur=43.955&lmt=1217275161822240&mt=1588443092&fvip=2&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIhALysFET3A8OxE-WXwemejstHevlaGfLef14P_LeHR8w7AiBHru3TGxfgD6aTphzqkkaJSTYA9Zl0jTyzbP_VT2gIuw%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRgIhAJeBfh04kt67WoWExeJx_w4dOqE9nXsL4qcAhO1YSFeOAiEA43NeNUYFyubVkf2k-3Ct60SGK9u2kwKsVWs8ZLHocz0%3D"sv },
			/* TA64 */ { "-wtvbB-moOE"sv, "https://r6---sn-4g5edne7.googlevideo.com/videoplayback?expire=1588464674&ei=wbetXuPWOYaF1gL74IHoDQ&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AKicNMVQY9ZOWpjwfdxq7GRs5WDEPMCjyiG-I7_HA6VW&itag=18&source=youtube&requiressl=yes&mh=I0&mm=31%2C29&mn=sn-4g5edne7%2Csn-h0jeen76&ms=au%2Crdu&mv=m&mvi=5&pl=33&initcwndbps=2042500&vprv=1&mime=video%2Fmp4&gir=yes&clen=2724931&ratebypass=yes&dur=36.292&lmt=1580396619061209&mt=1588442971&fvip=6&c=WEB&txp=1311222&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRAIgCJ5CT-RF2Z8Mq7YUgjymD2wxsEz1wm02R80RVQyjiuACIGMjzIuZX9WHFtpQVfL_TxDSbDXYnZWESZy5doP5Xnqj&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRAIgOf4_WfpTgEkv7_IF434AdNPnM0sOxdiwrx8qwbvv2SwCIDdxR1HA7fD-qeHaHX1p0wolNTHmsOWQcfMj9Vdyr987"sv },
			/* TA65 */ { "IDD8kv-VnaI"sv, "https://r4---sn-4g5e6nzl.googlevideo.com/videoplayback?expire=1588465070&ei=TbmtXv72O9Ck8gPxroOwBQ&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AJIo_lHXydIJyB4KGEnTiwgyaOgbsYCPxqP7hYzj1QLP&itag=18&source=youtube&requiressl=yes&mh=ql&mm=31%2C29&mn=sn-4g5e6nzl%2Csn-h0jeln7k&ms=au%2Crdu&mv=m&mvi=3&pl=33&initcwndbps=1986250&vprv=1&mime=video%2Fmp4&gir=yes&clen=2899543&ratebypass=yes&dur=43.189&lmt=1217276284220558&mt=1588443390&fvip=4&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIgVOJnh7oOf8R1u8HSCvqDps9hgl1YfCwwQMDTHkK8H1ICIQCesFDi6qE7NyEmhHqrthvSwKfms8BYtZz3iY0jDMtJZg%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRAIgHx24W3-41M_at_nGiIF0BEa29qXi6Cf7BQnC18rsBTECIGJuxF4Fpj8n0M2niTlO1moUe-6ZJdgK_YY5BXPB6Htd"sv },
			/* TA66 */ { "nEcIafhGKh8"sv, "https://r1---sn-4g5ednsl.googlevideo.com/videoplayback?expire=1588465086&ei=XrmtXv_GBYnX1wKb0JSAAQ&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AI6qjRNU3wadkusRVe_qC_aDqAbY4WroL_rxIKJNJNzi&itag=18&source=youtube&requiressl=yes&mh=Hh&mm=31%2C29&mn=sn-4g5ednsl%2Csn-h0jeln7r&ms=au%2Crdu&mv=m&mvi=0&pl=33&initcwndbps=2028750&vprv=1&mime=video%2Fmp4&gir=yes&clen=2899442&ratebypass=yes&dur=43.189&lmt=1217277142346910&mt=1588443390&fvip=1&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIhAKaPnGhmkcDpPI9IdE5sZdNGHYmgoDFxOXYJSIpD4x8sAiB65J2wWoEc3rmHBLRD9okhq3XhxiGo6dwdZgY-HZlmQw%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRAIgF18HHptvFp1Szx_mgq0UlOql_p8pXip3pfu_Anu_vYECIHR5Du1CIOsl40flb3OC-wEFrgZJka8YYmWcfhNXtLjS"sv },
			/* TA67 */ { "mMrZT2GzeKA"sv, "https://r1---sn-4g5ednss.googlevideo.com/videoplayback?expire=1588464693&ei=1betXvmwF4-o1wLDr4i4CA&ip=2a02%3A908%3A13c7%3Aa360%3A2e%3A1b9a%3A3c26%3Af28f&id=o-AM6cBFvQK9D_R_eqdq_gkJT3ki8UL-Ptc0PUlMbIDSxm&itag=18&source=youtube&requiressl=yes&mh=We&mm=31%2C29&mn=sn-4g5ednss%2Csn-h0jeln7r&ms=au%2Crdu&mv=m&mvi=0&pl=33&initcwndbps=2043750&vprv=1&mime=video%2Fmp4&gir=yes&clen=10430931&ratebypass=yes&dur=141.757&lmt=1217293396201954&mt=1588443028&fvip=1&c=WEB&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AJpPlLswRQIgOPen1SuZXeHnXRybx9VmXrauvyZ9j3JDtMMpL1efa-8CIQCeXaVVPmleIyz_iNUEFfVfzqnOSbz0q0sfeHAz_iG3ew%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=ALrAebAwRAIgaiDfmGNd8sPdbfYydNYxzgBNm9qjPlExhMuy9htJgu4CIFAlHkmmFZPpFp6m9lmsqbpUa_4F9qNWHdLzDphG6JAusv" }
		};

		const auto video_url_it = youtube_id_to_direct_video_url.find(youtube_id);
		if (video_url_it == youtube_id_to_direct_video_url.cend())
			return QUrl();

		return QUrl(QString::fromUtf8(video_url_it->second.data(), static_cast<int>(video_url_it->second.size())));
	}

	// Returns empty url if failed
	[[nodiscard]] QUrl video_path_from_youtube_id(const std::string_view youtube_id)
	{
		constexpr std::string_view search_directory = "../../../data/TUBE-ADVENTURES/video";
		const auto path = find_path_with_youtube_id(youtube_id, search_directory, ".mp4");

		if (path.empty())
			return QUrl();

		const auto path_utf8 = path.u8string();
		return QUrl::fromLocalFile(QString::fromUtf8(path_utf8.data(), static_cast<int>(path_utf8.size())));
	}
} // namespace

MainWindow::MainWindow(QWidget * parent)
	: QMainWindow(parent)
	//, ui(std::make_unique<Ui::MainWindow>())
	, ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	player = new QMediaPlayer;
	video = new QVideoWidget(ui->video_parent);

	player->setVideoOutput(video);

	const QRect geom = ui->central_widget->geometry();
	video->setGeometry(geom);

	//constexpr char video_filename[] = "C:\\Users\\Andoni\\Downloads\\TEMP BIDEUEK\\Kimi no Suizou wo Tabetai.mp4";
	//constexpr char video_filename[] = "C:\\Users\\Andoni\\Videos\\Renderizados\\donete.mp4";
	//constexpr char video_filename[] = "C:\\Users\\Andoni\\Downloads\\JDownloader\\TUBE-ADVENTURES (aventura interactiva)\\TUBE-ADVENTURES (aventura interactiva) (288p_30fps_H264-96kbit_AAC).mp4";
	//constexpr char video_filename[] = "C:\\Users\\Andoni\\AppData\\Roaming\\Telegram Desktop\\tdata\\tdld\\video_2018-12-03_20-57-13.mp4";

	constexpr char annotations_filename[] = "../../..//data/TUBE-ADVENTURES/TUBE-ADVENTURES (aventura interactiva) BckqqsJiDUI.xml";
	
	play_video(annotations_filename);
	video->show();

	ui->progress_bar->setRange(0, static_cast<int>(player->duration() / 1000));
	ui->progress_bar->setValue(0);

	connect(player, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::on_video_media_status_changed);
	connect(player, &QMediaPlayer::positionChanged, this, &MainWindow::on_video_position_changed);
	connect(player, &QMediaPlayer::durationChanged, this, &MainWindow::on_video_duration_changed);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::play_video(const std::filesystem::path & annotations_filename)
{
	const u8string annotations_filename_utf8 = annotations_filename.u8string();

	annotations.clear();
	annotation_buttons.clear();

	if (ParseAnnotationsResult parse_result = parse_annotations(annotations_filename_utf8.c_str()); parse_result.error != ParseAnnotationsError::success)
	{
		QMessageBox::critical(nullptr, "Failed to parse annotations", "Error when parsing annotation file \"" + QString::fromStdString(annotations_filename_utf8) + "\".\n\nError: " + QString::fromStdString(parse_result.error_string), QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::NoButton);
		this->close(); // FIXME: This doesn't close the window if running from the constructor. How do I close the window?
		return;
	}
	else
	{
		annotations = std::move(parse_result.annotations);
		annotation_buttons.reserve(annotations.size());
		std::fill_n(std::back_inserter(annotation_buttons), annotations.size(), nullptr);
	}

	const std::optional<std::string> youtube_id = path_to_youtube_video_id(annotations_filename, annotation_file_extension);

	const auto annotations_absolute_path = [&annotations_filename]()
	{
		std::error_code error;
		auto absolute_path = std::filesystem::weakly_canonical(annotations_filename, error);
		if (error)
			return annotations_filename;

		return absolute_path;
	};

	if (!youtube_id.has_value())
	{
		QMessageBox::critical(nullptr, "Invalid annotations filename", QString::fromStdString("Failed to extract youtube video ID from the annotations file: \"" + annotations_absolute_path().u8string() + '"'), QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::NoButton);
		this->close();
		return;
	}

#if false
	const QUrl video_url = video_url_from_youtube_id(*youtube_id); // Online videos
#else
	const QUrl video_url = video_path_from_youtube_id(*youtube_id); // Local videos
#endif
	if (video_url.isEmpty())
	{
		QMessageBox::critical(nullptr, "URL/path for video not found", QString::fromStdString("Cannot find URL/path of video for the annotations file: \"" + annotations_absolute_path().u8string() + "\".\n\nYoutube video ID: " + *youtube_id), QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::NoButton);
		this->close();
		return;
	}

	player->setMedia(QUrl(video_url));

	player->play();

	qDebug() << player->state();
}

void MainWindow::resizeEvent([[maybe_unused]] QResizeEvent * event)
{
	QMainWindow::resizeEvent(event);

	if (video == nullptr)
		return;

	assert(event != nullptr);
	const QRect geom = ui->central_widget->geometry();
	video->setGeometry(geom);
}

void MainWindow::keyPressEvent([[maybe_unused]] QKeyEvent * event)
{
	QMainWindow::keyPressEvent(event);

	if (player == nullptr)
		return;

	assert(event != nullptr);

	switch (event->key())
	{
	case Qt::Key::Key_Right:
	{
		assert(player != nullptr);
		change_video_position(*player, video_seek_duration());
		break;
	}
	case Qt::Key::Key_Left:
	{
		assert(player != nullptr);
		change_video_position(*player, -video_seek_duration());
		break;
	}
	case Qt::Key::Key_Space:
	{
		assert(player != nullptr);
		switch (player->state())
		{
		case QMediaPlayer::State::PlayingState:
			player->pause();
			break;
		case QMediaPlayer::State::PausedState:
			player->play();
			break;
		case QMediaPlayer::State::StoppedState:
			qDebug() << "QMediaPlayer::State::StoppedState in video. Case unhandled\n";
			break;
		default:
			unreachable("Invalid QMediaPlayer::State");
		}
		break;
	}
	}
}

void MainWindow::on_video_media_status_changed(const QMediaPlayer::MediaStatus new_status)
{
	assert(player != nullptr);

	if (new_status == QMediaPlayer::MediaStatus::EndOfMedia)
	{
		change_video_position(*player, -3s);
		player->play();
	}
}

void MainWindow::on_video_duration_changed(const qint64 duration_changed)
{
	ui->progress_bar->setMaximum(static_cast<int>(duration_changed / 1000));
}

void MainWindow::on_video_position_changed(const qint64 new_position)
{
	ui->progress_bar->setValue(static_cast<int>(new_position / 1000));

	const auto annotations_size = static_cast<int>(annotations.size());
	for (int i = 0; i < annotations_size; ++i)
	{
		const Annotation & annotation = annotations[i];
		std::unique_ptr<QPushButton> & button = annotation_buttons[i];

		const auto pos = video_position(new_position);

		const bool annotation_showing = pos >= annotation.start_rect.time && (!annotation.end_rect.has_value() || pos <= annotation.end_rect->time);
		if (annotation_showing && button == nullptr/* && annotation.type == Annotation::Type::gameplay*/)
		{
			qDebug() << "Button with text" << QString::fromUtf8(annotation.text.data(), static_cast<int>(annotation.text.size())) << "created";

			button = std::make_unique<QPushButton>(ui->central_widget);
			button->setText(QString::fromUtf8(annotation.text.data(), static_cast<int>(annotation.text.size())));
			constexpr float pos_scale = 2.0f;
			constexpr float size_scale = 3.0f;
			button->setGeometry(annotation.start_rect.x * pos_scale, annotation.start_rect.y * pos_scale, annotation.start_rect.width * size_scale, annotation.start_rect.height * size_scale);
			
			assert(!annotation.id.empty());
			button->setObjectName(QString::fromStdString(annotation.id));
			button->show();

			connect(button.get(), &QPushButton::clicked, this, &MainWindow::on_annotation_clicked);
		}

		else if (!annotation_showing && button != nullptr)
		{
			qDebug() << "Deleting button with text" << button->text();
			
			button.reset(); // Is this the way to do it??
		}
	}
}

void MainWindow::on_annotation_clicked(const bool /*checked*/)
{
	const auto * const button = qobject_cast<QPushButton *>(sender());
	assert(button != nullptr);

	const auto buttons_begin = annotation_buttons.begin();
	const auto buttons_end = annotation_buttons.end();
	const auto button_it = std::find_if(buttons_begin, buttons_end, [button](const std::unique_ptr<QPushButton> & button_in_container)
	{
		return button_in_container.get() == button;
	});

	assert(button_it != buttons_end);

	const auto button_index = static_cast<int>(button_it - buttons_begin);
	assert(button_index >= 0 && button_index < annotations.size() && annotations.size() == annotation_buttons.size());

	const Annotation & annotation = annotations[button_index];

	if (annotation.type != Annotation::Type::gameplay)
		return;

	const std::optional<std::string_view> youtube_id = youtube_video_id_from_url(annotation.click_url);

	if (!youtube_id.has_value())
	{
		QMessageBox::critical(nullptr, "Failed to get video ID from annotation", QString::fromStdString("Failed to get the destintation youtube video ID from the URL (\"" + annotation.click_url + "\") of the annotation \"" + annotation.id + "\" (text = \"" + annotation.text + "\")"), QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::NoButton);
		this->close();
		return;
	}

	constexpr std::string_view search_directory = "../../../data/TUBE-ADVENTURES";
	const auto path = find_path_with_youtube_id(*youtube_id, search_directory, ".xml");

	if (path.empty())
	{
		QMessageBox::critical(nullptr, "Annotation file not found", QString::fromStdString("No annotation file was found for the youtube ID \"" + std::string(*youtube_id) + "\" in directory \"" + std::string(search_directory) + '"'), QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::NoButton);
		this->close();
		return;
	}

	play_video(path);
}

