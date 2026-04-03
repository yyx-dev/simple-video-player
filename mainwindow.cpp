#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QStyle>
#include <QUrl>
#include <QDir>
#include <QMediaMetaData>
#include <QApplication>
#include <QScreen>
#include <QTimer>
#include <QKeyEvent>
#include <QPainter>
#include <QMimeData>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowIcon(coloredIcon(style()->standardIcon(QStyle::SP_MediaSeekForward)));
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    qApp->installEventFilter(this);

    qputenv("QT_MEDIA_BACKEND", "ffmpeg");
    qputenv("QT_FFMPEG_HWACCEL", "1");
    qputenv("QT_FFMPEG_DECODING_HW_DEVICE_TYPES", "d3d11va,dxva2");

    // 1. 初始化播放器核心
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);
    m_videoWidget = new QVideoWidget(this);
    m_videoWidget->setAcceptDrops(true);
    m_videoWidget->installEventFilter(this);
    m_player->setVideoOutput(m_videoWidget);

    // 2. 界面布局
    auto *centralWidget = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // 上方大视频区域（可拉伸）
    mainLayout->addWidget(m_videoWidget, 1);

    // ==================== 中间：进度条 + 时长显示 ====================
    auto *progressLayout = new QHBoxLayout();
    m_progressSlider = new QSlider(Qt::Horizontal, this);
    m_progressSlider->setRange(0, 0);
    m_progressSlider->setTracking(true);   // 拖动时实时更新
    progressLayout->addWidget(m_progressSlider);

    m_timeLabel = new QLabel("00:00 / 00:00", this);
    m_timeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    progressLayout->addWidget(m_timeLabel);

    mainLayout->addLayout(progressLayout);

    // ==================== 下方控制栏 ====================

    auto *controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(8);

    // 左侧：播放列表按钮
    m_playlistBtn = new QPushButton("播放列表", this);
    m_playlistBtn->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    m_playlistBtn->setFixedHeight(40);

    controlLayout->addWidget(m_playlistBtn);
    controlLayout->addStretch(1); // 中间左侧弹簧

    // 中间：上下集 左右跳 + 播放暂停（居中）
    m_prevBtn = new QPushButton(this);
    m_prevBtn->setIcon(coloredIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward)));
    m_prevBtn->setToolTip("上一集");
    m_nextBtn = new QPushButton(this);
    m_nextBtn->setIcon(coloredIcon(style()->standardIcon(QStyle::SP_MediaSkipForward)));
    m_nextBtn->setToolTip("下一集");
    m_skipBackBtn = new QPushButton(this);
    m_skipBackBtn->setIcon(coloredIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward)));
    m_skipBackBtn->setToolTip("后退 5 秒");
    m_skipBackBtn->setFixedSize(32, 32);
    m_skipForwardBtn = new QPushButton(this);
    m_skipForwardBtn->setIcon(coloredIcon(style()->standardIcon(QStyle::SP_MediaSeekForward)));
    m_skipForwardBtn->setToolTip("前进 5 秒");
    m_skipForwardBtn->setFixedSize(32, 32);
    m_playPauseBtn = new QPushButton(this);
    m_playPauseBtn->setIcon(coloredIcon(style()->standardIcon(QStyle::SP_MediaPlay)));
    m_playPauseBtn->setFixedSize(40, 40);

    controlLayout->addSpacing(100);
    controlLayout->addWidget(m_prevBtn);
    controlLayout->addSpacing(10);
    controlLayout->addWidget(m_skipBackBtn);
    controlLayout->addSpacing(10);
    controlLayout->addWidget(m_playPauseBtn);
    controlLayout->addSpacing(10);
    controlLayout->addWidget(m_skipForwardBtn);
    controlLayout->addSpacing(10);
    controlLayout->addWidget(m_nextBtn);

    controlLayout->addStretch(1); // 中间右侧弹簧

    // 右侧：音量和倍速（垂直排列）
    auto *rightPanel = new QVBoxLayout();
    rightPanel->setSpacing(6);

    // 音量行
    auto *volLayout = new QHBoxLayout();
    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setFixedWidth(120);
    m_volumeLabel = new QLabel("100%", this);

    volLayout->addWidget(new QLabel("音量"));
    volLayout->addWidget(m_volumeSlider);
    volLayout->addWidget(m_volumeLabel);
    rightPanel->addLayout(volLayout);

    // 倍速行
    auto *speedLayout = new QHBoxLayout();
    m_speedSlider = new QSlider(Qt::Horizontal, this);
    m_speedSlider->setRange(25, 300);   // 0.25x ~ 3.00x
    m_speedSlider->setValue(100);
    m_speedSlider->setTickInterval(25);
    m_speedSlider->setFixedWidth(120);
    m_speedLabel = new QLabel("1.00x", this);

    speedLayout->addWidget(new QLabel("倍速"));
    speedLayout->addWidget(m_speedSlider);
    speedLayout->addWidget(m_speedLabel);
    rightPanel->addLayout(speedLayout);

    controlLayout->addLayout(rightPanel);

    mainLayout->addLayout(controlLayout);
    setCentralWidget(centralWidget);

    // 3. 信号槽连接
    connect(m_playlistBtn, &QPushButton::clicked, this, &MainWindow::openPlaylistDialog);
    connect(m_playPauseBtn, &QPushButton::clicked, this, &MainWindow::togglePlayPause);
    connect(m_skipBackBtn, &QPushButton::clicked, this, &MainWindow::skipBackward);
    connect(m_skipForwardBtn, &QPushButton::clicked, this, &MainWindow::skipForward);
    connect(m_prevBtn, &QPushButton::clicked, this, &MainWindow::playPrevious);
    connect(m_nextBtn, &QPushButton::clicked, this, &MainWindow::playNext);

    connect(m_progressSlider, &QSlider::sliderReleased, this, [this] { seek(m_progressSlider->value()); });
    connect(m_player, &QMediaPlayer::positionChanged, this, &MainWindow::updatePosition);
    connect(m_player, &QMediaPlayer::durationChanged, this, &MainWindow::updateDuration);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &MainWindow::onPlaybackStateChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::metaDataChanged, this, &MainWindow::autoAdjustHeightToVideo);

    connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::setVolume);
    connect(m_speedSlider, &QSlider::valueChanged, this, &MainWindow::setPlaybackRate);

    // 4. 键盘快捷键支持
    m_fastTimer = new QTimer(this);
    m_fastTimer->setInterval(100);   // 每100ms触发一次加速

    connect(m_fastTimer, &QTimer::timeout, this, [this]() {
        if (m_player->playbackState() == QMediaPlayer::PlayingState) {
            m_player->setPlaybackRate(2.0);
        }
    });

    // 初始音量
    m_audioOutput->setVolume(1);

    // 持久化设置
    m_settings = new QSettings("YYX", "SimpleVideoPlayer", this);
    loadPlaylistAndPositions();
}

