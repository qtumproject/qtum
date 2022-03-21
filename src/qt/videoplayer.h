#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QDialog>
#include <QMediaPlayer>

namespace Ui {
class VideoPlayer;
}

class VideoPlayer : public QDialog
{
    Q_OBJECT

public:
    explicit VideoPlayer(QString videoPath, QWidget *parent = nullptr);
    ~VideoPlayer();

private slots:
    void setState(QMediaPlayer::State state);
    void seek(int seconds);
    void durationChanged(qint64 duration);
    void positionChanged(qint64 progress);

    void on_buttonPlay_clicked();
    void on_buttonStop_clicked();

private:
    Ui::VideoPlayer *ui;
    QMediaPlayer *m_mediaPlayer;
};

#endif // VIDEOPLAYER_H
