#pragma once

#include <chrono>

#include <QMainWindow>
#include <QVideoWidget>
#include <QMediaPlayer>

#include <QResizeEvent>
#include <QKeyEvent>

namespace Ui
{
	class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget * parent = nullptr);
	~MainWindow();

	MainWindow(const MainWindow &) = delete;
	MainWindow(MainWindow &&) = delete;
	MainWindow & operator=(MainWindow &&) = delete;
	MainWindow & operator=(const MainWindow &) = delete;

	Ui::MainWindow * ui = nullptr;

private slots:

private:
	void resizeEvent(QResizeEvent * event) override;
	void keyPressEvent(QKeyEvent *event) override;

private:
	void on_video_media_status_changed(const QMediaPlayer::MediaStatus new_status);
	void on_video_position_changed(const qint64 new_position);
	void on_video_duration_changed(const qint64 duration_changed);

private:
	QMediaPlayer * player = nullptr;
	QVideoWidget * video = nullptr;
};
