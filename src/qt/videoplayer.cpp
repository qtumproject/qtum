#include "qt/videoplayer.h"
#include "qt/forms/ui_videoplayer.h"

#include <QVideoWidget>
#include <QStyle>

VideoPlayer::VideoPlayer(QString videoPath, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VideoPlayer)
{
    ui->setupUi(this);

    m_mediaPlayer = new QMediaPlayer(this);
    m_mediaPlayer->setVideoOutput(ui->videoWidget);
    m_mediaPlayer->setMedia(QUrl::fromLocalFile(videoPath));

    ui->buttonPlay->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->buttonStop->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    ui->sliderPosition->setRange(0, 0);

    connect(m_mediaPlayer, &QMediaPlayer::stateChanged, this, &VideoPlayer::setState);
    connect(ui->sliderPosition, &QSlider::sliderMoved, this, &VideoPlayer::seek);
    connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this, &VideoPlayer::durationChanged);
    connect(m_mediaPlayer, &QMediaPlayer::positionChanged, this, &VideoPlayer::positionChanged);

    m_mediaPlayer->play();
}

VideoPlayer::~VideoPlayer()
{
    delete m_mediaPlayer;
    delete ui;
}

void VideoPlayer::setState(QMediaPlayer::State state)
{
    switch (state) {
    case QMediaPlayer::StoppedState:
        ui->buttonStop->setEnabled(false);
        ui->buttonPlay->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        break;
    case QMediaPlayer::PlayingState:
        ui->buttonStop->setEnabled(true);
        ui->buttonPlay->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
        break;
    case QMediaPlayer::PausedState:
        ui->buttonStop->setEnabled(true);
        ui->buttonPlay->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        break;
    }
}

void VideoPlayer::seek(int seconds)
{
    m_mediaPlayer->setPosition(seconds * 1000);
}

void VideoPlayer::durationChanged(qint64 duration)
{
    ui->sliderPosition->setMaximum(duration / 1000);
}

void VideoPlayer::positionChanged(qint64 progress)
{
    if (!ui->sliderPosition->isSliderDown())
        ui->sliderPosition->setValue(progress / 1000);
}

void VideoPlayer::on_buttonPlay_clicked()
{
    switch (m_mediaPlayer->state()) {
    case QMediaPlayer::StoppedState:
    case QMediaPlayer::PausedState:
        m_mediaPlayer->play();
        break;
    case QMediaPlayer::PlayingState:
        m_mediaPlayer->pause();
        break;
    }
}

void VideoPlayer::on_buttonStop_clicked()
{
    m_mediaPlayer->stop();
}
