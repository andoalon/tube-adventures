#include "ui_mainwindow.h"

#include "mainwindow.hh"

#include <cassert>
#include <QGuiApplication>

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
		assert(player.isSeekable());

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
} // namespace

MainWindow::MainWindow(QWidget * parent)
	: QMainWindow(parent)
	//, ui(std::make_unique<Ui::MainWindow>())
	, ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	//constexpr char video_filename[] = "C:\\Users\\Andoni\\Downloads\\TEMP BIDEUEK\\Kimi no Suizou wo Tabetai.mp4";
	//constexpr char video_filename[] = "C:\\Users\\Andoni\\Videos\\Renderizados\\donete.mp4";
	constexpr char video_filename[] = "C:\\Users\\Andoni\\Downloads\\JDownloader\\TUBE-ADVENTURES (aventura interactiva)\\TUBE-ADVENTURES (aventura interactiva) (288p_30fps_H264-96kbit_AAC).mp4";
	//constexpr char video_filename[] = "C:\\Users\\Andoni\\AppData\\Roaming\\Telegram Desktop\\tdata\\tdld\\video_2018-12-03_20-57-13.mp4";

	player = new QMediaPlayer;
	video = new QVideoWidget(ui->video_parent);

	player->setVideoOutput(video);
	player->setMedia(QUrl::fromLocalFile(video_filename));
	
	video->setGeometry(ui->central_widget->geometry());
	video->show();
	
	player->play();
	
	qDebug() << player->state();
	
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

void MainWindow::resizeEvent([[maybe_unused]] QResizeEvent * event)
{
#if true
	QMainWindow::resizeEvent(event);

	assert(event != nullptr);
	video->setGeometry(ui->central_widget->geometry());
#endif
}

void MainWindow::keyPressEvent([[maybe_unused]] QKeyEvent * event)
{
	QMainWindow::keyPressEvent(event);

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
	if (new_status == QMediaPlayer::MediaStatus::EndOfMedia)
		this->close();
}

void MainWindow::on_video_duration_changed(const qint64 duration_changed)
{
	ui->progress_bar->setMaximum(static_cast<int>(duration_changed / 1000));
}

void MainWindow::on_video_position_changed(const qint64 new_position)
{
	ui->progress_bar->setValue(static_cast<int>(new_position / 1000));
}

