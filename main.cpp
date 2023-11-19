#include <QtWidgets>
#include <QPixmap>
#include <QTimer>
#include <QImage>
#include <QLabel>
#include <QGridLayout>
#include <QThread>
#include <QMutex>
//#include <QtSql>

class ScreenCapture : public QWidget {
    Q_OBJECT

public:
    ScreenCapture(QWidget *parent = nullptr) : QWidget(parent) {
        startButton = new QPushButton("Start", this);
        stopButton = new QPushButton("Stop", this);
        similarityLabel = new QLabel("Similarity: N/A", this);
        runningIndecator = new QLabel("Program is not running", this);
        runningIndecator->setStyleSheet("color: red;");
        imageLabel = new QLabel(this);
        imageLabel->setFixedSize(700, 500);
        timerLabel = new QLabel("Time until next screenshot: N/A seconds", this);
        
        // Take a screenshot of the initial screen after launching the program
        previousScreenshot = QPixmap::grabWindow(QApplication::desktop()->winId());

        gridLayout = new QGridLayout(this);
        setFixedSize(800, 600);
        gridLayout->addWidget(startButton, 0, 0, 1, 1, Qt::AlignLeft);
        gridLayout->addWidget(stopButton, 0, 1, 1, 1, Qt::AlignRight);
        gridLayout->addWidget(similarityLabel, 1, 0, 1, 2, Qt::AlignCenter);
        gridLayout->addWidget(imageLabel, 2, 0, 1, 2, Qt::AlignCenter);
        gridLayout->addWidget(runningIndecator, 3, 0, 1, 2, Qt::AlignCenter);
        gridLayout->addWidget(timerLabel, 4, 0, 1, 2, Qt::AlignCenter);

        captureTimer = new QTimer(this);
        connect(captureTimer, &QTimer::timeout, this, &ScreenCapture::captureScreen);
        compareThread = new QThread(this);

        timeTrackerTimer = new QTimer(this);

        connect(startButton, &QPushButton::clicked, this, &ScreenCapture::startCapture);
        connect(stopButton, &QPushButton::clicked, this, &ScreenCapture::stopCapture);
        connect(this, &ScreenCapture::updateImage, this, &ScreenCapture::displayScreenshot, Qt::QueuedConnection);
        connect(timeTrackerTimer, &QTimer::timeout, this, &ScreenCapture::updateTimer);


        moveToThread(compareThread);
        compareThread->start();

        // Rescale screenshot of the initial screen to fit in ours program window and display it
        imageLabel->setPixmap(previousScreenshot.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    ~ScreenCapture() {
        compareThread->quit();
        compareThread->wait();
    }

signals:
    void updateImage(const QPixmap &screenshot);

public slots:
    void startCapture() {
        captureTimer->start(60000);
        timeTrackerTimer->start(1000);
        runningIndecator->setText(QString("Program is running"));
        runningIndecator->setStyleSheet("color: green;");
    }

    void stopCapture() {
        captureTimer->stop();
        runningIndecator->setText(QString("Program is not running"));
        runningIndecator->setStyleSheet("color: red;");
    }

    void captureScreen() {
        QPixmap screenshot = QPixmap::grabWindow(QApplication::desktop()->winId());
        emit updateImage(screenshot);

        /*This calls the compareScreenshots method for the current object (this) asynchronously in the main thread.
        This allows us to interact with the GUI in response to image changes,
        but avoids the problems associated with directly accessing GUI elements from other threads.*/
        QMetaObject::invokeMethod(this, "compareScreenshots", Qt::QueuedConnection, Q_ARG(QPixmap, screenshot));
    }

    void compareScreenshots(const QPixmap &screenshot) {
        double similarityPercentage = compareImages(screenshot, previousScreenshot);
        QMetaObject::invokeMethod(this, "updateUI", Qt::QueuedConnection, Q_ARG(double, similarityPercentage));
        previousScreenshot = screenshot;
    }

    void updateUI(double similarityPercentage) {
        similarityLabel->setText(QString("Similarity: %1%").arg(similarityPercentage, 0, 'f', 2));
    }

    void displayScreenshot(const QPixmap &screenshot) {
        //rescale screenshot so it can fit in our window
        QPixmap scaledPixmap = screenshot.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        imageLabel->setPixmap(scaledPixmap);
    }

    void updateTimer() {
        int remainingSeconds = captureTimer->remainingTime() / 1000;
        timerLabel->setText(QString("Time to next screenshot: %1 seconds").arg(remainingSeconds));
    }
private:
    double compareImages(const QPixmap &screenshot1, const QPixmap &screenshot2) {
        QImage img1 = screenshot1.toImage();
        QImage img2 = screenshot2.toImage();

        if (img1.size() != img2.size()) {
            qWarning("Images have different sizes");
            return 0.0;
        }

        int width = img1.width();
        int height = img1.height();

        int similarPixels = 0;
        QRgb pixel1;
        QRgb pixel2;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                pixel1 = img1.pixel(x, y);
                pixel2 = img2.pixel(x, y);

                if (pixel1 == pixel2) {
                    similarPixels++;
                }
            }
        }

        double similarityPercentage = (static_cast<double>(similarPixels) / (width * height)) * 100.0;
        return similarityPercentage;
    }

private:
    QPushButton *startButton;
    QPushButton *stopButton;
    QLabel *similarityLabel;
    QLabel *runningIndecator;
    QLabel *imageLabel;
    QGridLayout *gridLayout;
    QTimer *captureTimer;
    QTimer *timeTrackerTimer;
    QLabel *timerLabel;
    QThread *compareThread;
    QPixmap previousScreenshot;
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    ScreenCapture screenCapture;
    screenCapture.show();

    return a.exec();
}

#include "main.moc"