void MainWindow::playSingleFile(const QUrl &url)
{
    m_playlist.clear();
    m_playlist.append(url);
    m_currentIndex = 0;
    playCurrentFile();
}

void MainWindow::openPlaylistDialog()
{
    if (!m_playlistDialog) {
        m_playlistDialog = new PlaylistDialog(this);

        connect(m_playlistDialog, &PlaylistDialog::playlistUpdated,
                this, [this](const QList<QUrl>& list) {
                    m_playlist = list;
                    if (m_currentIndex >= m_playlist.size())
                        m_currentIndex = m_playlist.size() - 1;
                });

        connect(m_playlistDialog, &PlaylistDialog::playRequested,
                this, &MainWindow::onPlayRequested);
    }

    m_playlistDialog->setPlaylist(m_playlist, m_currentIndex);
    m_playlistDialog->show();
}

void MainWindow::onPlayRequested(int index)
{
    if (index < 0 || index >= m_playlist.size())
        return;

    m_currentIndex = index;
    playCurrentFile();
}

void MainWindow::playCurrentFile()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_playlist.size())
        return;

    const QUrl &url = m_playlist.at(m_currentIndex);
    m_player->setSource(url);
    m_player->pause();

    QString key = getFileKey(url);
    if (m_positions.contains(key)) {
        qint64 lastPos = m_positions[key];
        qint64 startPos = qMax(qint64(0), lastPos - 5000);
        m_player->setPosition(startPos);
    }

    // 更新窗口标题为当前文件名
    QString fileName = url.fileName();
    setWindowTitle(fileName.left(fileName.lastIndexOf('.')));
}

void MainWindow::togglePlayPause()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
    } else {
        m_player->play();
    }
}

void MainWindow::skipBackward()
{
    qint64 pos = m_player->position() - 5000;
    if (pos < 0) pos = 0;
    m_player->setPosition(pos);
}

void MainWindow::skipForward()
{
    qint64 pos = m_player->position() + 5000;
    if (pos > m_player->duration()) pos = m_player->duration();
    m_player->setPosition(pos);
}

void MainWindow::playPrevious()
{
    if (m_playlist.isEmpty()) return;
    if (--m_currentIndex < 0) m_currentIndex = m_playlist.size() - 1;
    playCurrentFile();
}

void MainWindow::playNext()
{
    if (m_playlist.isEmpty()) return;
    if (++m_currentIndex >= m_playlist.size()) m_currentIndex = 0;
    playCurrentFile();
}

void MainWindow::updatePosition(qint64 position)
{
    // 防止拖动进度条时产生循环
    if (!m_progressSlider->isSliderDown()) {
        m_progressSlider->setValue(static_cast<int>(position));
    }

    QString current = formatTime(position);
    QString total = formatTime(m_player->duration());
    m_timeLabel->setText(current + " / " + total);
}

void MainWindow::updateDuration(qint64 duration)
{
    m_progressSlider->setMaximum(static_cast<int>(duration));
    QString current = formatTime(m_player->position());
    QString total = formatTime(duration);
    m_timeLabel->setText(current + " / " + total);
}

void MainWindow::seek(int position)
{
    m_player->setPosition(position);
}

void MainWindow::setVolume(int volume)
{
    QString text;
    if (volume == 100) text = QString("%1%").arg(volume);
    else if (volume >= 10) text = QString(" %1% ").arg(volume);
    else text = QString("  %1%  ").arg(volume);

    m_volumeLabel->setText(text);
    m_audioOutput->setVolume(volume / 100.0);
}

void MainWindow::setPlaybackRate(int value)
{
    const QList<int> speedSteps = {25, 50, 75, 100, 125, 150, 175, 200, 250, 300};

    int closest = 100;
    int minDiff = INT_MAX;

    for (int step : speedSteps) {
        int diff = std::abs(value - step);
        if (diff < minDiff) {
            minDiff = diff;
            closest = step;
        }
    }

    if (closest != value) {
        m_speedSlider->setValue(closest);
        double rate = closest / 100.0;
        m_player->setPlaybackRate(rate);
        m_speedLabel->setText(QString::number(rate, 'f', 2) + "x");
    }
}

void MainWindow::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    if (state == QMediaPlayer::PlayingState) {
        m_playPauseBtn->setIcon(coloredIcon(style()->standardIcon(QStyle::SP_MediaPause)));
    } else {
        m_playPauseBtn->setIcon(coloredIcon(style()->standardIcon(QStyle::SP_MediaPlay)));
    }
}

void MainWindow::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        // 记录最终位置（防止还没保存就关闭）
        if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size()) {
            QString key = getFileKey(m_playlist.at(m_currentIndex));
            m_positions[key] = m_player->duration();   // 标记为已播放完毕
        }

        // 播放下一首
        if (m_currentIndex < m_playlist.size()) {
            playCurrentFile();
        } else {
            m_player->stop();
            m_currentIndex = -1;
        }

        savePlaylistAndPositions();   // 立即保存
    }
}

QString MainWindow::formatTime(qint64 ms) const
{
    if (ms < 0) ms = 0;

    int totalSeconds = static_cast<int>(ms / 1000);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    if (hours > 0) {
        // 超过1小时：显示 HH:MM:SS
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QLatin1Char('0'))
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    } else {
        // 小于1小时：显示 MM:SS
        return QString("%1:%2")
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    }
}

void MainWindow::autoAdjustHeightToVideo()
{
    if (!m_videoWidget)
        return;

    QSize videoResolution = m_player->metaData().value(QMediaMetaData::Resolution).toSize();
    if (videoResolution.isEmpty() || videoResolution.width() <= 0 || videoResolution.height() <= 0)
        return;
    double aspectRatio = static_cast<double>(videoResolution.width()) / videoResolution.height();

    int videoWidgetWidth = m_videoWidget->width();
    int desiredVideoHeight = static_cast<int>(videoWidgetWidth / aspectRatio);

    // 根据 videoWidget 当前宽度计算最合适的高度
    m_videoWidget->resize(videoWidgetWidth, desiredVideoHeight);
}

QIcon MainWindow::coloredIcon(const QIcon& icon)
{
    QPalette pal = QApplication::palette();
    QColor accentColor = pal.color(QPalette::Accent);

    QPixmap pixmap = icon.pixmap(48, 48);
    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), accentColor);
    painter.end();

    return QIcon(pixmap);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Space: // 空格键：播放/暂停
        togglePlayPause();
        break;

    case Qt::Key_Left: // 左箭头
        if (event->isAutoRepeat()) { // 长按
            if (!m_fastTimer->isActive()) {
                m_normalPlaybackRate = m_player->playbackRate();
                m_fastTimer->start();
                m_player->setPlaybackRate(2.0); // 立即开始 2x 快退
            }
        } else { // 单次按下
            skipBackward();
        }
        break;

    case Qt::Key_Right: // 右箭头
        if (event->isAutoRepeat()) { // 长按
            if (!m_fastTimer->isActive()) {
                m_normalPlaybackRate = m_player->playbackRate();
                m_fastTimer->start();
                m_player->setPlaybackRate(2.0); // 立即开始 2x 快进
            }
        } else { // 单次按下
            skipForward();
        }
        break;

    default:
        QMainWindow::keyPressEvent(event);
        break;
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) {
        if (m_fastTimer->isActive()) {
            m_fastTimer->stop();
            m_player->setPlaybackRate(m_normalPlaybackRate);  // 恢复原来的倍速
        }
    }
    QMainWindow::keyReleaseEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (!mimeData->hasUrls())
        return;

    QList<QUrl> urls = mimeData->urls();
    if (urls.isEmpty())
        return;

    bool hasNewFile = false;

    for (const QUrl &url : urls) {
        if (url.isLocalFile()) {
            QString suffix = QFileInfo(url.toLocalFile()).suffix().toLower();

            if (suffix == "mp4" || suffix == "mkv" || suffix == "avi" || suffix == "mov" ||
                suffix == "wmv" || suffix == "flv" || suffix == "webm" || suffix == "ts" ||
                suffix == "m4v" || suffix == "rmvb") {

                m_playlist.append(url);
                hasNewFile = true;
            }
        }
    }

    if (hasNewFile) {
        // 如果当前没有播放内容，则自动播放第一个新文件
        if (m_currentIndex < 0 || m_currentIndex >= m_playlist.size()) {
            m_currentIndex = m_playlist.size() - urls.size();
        }
        playCurrentFile();

        // 刷新播放列表对话框（如果打开的话）
        if (m_playlistDialog) {
            m_playlistDialog->setPlaylist(m_playlist, m_currentIndex);
        }
    }

    event->acceptProposedAction();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // if (watched == m_videoWidget) {
        if (event->type() == QEvent::DragEnter) {
            dragEnterEvent(static_cast<QDragEnterEvent*>(event));
            return true;
        }
        if (event->type() == QEvent::Drop) {
            dropEvent(static_cast<QDropEvent*>(event));
            return true;
        }
    // }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::loadPlaylistAndPositions()
{
    m_playlist.clear();
    m_positions.clear();

    // 读取保存的播放列表
    QStringList paths = m_settings->value("playlist").toStringList();
    for (const QString &path : paths) {
        if (!path.isEmpty())
            m_playlist.append(QUrl::fromLocalFile(path));
    }

    // 读取每个视频的播放位置
    m_settings->beginGroup("positions");
    QStringList keys = m_settings->childKeys();
    for (const QString &key : keys) {
        m_positions[key] = m_settings->value(key).toLongLong();
    }
    m_settings->endGroup();

    // 启动时清理已播放完毕的视频
    cleanFinishedVideos();

    // 如果还有视频，则播放第一个
    if (!m_playlist.isEmpty()) {
        m_currentIndex = 0;
        playCurrentFile();
    }
}

void MainWindow::savePlaylistAndPositions()
{
    // 保存播放列表（只存路径）
    QStringList paths;
    for (const QUrl &url : m_playlist) {
        paths << url.toLocalFile();
    }
    m_settings->setValue("playlist", paths);

    // 保存每个视频的播放位置
    m_settings->beginGroup("positions");
    m_settings->remove("");                     // 先清空旧数据
    for (auto it = m_positions.constBegin(); it != m_positions.constEnd(); ++it) {
        m_settings->setValue(it.key(), it.value());
    }
    m_settings->endGroup();

    m_settings->sync();   // 立即写入磁盘
}

QString MainWindow::getFileKey(const QUrl &url) const
{
    return url.toLocalFile();   // 使用绝对路径作为唯一key
}

void MainWindow::cleanFinishedVideos()
{
    QList<QUrl> newPlaylist;
    QMap<QString, qint64> newPositions;

    for (int i = 0; i < m_playlist.size(); ++i) {
        const QUrl &url = m_playlist.at(i);
        QString key = getFileKey(url);

        if (m_positions.contains(key)) {
            qint64 position = m_positions[key];
            qint64 duration = m_player->metaData().value(QMediaMetaData::Duration).toLongLong();

            // 如果播放位置达到总时长的 95% 以上，视为已播放完毕，删除
            if (duration > 0 && position >= duration * 0.95) {
                continue;   // 跳过已完成视频
            }
        }

        // 未完成或没有记录的视频保留
        newPlaylist.append(url);
        if (m_positions.contains(key)) {
            newPositions[key] = m_positions[key];
        }
    }

    m_playlist = newPlaylist;
    m_positions = newPositions;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // 程序关闭前保存当前播放位置
    if (m_player && m_currentIndex >= 0 && m_currentIndex < m_playlist.size()) {
        QString key = getFileKey(m_playlist.at(m_currentIndex));
        m_positions[key] = m_player->position();
    }

    savePlaylistAndPositions();
    QMainWindow::closeEvent(event);
}